//=============================================================================
//  MuseScore
//  Linux Music Score Editor
//
//  Copyright (C) 2002-2011 Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#include <cmath>

#include "click.h"
#include "config.h"
#include "musescore.h"
#include "pianotools.h"
#include "playpanel.h"
#include "preferences.h"
#include "scoreview.h"
#include "seq.h"
#include "synthcontrol.h"

#include "audio/midi/msynthesizer.h"

#ifdef HAS_AUDIOFILE
#include "audiofile/audiofile.h"
#endif

#include <QFile>

#include "libmscore/audio.h"
#include "libmscore/chord.h"
#include "libmscore/measure.h"
#include "libmscore/note.h"
#include "libmscore/part.h"
#include "libmscore/rendermidi.h"
#include "libmscore/repeatlist.h"
#include "libmscore/score.h"
#include "libmscore/segment.h"
#include "libmscore/sig.h"
#include "libmscore/staff.h"
#include "libmscore/tempo.h"
#include "libmscore/tie.h"
#include "libmscore/utils.h"

#include "pianoroll/pianoroll.h"

#ifdef USE_PORTMIDI
#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
  #include "portmidi/porttime/porttime.h"
#else
  #include <porttime.h>
#endif
#endif

#define OV_EXCLUDE_STATIC_CALLBACKS
#include <vorbis/vorbisfile.h>

namespace Ms {

Seq* seq;

static const int guiRefresh   = 10;       // Hz
static const int peakHoldTime = 1400;     // msec
static const int peakHold     = (peakHoldTime * guiRefresh) / 1000;
static OggVorbis_File vf;

static constexpr int minUtickBufferSize = 480 * 4 * 10; // about 10 measures of 4/4 time signature

#if 0 // yet(?) unused
static const int AUDIO_BUFFER_SIZE = 1024 * 512;  // 2 MB
#endif

//---------------------------------------------------------
//   VorbisData
//---------------------------------------------------------

struct VorbisData {
      int pos;          // current position in audio->data()
      QByteArray data;
      };

static VorbisData vorbisData;

static size_t ovRead(void* ptr, size_t size, size_t nmemb, void* datasource);
static int ovSeek(void* datasource, ogg_int64_t offset, int whence);
static long ovTell(void* datasource);

static ov_callbacks ovCallbacks = {
      ovRead, ovSeek, 0, ovTell
      };

//---------------------------------------------------------
//   ovRead
//---------------------------------------------------------

static size_t ovRead(void* ptr, size_t size, size_t nmemb, void* datasource)
      {
      VorbisData* vd = (VorbisData*)datasource;
      size_t n = size * nmemb;
      if (vd->data.size() < int(vd->pos + n))
            n = vd->data.size() - vd->pos;
      if (n) {
            const char* src = vd->data.data() + vd->pos;
            memcpy(ptr, src, n);
            vd->pos += int(n);
            }
      return n;
      }

//---------------------------------------------------------
//   ovSeek
//---------------------------------------------------------

static int ovSeek(void* datasource, ogg_int64_t offset, int whence)
      {
      VorbisData* vd = (VorbisData*)datasource;
      switch(whence) {
            case SEEK_SET:
                  vd->pos = (int)offset;
                  break;
            case SEEK_CUR:
                  vd->pos += (int)offset;
                  break;
            case SEEK_END:
                  vd->pos = vd->data.size() - (int)offset;
                  break;
            }
      return 0;
      }

//---------------------------------------------------------
//   ovTell
//---------------------------------------------------------

static long ovTell(void* datasource)
      {
      VorbisData* vd = (VorbisData*)datasource;
      return vd->pos;
      }

//---------------------------------------------------------
//   Seq
//---------------------------------------------------------

Seq::Seq()
   : midi(nullptr)
      {
      running         = false;
      playlistChanged = false;
      cs              = 0;
      cv              = 0;
      tackRemain            = 0;
      tickRemain            = 0;
      independentTackRemain = 0;
      independentTickRemain = 0;
      independentTackVolume = 1.0;
      independentTickVolume = 1.0;
      maxMidiOutPort        = 0;

      endUTick  = 0;
      state    = Transport::STOP;
      oggInit  = false;
      _driver  = 0;
      playPos  = events.cbegin();
      playFrame  = 0;

      metronomeVolumeValue =
            qBound(0.0,
                   preferences.getDouble(PREF_APP_PLAYBACK_METRONOME_VOLUME),
                   10.0); // 10.0 upper bound corresponds to the VolSlider maximum of +20dB

      useJackTransportSavedFlag = false;

      inCountIn         = false;
      countInPlayPos    = countInEvents.cbegin();
      countInPlayFrame  = 0;

      meterValue[0]     = 0.0;
      meterValue[1]     = 0.0;
      meterPeakValue[0] = 0.0;
      meterPeakValue[1] = 0.0;
      peakTimer[0]       = 0;
      peakTimer[1]       = 0;

      heartBeatTimer = new QTimer(this);
      connect(heartBeatTimer, SIGNAL(timeout()), this, SLOT(heartBeatTimeout()));

      noteTimer = new QTimer(this);
      noteTimer->setSingleShot(true);
      connect(noteTimer, SIGNAL(timeout()), this, SLOT(stopNotes()));
      noteTimer->stop();

      independentMetronomeEnabledValue = false;

      independentMetronomeBpmValue =
            preferences.getDouble(PREF_APP_PLAYBACK_INDEPENDENT_METRONOME_BPM);

      independentMetronomeNumeratorValue =
            preferences.getInt(PREF_APP_PLAYBACK_INDEPENDENT_METRONOME_NUMERATOR);

      independentMetronomeDenominatorValue =
            preferences.getInt(PREF_APP_PLAYBACK_INDEPENDENT_METRONOME_DENOMINATOR);

      independentMetronomeFollowPlaybackValue =
            preferences.getBool(PREF_APP_PLAYBACK_INDEPENDENT_METRONOME_FOLLOW);

      independentMetronomeBeatAccentsValue =
            preferences.getBool(PREF_APP_PLAYBACK_INDEPENDENT_METRONOME_ACCENTS);

      if (independentMetronomeBpmValue < 20.0
          || independentMetronomeBpmValue > 400.0) {
            independentMetronomeBpmValue = 120.0;
            }

      if (independentMetronomeNumeratorValue < 1
          || independentMetronomeNumeratorValue > 32) {
            independentMetronomeNumeratorValue = 4;
            }

      if (independentMetronomeDenominatorValue <= 0
          || (independentMetronomeDenominatorValue
              & (independentMetronomeDenominatorValue - 1)) != 0
          || (DIVISION * 4) % independentMetronomeDenominatorValue != 0) {
            independentMetronomeDenominatorValue = 4;
            }

      syncIndependentMetronomeRealtimeConfig();

      connect(this, SIGNAL(toGui(int, int)), this, SLOT(seqMessage(int, int)), Qt::QueuedConnection);

      prevTimeSig.setNumerator(0);
      prevTempo = 0;
      connect(this, SIGNAL(timeSigChanged()),this,SLOT(handleTimeSigTempoChanged()));
      connect(this, SIGNAL(tempoChanged()),this,SLOT(handleTimeSigTempoChanged()));

      initialMillisecondTimestampWithLatency = 0;
      }

//---------------------------------------------------------
//   Seq
//---------------------------------------------------------

Seq::~Seq()
      {
      delete _driver;
      }

//---------------------------------------------------------
//   setScoreView
//---------------------------------------------------------

void Seq::setScoreView(ScoreView* v)
      {
      if (oggInit) {
            ov_clear(&vf);
            oggInit = false;
            }
      if (cv !=v && cs) {
            unmarkNotes();
            stopWait();
            }
      cv = v;
      if (cs)
            disconnect(cs, SIGNAL(playlistChanged()), this, SLOT(setPlaylistChanged()));
      cs = cv ? cv->score()->masterScore() : 0;
      midi = MidiRenderer(cs);
      midi.setMinChunkSize(10);

      if (!heartBeatTimer->isActive())
            heartBeatTimer->start(20);    // msec

      playlistChanged = true;
      _synti->reset();
      if (cs) {
            initInstruments();
            connect(cs, SIGNAL(playlistChanged()), this, SLOT(setPlaylistChanged()));
            }
      }

//---------------------------------------------------------
//   preferencesChanged
//---------------------------------------------------------

void Seq::preferencesChanged()
      {
      cachedPrefs.update();
      reloadMetronomeSamples();
      }

//---------------------------------------------------------
//   Seq::CachedPreferences::update
//---------------------------------------------------------

void Seq::CachedPreferences::update()
      {
#ifdef USE_PORTMIDI
      portMidiOutputLatencyMilliseconds = preferences.getInt(PREF_IO_PORTMIDI_OUTPUTLATENCYMILLISECONDS);
#endif
      jackTimeBaseMaster = preferences.getBool(PREF_IO_JACK_TIMEBASEMASTER);
      useJackTransport = preferences.getBool(PREF_IO_JACK_USEJACKTRANSPORT);
      useJackMidi = preferences.getBool(PREF_IO_JACK_USEJACKMIDI);
      useJackAudio = preferences.getBool(PREF_IO_JACK_USEJACKAUDIO);
      useAlsaAudio = preferences.getBool(PREF_IO_ALSA_USEALSAAUDIO);
#ifdef USE_PORTAUDIO
      usePortAudio = preferences.getBool(PREF_IO_PORTAUDIO_USEPORTAUDIO);
#endif
      usePulseAudio = preferences.getBool(PREF_IO_PULSEAUDIO_USEPULSEAUDIO);
      }

//---------------------------------------------------------
//   startTransport
//---------------------------------------------------------

void Seq::startTransport()
      {
      cachedPrefs.update();
      _driver->startTransport();
      }

//---------------------------------------------------------
//   stopTransport
//---------------------------------------------------------

void Seq::stopTransport()
      {
      _driver->stopTransport();
      }

//---------------------------------------------------------
//   init
//    return false on error
//---------------------------------------------------------

bool Seq::init(bool hotPlug)
      {
      // Rebuild realtime metronome state before the audio
      // callback can start running
      syncIndependentMetronomeRealtimeConfig();

      if (!_driver || !_driver->start(hotPlug)) {
            qDebug("Cannot start I/O");
            running = false;
            return false;
            }

      cachedPrefs.update();

      // The driver has established the output sample rate
      reloadMetronomeSamples(true);

      running = true;
      return true;
      }

//---------------------------------------------------------
//   exit
//---------------------------------------------------------

void Seq::exit()
      {
      if (_driver) {
            if (MScore::debugMode)
                  qDebug("Stop I/O");
            stopWait();
            delete _driver;
            _driver = 0;
            }
      }

//---------------------------------------------------------
//   rewindStart
//---------------------------------------------------------

void Seq::rewindStart()
      {
      seek(0);
      }

//---------------------------------------------------------
//   loopStart
//---------------------------------------------------------

void Seq::loopStart()
      {
      start();
//      qDebug("LoopStart. playPos = %d", playPos);
      }

//---------------------------------------------------------
//   canStart
//    return true if sequencer can be started
//---------------------------------------------------------

bool Seq::canStart()
      {
      if (!_driver)
            return false;
      collectEvents(getPlayStartUtick());
      return (!events.empty() && endUTick != 0);
      }

//---------------------------------------------------------
//   start
//    called from gui thread
//---------------------------------------------------------

void Seq::start()
      {
      if (!_driver) {
            qDebug("No driver!");
            return;
            }

      mscore->moveControlCursor();

      allowBackgroundRendering = true;
      collectEvents(getPlayStartUtick());
      if (cs->playMode() == PlayMode::AUDIO) {
            if (!oggInit) {
                  vorbisData.pos  = 0;
                  vorbisData.data = cs->audio()->data();
                  int n = ov_open_callbacks(&vorbisData, &vf, 0, 0, ovCallbacks);
                  if (n < 0) {
                        qDebug("ogg open failed: %d", n);
                        }
                  oggInit = true;
                  }
            }

      if (!preferences.getBool(PREF_IO_JACK_USEJACKTRANSPORT) || (preferences.getBool(PREF_IO_JACK_USEJACKTRANSPORT) && state == Transport::STOP))
            seek(getPlayStartUtick());

      if (preferences.getBool(PREF_IO_JACK_USEJACKTRANSPORT) && mscore->countIn() && state == Transport::STOP) {
            // Ready to start playing count in, switching to fake transport
            // to prevent playing in other applications with our ticks simultaneously
            useJackTransportSavedFlag    = true;
            preferences.setPreference(PREF_IO_JACK_USEJACKTRANSPORT, false);
            }
      startTransport();
      }

//---------------------------------------------------------
//   stop
//    called from gui thread
//---------------------------------------------------------

void Seq::stop()
      {
      const bool seqStopped = (state == Transport::STOP);
      const bool driverStopped = !_driver || _driver->getState() == Transport::STOP;
      if (seqStopped && driverStopped)
            return;

      allowBackgroundRendering = false;
      if (oggInit) {
            ov_clear(&vf);
            oggInit = false;
            }
      if (!_driver)
            return;
      if (!preferences.getBool(PREF_IO_JACK_USEJACKTRANSPORT) || (preferences.getBool(PREF_IO_JACK_USEJACKTRANSPORT) && _driver->getState() == Transport::PLAY))
            stopTransport();
      if (cv)
            cv->setCursorOn(false);
      if (midiRenderFuture.isRunning())
            midiRenderFuture.waitForFinished();
      if (cs) {
            cs->setUpdateAll();
            cs->update();
            }
      }

//---------------------------------------------------------
//   stopWait
//---------------------------------------------------------

void Seq::stopWait()
      {
      stop();
      QWaitCondition sleep;
#ifndef NDEBUG
      int idx = 0;
#endif
      while (state != Transport::STOP) {
            qDebug("State %d", (int)state);
            mutex.lock();
            sleep.wait(&mutex, 100);
            mutex.unlock();
#ifndef NDEBUG
            Q_ASSERT(++idx <= 10);
#endif
            }
      }

//---------------------------------------------------------
//   seqStarted
//---------------------------------------------------------

void MuseScore::seqStarted()
      {
      if (cv)
            cv->setCursorOn(true);
      if (cs)
            cs->update();
      }

//---------------------------------------------------------
//   seqStopped
//    JACK has stopped
//    executed in gui environment
//---------------------------------------------------------

void MuseScore::seqStopped()
      {
      cv->setCursorOn(false);
      }

//---------------------------------------------------------
//   unmarkNotes
//---------------------------------------------------------

void Seq::unmarkNotes()
      {
      foreach(const Note* n, markedNotes) {
            n->setMark(false);
            cs->addRefresh(n->canvasBoundingRect());
            }
      markedNotes.clear();
      PianoTools* piano = mscore->pianoTools();
      if (piano && piano->isVisible())
            piano->setPlaybackNotes(markedNotes);
      }

//---------------------------------------------------------
//   guiStop
//---------------------------------------------------------

void Seq::guiStop()
      {
      QAction* a = getAction("play");
      a->setChecked(false);

      unmarkNotes();
      if (!cs)
            return;

      int tck = cs->repeatList().utick2tick(cs->utime2utick(qreal(playFrame) / qreal(MScore::sampleRate)));
      cs->setPlayPos(Fraction::fromTicks(tck));
      cs->update();
      emit stopped();
      }

//---------------------------------------------------------
//   seqSignal
//    sequencer message to GUI
//    execution environment: gui thread
//---------------------------------------------------------

void Seq::seqMessage(int msg, int arg)
      {
      switch(msg) {
            case '5': {
                  // Update the screen after seeking from the realtime thread
                  Segment* seg = cs->tick2segment(Fraction::fromTicks(arg));
                  if (seg)
                        mscore->currentScoreView()->moveCursor(seg->tick());
                  cs->setPlayPos(Fraction::fromTicks(arg));
                  cs->update();
                  break;
                  }
            case '4':   // Restart the playback at the end of the score
                  loopStart();
                  break;
            case '3':   // Loop restart while playing
                  seek(cs->repeatList().tick2utick(cs->loopInTick().ticks()));
                  break;
            case '2':
                  guiStop();
//                  heartBeatTimer->stop();
                  if (_driver && mscore->getSynthControl()) {
                        meterValue[0]     = .0f;
                        meterValue[1]     = .0f;
                        meterPeakValue[0] = .0f;
                        meterPeakValue[1] = .0f;
                        peakTimer[0]       = 0;
                        peakTimer[1]       = 0;
                        mscore->getSynthControl()->setMeter(0.0, 0.0, 0.0, 0.0);
                        }
                  seek(0);
                  break;
            case '0':         // STOP
                  guiStop();
//                  heartBeatTimer->stop();
                  if (_driver && mscore->getSynthControl()) {
                        meterValue[0]     = .0f;
                        meterValue[1]     = .0f;
                        meterPeakValue[0] = .0f;
                        meterPeakValue[1] = .0f;
                        peakTimer[0]       = 0;
                        peakTimer[1]       = 0;
                        mscore->getSynthControl()->setMeter(0.0, 0.0, 0.0, 0.0);
                        }
                  break;

            case '1':         // PLAY
                  emit started();
//                  heartBeatTimer->start(1000/guiRefresh);
                  break;

            default:
                  qDebug("MScore::Seq:: unknown seq msg %d", msg);
                  break;
            }
      }

//---------------------------------------------------------
//   playEvent
//    send one event to the synthesizer
//---------------------------------------------------------

void Seq::playEvent(const NPlayEvent& event, unsigned framePos)
      {
      int type = event.type();
      PianoTools* piano = mscore->pianoTools();
      if (type == ME_NOTEON) {
            if (!event.isMuted()) {
                  if (event.discard()) { // ignore noteoff but restrike noteon
                        if (event.velo() > 0)
                              putEvent(NPlayEvent(ME_NOTEON, event.channel(), event.pitch(), 0) ,framePos);
                        else
                              return;
                        }
                  putEvent(event, framePos);
                  if (piano && piano->isVisible()) {
                        if (event.velo() > 0)
                              piano->pressPlaybackPitch(event.pitch());
                        else // Note-offs are synthesized as ME_NOTEON with 0 velocity
                              piano->releasePlaybackPitch(event.pitch());
                        }
                  }
            }
      else if (type == ME_CONTROLLER || type == ME_PITCHBEND || type == ME_AFTERTOUCH || type == ME_POLYAFTER)
            putEvent(event, framePos);
      }

//---------------------------------------------------------
//   recomputeMaxMidiOutPort
//   Computes the maximum number of midi out ports
//   in all opened scores
//---------------------------------------------------------

void Seq::recomputeMaxMidiOutPort()
      {
      if (!(preferences.getBool(PREF_IO_JACK_USEJACKMIDI) || preferences.getBool(PREF_IO_ALSA_USEALSAAUDIO)))
            return;
      int max = 0;
      for (Score * s : MuseScoreCore::mscoreCore->scores()) {
            if (s->masterScore()->midiPortCount() > max)
                  max = s->masterScore()->midiPortCount();
            }
      maxMidiOutPort = max;
      }

//---------------------------------------------------------
//   processMessages
//   from gui to process thread
//---------------------------------------------------------

void Seq::processMessages()
      {
      for (;;) {
            if (toSeq.empty())
                  break;
            SeqMsg msg = toSeq.dequeue();
            switch(msg.id) {
                  case SeqMsgId::TEMPO_CHANGE:
                        {
                        if (!cs)
                              continue;
                        if (playFrame != 0) {
                              int utick = cs->utime2utick(qreal(playFrame) / qreal(MScore::sampleRate));
                              cs->tempomap()->setRelTempo(msg.realVal);
                              playFrame = cs->utick2utime(utick) * MScore::sampleRate;
                              if (cachedPrefs.jackTimeBaseMaster && cachedPrefs.useJackTransport)
                                    _driver->seekTransport(utick + 2 * cs->utime2utick(qreal((_driver->bufferSize()) + 1) / qreal(MScore::sampleRate)));
                              }
                        else
                              cs->tempomap()->setRelTempo(msg.realVal);
                        cs->masterScore()->updateRepeatListTempo();
                        prevTempo = curTempo();
                        emit tempoChanged();
                        }
                        break;
                  case SeqMsgId::PLAY:
                        putEvent(msg.event);
                        break;
                  case SeqMsgId::SEEK:
                        setPos(msg.intVal);
                        break;
                  case SeqMsgId::ALL_NOTE_OFF:
                        {
                        _synti->allNotesOff(msg.intVal);
                        PianoTools* piano = mscore->pianoTools();
                        // PianoTools here returns the playback pitches by-value
                        // to ensure no discrepancy while erasing items from
                        // the _pressedPlaybackPitches container
                        for (int pressedPitch : piano->pressedPlaybackPitches())
                              piano->releasePlaybackPitch(pressedPitch);
                        }
                        break;
                  case SeqMsgId::INDEPENDENT_METRONOME_ENABLED:
                        independentMetronomeEnabledRT = msg.intVal != 0;

                        resetIndependentMetronomeRealtimeState();

                        if (!independentMetronomeEnabledRT)
                              independentMetronomeFollowPlaybackActiveRT = false;
                        break;

                  case SeqMsgId::INDEPENDENT_METRONOME_BPM:
                        independentMetronomeBpmRT = msg.realVal;

                        // Changing BPM starts a complete interval at the new tempo
                        if (independentMetronomeEnabledRT
                            && !independentMetronomeFollowPlaybackActiveRT) {
                              independentMetronomeFramesUntilNextClick =
                                    independentMetronomeFramesPerClick();
                              }
                        break;

                  case SeqMsgId::INDEPENDENT_METRONOME_TIME_SIGNATURE:
                        independentMetronomeNumeratorRT =
                              msg.intVal;
                        independentMetronomeDenominatorRT =
                              msg.intVal2;

                        independentMetronomeRtick = 0;

                        if (independentMetronomeEnabledRT
                            && !independentMetronomeFollowPlaybackActiveRT) {
                              independentMetronomeFramesUntilNextClick =
                                    independentMetronomeFramesPerClick();
                              }
                        break;

                  case SeqMsgId::INDEPENDENT_METRONOME_FOLLOW:
                        independentMetronomeFollowPlaybackRT =
                              msg.intVal != 0;
                        break;

                  case SeqMsgId::INDEPENDENT_METRONOME_ACCENTS:
                        independentMetronomeBeatAccentsRT =
                              msg.intVal != 0;
                        break;

                  case SeqMsgId::METRONOME_GAIN:
                        metronomeVolume = msg.realVal;
                        break;

                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   metronome
//---------------------------------------------------------

void Seq::metronome(unsigned n, float* p, bool force)
      {
      if (!mscore->metronome() && !force) {
            tickRemain = 0;
            tackRemain = 0;

            _tickCustomSampleInUse.store(nullptr);
            _tackCustomSampleInUse.store(nullptr);

            return;
            }

      if (tickRemain) {
            const MetronomeCustomSample* sample =
                  _tickCustomSampleInUse.load();

            mixMetronomeSample(
                  n, p,
                  tickRemain,
                  tickVolume,
                  sample,
                  metronomeTick,
                  tickLength);

            if (!tickRemain) {
                  _tickCustomSampleInUse.store(nullptr);
                  }
            }

      if (tackRemain) {
            const MetronomeCustomSample* sample =
                  _tackCustomSampleInUse.load();

            mixMetronomeSample(
                  n, p,
                  tackRemain,
                  tackVolume,
                  sample,
                  metronomeTack,
                  tackLength);

            if (!tackRemain) {
                  _tackCustomSampleInUse.store(nullptr);
                  }
            }
      }

//---------------------------------------------------------
//   mixIndependentMetronomeSamples
//---------------------------------------------------------

void Seq::mixIndependentMetronomeSamples(unsigned n, float* p)
      {
      if (independentTickRemain) {
            const MetronomeCustomSample* sample =
                  _independentTickCustomSampleInUse.load();

            mixMetronomeSample(
                  n, p,
                  independentTickRemain,
                  independentTickVolume,
                  sample,
                  metronomeTick,
                  tickLength);

            if (!independentTickRemain) {
                  _independentTickCustomSampleInUse.store(nullptr);
                  }
            }

      if (independentTackRemain) {
            const MetronomeCustomSample* sample =
                  _independentTackCustomSampleInUse.load();

            mixMetronomeSample(
                  n, p,
                  independentTackRemain,
                  independentTackVolume,
                  sample,
                  metronomeTack,
                  tackLength);

            if (!independentTackRemain) {
                  _independentTackCustomSampleInUse.store(nullptr);
                  }
            }
      }

//---------------------------------------------------------
//   independentMetronome
//---------------------------------------------------------

void Seq::independentMetronome(unsigned n, float* p)
      {
      if (!independentMetronomeEnabledRT)
            return;

      if (independentMetronomeFollowPlaybackActiveRT) {
            mixIndependentMetronomeSamples(n, p);
            return;
            }

      unsigned frame = 0;

      while (frame < n) {
            // Start any click whose musical position has arrived:
            if (independentMetronomeFramesUntilNextClick <= 0.0) {
                  const TimeSigFrac timeSig(
                        independentMetronomeNumeratorRT,
                        independentMetronomeDenominatorRT);

                  const BeatType type =
                        timeSig.rtick2beatType(
                              independentMetronomeRtick);

                  const NPlayEvent event(type);
                  const qreal volume =
                        independentMetronomeEventVolume(event);

                  if (event.type() == ME_TICK1)
                        startMetronomeTick(volume, true);
                  else if (event.type() == ME_TICK2)
                        startMetronomeTack(volume, true);

                  const int clickTicks =
                        independentMetronomeClickTicks();

                  independentMetronomeRtick =
                        (independentMetronomeRtick + clickTicks)
                        % timeSig.ticksPerMeasure();

                  // Add rather than assign. If a fractional
                  // frame crossed the boundary by part of a frame,
                  // that error is carried into the next interval
                  // rather than accumulating
                  independentMetronomeFramesUntilNextClick +=
                        independentMetronomeFramesPerClick();
                  }

            const unsigned framesRemaining = n - frame;

            unsigned framesToClick =
                  unsigned(std::ceil(
                        independentMetronomeFramesUntilNextClick));

            if (framesToClick == 0)
                  framesToClick = 1;

            const unsigned framesToProcess =
                  qMin(framesRemaining, framesToClick);

            mixIndependentMetronomeSamples(
                  framesToProcess,
                  p + frame * 2);

            frame += framesToProcess;

            independentMetronomeFramesUntilNextClick -=
                  framesToProcess;
            }
      }

//---------------------------------------------------------
//   resetIndependentMetronomeRealtimeState
//   This must run before audio starts or from
//   the realtime thread
//---------------------------------------------------------

void Seq::resetIndependentMetronomeRealtimeState()
      {
      independentMetronomeRtick = 0;
      independentMetronomeFramesUntilNextClick = 0.0;

      independentTickRemain = 0;
      independentTackRemain = 0;

      _independentTickCustomSampleInUse.store(nullptr);
      _independentTackCustomSampleInUse.store(nullptr);
      }

//---------------------------------------------------------
//   syncIndependentMetronomeRealtimeConfig
//---------------------------------------------------------

void Seq::syncIndependentMetronomeRealtimeConfig()
      {
      independentMetronomeEnabledRT =
            independentMetronomeEnabledValue;

      independentMetronomeBpmRT =
            independentMetronomeBpmValue;

      independentMetronomeNumeratorRT =
            independentMetronomeNumeratorValue;

      independentMetronomeDenominatorRT =
            independentMetronomeDenominatorValue;

      independentMetronomeFollowPlaybackRT =
            independentMetronomeFollowPlaybackValue;

      independentMetronomeBeatAccentsRT =
            independentMetronomeBeatAccentsValue;

      metronomeVolume = metronomeVolumeValue;

      independentMetronomeFollowPlaybackActiveRT = false;

      resetIndependentMetronomeRealtimeState();
      }

//---------------------------------------------------------
//   updateIndependentMetronomeFollowState
//---------------------------------------------------------

void Seq::updateIndependentMetronomeFollowState()
      {
      const bool followActive =
            independentMetronomeEnabledRT
            && independentMetronomeFollowPlaybackRT
            && state == Transport::PLAY;

      if (followActive == independentMetronomeFollowPlaybackActiveRT)
            return;

      // Crossing either boundary starts with clean audio state

      // Enter Follow:
      //   discard the free-running click/phase and let the score's
      //   next ME_TICK event establish playback timing

      // Leave Follow:
      //   re-arm the manual clock from a new downbeat

      resetIndependentMetronomeRealtimeState();

      independentMetronomeFollowPlaybackActiveRT =
            followActive;
      }

//---------------------------------------------------------
//   addCountInClicks
//---------------------------------------------------------

void Seq::addCountInClicks()
      {
      const Fraction plPos = cs->playPos();
      Measure*    m        = cs->tick2measure(plPos);
      Fraction   msrTick   = m->tick();
      qreal tempo          = cs->tempomap()->tempo(msrTick.ticks());
      TimeSigFrac timeSig  = cs->sigmap()->timesig(m->tick()).nominal();

      const int clickTicks = timeSig.isBeatedCompound(tempo) ? timeSig.beatTicks() : timeSig.dUnitTicks();

      // add at least one full measure of just clicks.
      Fraction endTick = Fraction::fromTicks(timeSig.ticksPerMeasure());

      // add extra clicks if...
      endTick += plPos - msrTick;   // ...not starting playback at beginning of measure

      if (m->isAnacrusis())         // ...measure is incomplete (anacrusis)
            endTick += Fraction::fromTicks(timeSig.ticksPerMeasure()) - m->ticks();

      for (int t = 0; t < endTick.ticks(); t += clickTicks) {
            const int rtick = t % timeSig.ticksPerMeasure();
            countInEvents.insert(std::pair<int,NPlayEvent>(t, NPlayEvent(timeSig.rtick2beatType(rtick))));
            }

      NPlayEvent event;
      event.setType(ME_INVALID);
      event.setPitch(0);
      countInEvents.insert( std::pair<int,NPlayEvent>(endTick.ticks(), event));
      // initialize play parameters to count-in events
      countInPlayPos  = countInEvents.cbegin();
      countInPlayFrame = 0;
      }

//-------------------------------------------------------------------
//   process
//    This function is called in a realtime context. This
//    means that no blocking operations are allowed which
//    includes memory allocation. The usual thread synchronization
//    methods like semaphores can also not be used.
//-------------------------------------------------------------------

void Seq::process(unsigned framesPerPeriod, float* buffer)
      {
      unsigned framesRemain = framesPerPeriod; // the number of frames remaining to be processed by this call to Seq::process
      Transport driverState = _driver->getState();
      // Checking for the reposition from JACK Transport
      _driver->checkTransportSeek(playFrame, framesRemain, inCountIn);

      if (driverState != state) {
            // Got a message from JACK Transport panel: Play
            if (state == Transport::STOP && driverState == Transport::PLAY) {
                  if ((cachedPrefs.useJackMidi || cachedPrefs.useJackAudio) && !getAction("play")->isChecked()) {
                        // Do not play while editing elements
                        if (mscore->state() != STATE_NORMAL || !isRunning() || !canStart())
                              return;
                        getAction("play")->setChecked(true);
                        getAction("play")->triggered(true);

                        // If we just launch MuseScore and press "Play" on JACK Transport with time 0:00
                        // MuseScore doesn't seek to 0 and guiPos is uninitialized, so let's make it manually
                        if (cachedPrefs.useJackTransport && getCurTick() == 0)
                              seekRT(0);

                        // Switching to fake transport while playing count in
                        // to prevent playing in other applications with our ticks simultaneously
                        if (cachedPrefs.useJackTransport && mscore->countIn()) {
                              // Stopping real JACK Transport
                              stopTransport();
                              // Starting fake transport
                              useJackTransportSavedFlag = preferences.getBool(PREF_IO_JACK_USEJACKTRANSPORT);
                              preferences.setPreference(PREF_IO_JACK_USEJACKTRANSPORT, false);
                              startTransport();
                              }
                        }
                  // Initializing instruments every time we start playback.
                  // External synth can have wrong values, for example
                  // if we switch between scores
                  initInstruments(true);
                  // Need to change state after calling collectEvents()
                  state = Transport::PLAY;
                  if (mscore->countIn() && cs->playMode() == PlayMode::SYNTHESIZER) {
                        countInEvents.clear();
                        inCountIn = true;
                        }
                  emit toGui('1');
                  }
            // Got a message from JACK Transport panel: Stop
            else if (state == Transport::PLAY && driverState == Transport::STOP) {
                  state = Transport::STOP;
                  // Muting all notes
                  stopNotes(-1, true);
                  initInstruments(true);
                  if (playPos == eventsEnd) {
                        if (mscore->loop()) {
                              qDebug("Seq.cpp - Process - Loop whole score. playPos = %d, cs->pos() = %d", playPos->first, cs->pos().ticks());
                              emit toGui('4');
                              return;
                              }
                        else {
                              emit toGui('2');
                              }
                        }
                  else {
                     emit toGui('0');
                     }
                  }
            else if (state != driverState)
                  qDebug("Seq: state transition %d -> %d ?",
                     (int)state, (int)driverState);
            }

      memset(buffer, 0, sizeof(float) * framesPerPeriod * 2); // assume two channels
      float* p = buffer;

      processMessages();

      updateIndependentMetronomeFollowState();

      const bool independentFollowPlaybackActive =
            independentMetronomeFollowPlaybackActiveRT;

      if (state == Transport::PLAY) {
            if (!cs)
                  return;

            // if currently in count-in, these pointers will reference data in the count-in
            EventMap::const_iterator* pPlayPos   = &playPos;
            EventMap::const_iterator  pEventsEnd = eventsEnd;
            int*                      pPlayFrame = &playFrame;
            if (inCountIn) {
                  if (countInEvents.size() == 0)
                        addCountInClicks();
                  pEventsEnd = countInEvents.cend();
                  pPlayPos   = &countInPlayPos;
                  pPlayFrame = &countInPlayFrame;
                  }

            //
            // play events for one segment
            //
            unsigned framePos = 0; // frame currently being processed relative to the first frame of this call to Seq::process
            int periodEndFrame = *pPlayFrame + framesPerPeriod; // the ending frame (relative to start of playback) of the period being processed by this call to Seq::process
            int scoreEndUTick = cs->repeatList().tick2utick(cs->lastMeasure()->endTick().ticks());
            while (*pPlayPos != pEventsEnd) {
                  int playPosUTick = (*pPlayPos)->first;
                  int n; // current frame (relative to start of playback) that is being synthesized

                  if (inCountIn) {
                        qreal beatsPerSecond = curTempo() * cs->tempomap()->relTempo(); // relTempo needed here to ensure that bps changes as we slide the tempo bar
                        qreal ticksPerSecond = beatsPerSecond * DIVISION;
                        qreal playPosSeconds = playPosUTick / ticksPerSecond;
                        int playPosFrame = playPosSeconds * MScore::sampleRate;
                        if (playPosFrame >= periodEndFrame)
                              break;
                        n = playPosFrame - *pPlayFrame;
                        if (n < 0) {
                              qDebug("Count-in: playPosUTick %d: n = %d - %d", playPosUTick, playPosFrame, *pPlayFrame);
                              n = 0;
                              }
                        }
                  else {
                        qreal playPosSeconds = cs->utick2utime(playPosUTick);
                        int playPosFrame = playPosSeconds * MScore::sampleRate;
                        if (playPosFrame >= periodEndFrame)
                              break;
                        n = playPosFrame - *pPlayFrame;
                        if (n < 0) {
                              qDebug("%d:  %d - %d", playPosUTick, playPosFrame, *pPlayFrame);
                              n = 0;
                              }
                        if (mscore->loop()) {
                              int loopOutUTick = cs->repeatList().tick2utick(cs->loopOutTick().ticks());
                              if (loopOutUTick < scoreEndUTick) {
                                    qreal framesPerPeriodInTime = static_cast<qreal>(framesPerPeriod) / MScore::sampleRate;
                                    int framesPerPeriodInTicks = cs->utime2utick(framesPerPeriodInTime);
                                    // Also make sure we are inside the loop
                                    if (playPosUTick >= loopOutUTick - 2 * framesPerPeriodInTicks || cs->repeatList().utick2tick(playPosUTick) < cs->loopInTick().ticks()) {
                                          qDebug ("Process: playPosUTick = %d, cs->loopInTick().ticks() = %d, cs->loopOutTick().ticks() = %d, getCurTick() = %d, loopOutUTick = %d, playFrame = %d",
                                                            playPosUTick,      cs->loopInTick().ticks(),      cs->loopOutTick().ticks(),      getCurTick(),      loopOutUTick,    *pPlayFrame);
                                          if (cachedPrefs.useJackTransport) {
                                                int loopInUTick = cs->repeatList().tick2utick(cs->loopInTick().ticks());
                                                _driver->seekTransport(loopInUTick);
                                                if (loopInUTick != 0) {
                                                      int seekto = loopInUTick - 2 * cs->utime2utick((qreal)_driver->bufferSize() / MScore::sampleRate);
                                                      seekRT((seekto > 0) ? seekto : 0 );
                                                      }
                                                }
                                          else {
                                                emit toGui('3'); // calls loopStart()
                                                }
                                          // Exit this function to avoid segmentation fault in Scoreview
                                          return;
                                          }
                                    }
                              }
                        }
                  if (n) {
                        if (cs->playMode() == PlayMode::SYNTHESIZER) {
                              metronome(n, p, inCountIn);
                              _synti->process(n, p);

                              // Mix after synthesizer processing so the
                              // independent click bypasses Master Volume
                              independentMetronome(n, p);

                              p += n * 2;
                              *pPlayFrame  += n;
                              framesRemain -= n;
                              framePos     += n;
                              }
                        else {
                              while (n > 0) {
                                    int section;
                                    float** pcm;
                                    long rn = ov_read_float(&vf, &pcm, n, &section);
                                    if (rn == 0)
                                          break;

                                    float* audioStart = p;

                                    for (int i = 0; i < rn; ++i) {
                                          *p++ = pcm[0][i];
                                          *p++ = pcm[1][i];
                                          }

                                    independentMetronome(rn, audioStart);

                                    *pPlayFrame  += rn;
                                    framesRemain -= rn;
                                    framePos     += rn;
                                    n            -= rn;
                                    }
                              }
                        }
                  const NPlayEvent& event = (*pPlayPos)->second;
                  playEvent(event, framePos);
                  if (event.type() == ME_TICK1) {
                        const qreal volume =
                              event.velo()
                              ? qreal(event.value()) / 127.0
                              : 1.0;

                        startMetronomeTick(volume, false);

                        if (independentFollowPlaybackActive)
                              startMetronomeTick(
                                    independentMetronomeEventVolume(event),
                                    true);
                        }
                  else if (event.type() == ME_TICK2) {
                        const qreal volume =
                              event.velo()
                              ? qreal(event.value()) / 127.0
                              : 1.0;

                        startMetronomeTack(volume, false);

                        if (independentFollowPlaybackActive)
                              startMetronomeTack(
                                    independentMetronomeEventVolume(event),
                                    true);
                        }
                  mutex.lock();
                  ++(*pPlayPos);
                  mutex.unlock();
                  }
            if (framesRemain) {
                  if (cs->playMode() == PlayMode::SYNTHESIZER) {
                        metronome(framesRemain, p, inCountIn);
                        _synti->process(framesRemain, p);

                        independentMetronome(framesRemain, p);

                        *pPlayFrame += framesRemain;
                        }
                  else {
                        int n = framesRemain;
                        while (n > 0) {
                              int section;
                              float** pcm;
                              long rn = ov_read_float(&vf, &pcm, n, &section);
                              if (rn == 0)
                                    break;

                              float* audioStart = p;

                              for (int i = 0; i < rn; ++i) {
                                    *p++ = pcm[0][i];
                                    *p++ = pcm[1][i];
                                    }

                              independentMetronome(rn, audioStart);

                              *pPlayFrame  += rn;
                              framesRemain -= rn;
                              framePos     += rn;
                              n            -= rn;
                              }
                        }
                  }
            if (*pPlayPos == pEventsEnd) {
                  if (inCountIn) {
                        inCountIn = false;
                        // Connecting to JACK Transport if MuseScore was temporarily disconnected from it
                        if (useJackTransportSavedFlag) {
                              // Stopping fake driver
                              stopTransport();
                              preferences.setPreference(PREF_IO_JACK_USEJACKTRANSPORT, true);
                              // Starting the real JACK Transport. All applications play in sync now
                              startTransport();
                              }
                        }
                  else
                        stopTransport();
                  }
            }
      else {
            // Outside of playback mode
            while (!liveEventQueue()->empty()) {
                  const NPlayEvent& event = liveEventQueue()->dequeue();
                  const qreal volume =
                        event.velo()
                        ? qreal(event.value()) / 127.0
                        : 1.0;

                  if (event.type() == ME_TICK1)
                        startMetronomeTick(volume, false);
                  else if (event.type() == ME_TICK2)
                        startMetronomeTack(volume, false);
                  }
            if (framesRemain) {
                  metronome(framesRemain, p, true);
                  _synti->process(framesRemain, p);

                  independentMetronome(framesRemain, p);
                  }
            }

      //
      // metering / master gain
      //
      qreal lv = 0.0f;
      qreal rv = 0.0f;
      p = buffer;
      for (unsigned i = 0; i < framesRemain; ++i) {
            qreal val = *p;
            lv = qMax(lv, qAbs(val));
            p++;

            val = *p;
            rv = qMax(rv, qAbs(val));
            p++;
            }
      meterValue[0] = lv;
      meterValue[1] = rv;
      if (meterPeakValue[0] < lv) {
            meterPeakValue[0] = lv;
            peakTimer[0] = 0;
            }
      if (meterPeakValue[1] < rv) {
            meterPeakValue[1] = rv;
            peakTimer[1] = 0;
            }
      }

//---------------------------------------------------------
//   initInstruments
//---------------------------------------------------------

void Seq::initInstruments(bool realTime)
      {
      // Add midi out ports if necessary
      if (cs && (cachedPrefs.useJackMidi || cachedPrefs.useAlsaAudio)) {
            // Increase the maximum number of midi ports if user adds staves/instruments
            int scoreMaxMidiPort = cs->masterScore()->midiPortCount();
            if (maxMidiOutPort < scoreMaxMidiPort)
                  maxMidiOutPort = scoreMaxMidiPort;
            // if maxMidiOutPort is equal to existing ports number, it will do nothing
            if (_driver)
                  _driver->updateOutPortCount(maxMidiOutPort + 1);
            }

      for (const MidiMapping& mm : cs->midiMapping()) {
            const Channel* channel = mm.articulation();
            for (const MidiCoreEvent& e : channel->initList()) {
                  if (e.type() == ME_INVALID)
                        continue;
                  NPlayEvent event(e.type(), channel->channel(), e.dataA(), e.dataB());
                  if (realTime)
                        putEvent(event);
                  else
                        sendEvent(event);
                  }
            // Setting pitch bend sensitivity to 12 semitones for external synthesizers
            if ((cachedPrefs.useJackMidi || cachedPrefs.useAlsaAudio) && mm.channel() != 9) {
                  if (realTime) {
                        putEvent(NPlayEvent(ME_CONTROLLER, channel->channel(), CTRL_LRPN, 0));
                        putEvent(NPlayEvent(ME_CONTROLLER, channel->channel(), CTRL_HRPN, 0));
                        putEvent(NPlayEvent(ME_CONTROLLER, channel->channel(), CTRL_HDATA,12));
                        putEvent(NPlayEvent(ME_CONTROLLER, channel->channel(), CTRL_LRPN, 127));
                        putEvent(NPlayEvent(ME_CONTROLLER, channel->channel(), CTRL_HRPN, 127));
                        }
                  else {
                        sendEvent(NPlayEvent(ME_CONTROLLER, channel->channel(), CTRL_LRPN, 0));
                        sendEvent(NPlayEvent(ME_CONTROLLER, channel->channel(), CTRL_HRPN, 0));
                        sendEvent(NPlayEvent(ME_CONTROLLER, channel->channel(), CTRL_HDATA,12));
                        sendEvent(NPlayEvent(ME_CONTROLLER, channel->channel(), CTRL_LRPN, 127));
                        sendEvent(NPlayEvent(ME_CONTROLLER, channel->channel(), CTRL_HRPN, 127));
                        }
                  }
            }
      }

//---------------------------------------------------------
//   renderChunk
//---------------------------------------------------------

void Seq::renderChunk(const MidiRenderer::Chunk& ch, EventMap* eventMap)
      {
      SynthesizerState synState = mscore->synthesizerState();
      MidiRenderer::Context ctx(synState);
      ctx.metronome = true;
      ctx.renderHarmony = true;
      midi.renderChunk(ch, eventMap, ctx);
      renderEventsStatus.setOccupied(ch.utick1(), ch.utick2());
      }

//---------------------------------------------------------
//   updateEventsEnd
//---------------------------------------------------------

void Seq::updateEventsEnd()
      {
      auto end = events.cend();
      eventsEnd = end;
      endUTick = events.empty() ? 0 : (--end)->first;
      }

//---------------------------------------------------------
//   collectEvents
//---------------------------------------------------------

void Seq::collectEvents(int utick)
      {
      //do not collect even while playing
      if (state == Transport::PLAY && playlistChanged)
            return;

      mutex.lock();

      if (midiRenderFuture.isRunning())
            midiRenderFuture.waitForFinished();

      if (playlistChanged) {
            midi.setScoreChanged();
            events.clear();
            renderEvents.clear();
            renderEventsStatus.clear();
            }
      else if (!renderEvents.empty()) {
            events.insert(renderEvents.begin(), renderEvents.end());
            renderEvents.clear();
            }

      int unrenderedUtick = renderEventsStatus.occupiedRangeEnd(utick);
      while (unrenderedUtick - utick < minUtickBufferSize) {
            const MidiRenderer::Chunk chunk = midi.getChunkAt(unrenderedUtick);
            if (!chunk)
                  break;
            renderChunk(chunk, &events);
            unrenderedUtick = renderEventsStatus.occupiedRangeEnd(utick);
            }

      updateEventsEnd();
      playPos = mscore->loop() ? events.find(cs->loopInTick().ticks()) : events.cbegin();
      playlistChanged = false;
      mutex.unlock();
      }

//---------------------------------------------------------
//   ensureBufferAsync
//---------------------------------------------------------

void Seq::ensureBufferAsync(int utick)
      {
      if (mutex.tryLock()) { // sync with possible collectEvents calls

            if (midiRenderFuture.isRunning() || !allowBackgroundRendering) {
                  mutex.unlock();
                  return;
                  }

            if (!renderEvents.empty()) {
                  // TODO: use C++17 map::merge()?
                  events.insert(renderEvents.begin(), renderEvents.end());
                  updateEventsEnd();
                  renderEvents.clear();
                  }

            const int unrenderedUtick = renderEventsStatus.occupiedRangeEnd(utick);
            if (unrenderedUtick - utick < minUtickBufferSize) {
                  const MidiRenderer::Chunk chunk = midi.getChunkAt(unrenderedUtick);
                  if (chunk) {
                        midiRenderFuture = QtConcurrent::run([this, chunk]() {
                              renderChunk(chunk, &renderEvents);
                              });
                        }
                  }
            mutex.unlock();
            }
      }

//---------------------------------------------------------
//   getCurTick
//---------------------------------------------------------

int Seq::getCurTick()
      {
      return cs->utime2utick(qreal(playFrame) / qreal(MScore::sampleRate));
      }

//---------------------------------------------------------
//   setRelTempo
//    relTempo = 1.0 = normal tempo
//---------------------------------------------------------

void Seq::setRelTempo(double relTempo)
      {
      guiToSeq(SeqMsg(SeqMsgId::TEMPO_CHANGE, relTempo));
      }

//---------------------------------------------------------
//   setPos
//    seek
//    realtime environment
//---------------------------------------------------------

void Seq::setPos(int utick)
      {
      if (cs == 0)
            return;
      stopNotes(-1, true);

      int ucur;
      mutex.lock();
      if (playPos != events.end())
            ucur = cs->repeatList().utick2tick(playPos->first);
      else
            ucur = utick - 1;
      if (utick != ucur)
            updateSynthesizerState(ucur, utick);

      playFrame = cs->utick2utime(utick) * MScore::sampleRate;
      playPos   = events.lower_bound(utick);
      mutex.unlock();
      }

//---------------------------------------------------------
//   getPlayStartUtick
//---------------------------------------------------------

int Seq::getPlayStartUtick()
      {
      if ((mscore->loop())) {
            if (preferences.getBool(PREF_APP_PLAYBACK_LOOPTOSELECTIONONPLAY)) {
                  setLoopSelection();
                  }
            return cs->repeatList().tick2utick(cs->loopInTick().ticks());
            }
      return cs->repeatList().tick2utick(cs->playPos().ticks());
      }

//---------------------------------------------------------
//   seekCommon
//   a common part of seek() and seekRT(), contains code
//   that could be safely called from any thread.
//   Do not use explicitly, use seek() or seekRT()
//---------------------------------------------------------

void Seq::seekCommon(int utick)
      {
      if (cs == 0)
            return;

      collectEvents(utick);

      if (cs->playMode() == PlayMode::AUDIO) {
            ogg_int64_t sp = cs->utick2utime(utick) * MScore::sampleRate;
            ov_pcm_seek(&vf, sp);
            }

      guiPos = events.lower_bound(utick);
      mscore->setPos(Fraction::fromTicks(cs->repeatList().utick2tick(utick)));
      unmarkNotes();
      }

//---------------------------------------------------------
//   seek
//   send seek message to sequencer
//   gui thread
//---------------------------------------------------------

void Seq::seek(int utick)
      {
      if (preferences.getBool(PREF_IO_JACK_USEJACKTRANSPORT)) {
            if (utick > endUTick)
                  utick = 0;
            _driver->seekTransport(utick);
            if (utick != 0)
                  return;
            }
      seekCommon(utick);

      int t = cs->repeatList().utick2tick(utick);
      Segment* seg = cs->tick2segment(Fraction::fromTicks(t));
      if (seg)
            mscore->currentScoreView()->moveCursor(seg->tick());
      cs->setPlayPos(Fraction::fromTicks(t));
      cs->update();
      guiToSeq(SeqMsg(SeqMsgId::SEEK, utick));
      }

//---------------------------------------------------------
//   seekRT
//   realtime thread
//---------------------------------------------------------

void Seq::seekRT(int utick)
      {
      if (cachedPrefs.useJackTransport && utick > endUTick)
                  utick = 0;
      seekCommon(utick);
      setPos(utick);
      // Update the screen in GUI thread
      emit toGui('5', cs->repeatList().utick2tick(utick));
      }

//---------------------------------------------------------
//   startNote
//---------------------------------------------------------

void Seq::startNote(int channel, int pitch, int velo, double nt)
      {
      if (state != Transport::STOP && state != Transport::PLAY)
            return;
      NPlayEvent ev(ME_NOTEON, channel, pitch, velo);
      ev.setTuning(nt);
      sendEvent(ev);
      }

void Seq::startNote(int channel, int pitch, int velo, int duration, double nt)
      {
      stopNotes();
      startNote(channel, pitch, velo, nt);
      startNoteTimer(duration);
      }

//---------------------------------------------------------
//   playMetronomeBeat
//---------------------------------------------------------

void Seq::playMetronomeBeat(BeatType type)
      {
      if (state != Transport::STOP)
            return;
      liveEventQueue()->enqueue(NPlayEvent(type));
      }

//---------------------------------------------------------
//   loadMetronomeSample
//---------------------------------------------------------

const Seq::MetronomeCustomSample* Seq::loadMetronomeSample(const QString& path)
      {
#ifdef HAS_AUDIOFILE
      if (path.isEmpty())
            return nullptr;

      QFile file(path);
      if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "Cannot open custom metronome sound:" << path;
            return nullptr;
            }

      const QByteArray data = file.readAll();
      if (data.isEmpty()) {
            qWarning() << "Custom metronome sound is empty:" << path;
            return nullptr;
            }

      AudioFile audio;
      if (!audio.open(data)) {
            qWarning() << "Cannot decode custom metronome sound:" << path;
            return nullptr;
            }

      const int channels = audio.channels();
      const int sourceRate = audio.samplerate();
      const sf_count_t sourceFrames = audio.frames();

      if ((channels != 1 && channels != 2)
          || sourceRate <= 0
          || sourceFrames <= 0) {
            qWarning() << "Unsupported custom metronome sound:" << path
                       << "channels:" << channels
                       << "sample rate:" << sourceRate
                       << "frames:" << sourceFrames;
            return nullptr;
            }

      std::vector<float> source;
      source.resize(size_t(sourceFrames) * size_t(channels));

      const sf_count_t framesRead =
            audio.readData(source.data(), sourceFrames);

      if (framesRead <= 0) {
            qWarning()
                  << "Cannot read custom metronome sound:"
                  << path;
            return nullptr;
            }

      float maxSignal = 0.0f;

      const size_t samplesRead =
            size_t(framesRead) * size_t(channels);

      for (size_t i = 0; i < samplesRead; ++i)
            maxSignal = qMax(maxSignal, qAbs(source[i]));

      // Some libsndfile versions can return OGG float samples outside
      // the normal [-1, 1] range, so we'll correct that but not normalize
      // any custom tick/tack sounds
      const float sourceGain =
            maxSignal > 1.0f
            ? 1.0f / maxSignal
            : 1.0f;

      const double rateRatio =
            double(MScore::sampleRate) / double(sourceRate);

      const unsigned targetFrames =
            unsigned(qMax<qint64>(
                  1,
                  qRound64(double(framesRead) * rateRatio)));

      std::unique_ptr<MetronomeCustomSample> sample(
            new MetronomeCustomSample);

      sample->data.resize(size_t(targetFrames) * 2);

      for (unsigned frame = 0; frame < targetFrames; ++frame) {
            const double sourcePos =
                  double(frame) * double(sourceRate)
                  / double(MScore::sampleRate);

            const sf_count_t frame0 =
                  qMin<sf_count_t>(sf_count_t(sourcePos),
                                   framesRead - 1);

            const sf_count_t frame1 =
                  qMin<sf_count_t>(frame0 + 1,
                                   framesRead - 1);

            const float fraction =
                  float(sourcePos - double(frame0));

            for (int outputChannel = 0; outputChannel < 2;
                 ++outputChannel) {
                  // Mono is duplicated to left and right
                  // Stereo retains its original channels
                  const int sourceChannel =
                        channels == 1 ? 0 : outputChannel;

                  const float sample0 =
                        source[size_t(frame0) * channels + sourceChannel]
                        * sourceGain;

                  const float sample1 =
                        source[size_t(frame1) * channels + sourceChannel]
                        * sourceGain;

                  sample->data[size_t(frame) * 2 + outputChannel] =
                        sample0 + (sample1 - sample0) * fraction;
                  }
            }

      const MetronomeCustomSample* result = sample.get();
      _metronomeSampleStorage.emplace_back(std::move(sample));

      return result;
#else
      Q_UNUSED(path);
      return nullptr;
#endif
      }

//---------------------------------------------------------
//   reloadMetronomeSamples
//---------------------------------------------------------

void Seq::reloadMetronomeSamples(bool force)
      {
#ifdef HAS_AUDIOFILE
      const QString tickPath =
            preferences.getString(
                  PREF_APP_PLAYBACK_METRONOME_DOWNBEAT_SOUND);

      const QString tackPath =
            preferences.getString(
                  PREF_APP_PLAYBACK_METRONOME_BEAT_SOUND);

      const bool sampleRateChanged =
            _loadedMetronomeSampleRate != MScore::sampleRate;

      const bool tickNeedsRetry =
            !tickPath.isEmpty()
            && !_customMetronomeTickSample.load();

      const bool tackNeedsRetry =
            !tackPath.isEmpty()
            && !_customMetronomeTackSample.load();

      if (force || sampleRateChanged
          || tickPath != _loadedMetronomeTickPath
          || tickNeedsRetry) {
            const MetronomeCustomSample* sample =
                  loadMetronomeSample(tickPath);

            _customMetronomeTickSample.store(sample);

            _loadedMetronomeTickPath = tickPath;
            }

      if (force || sampleRateChanged
          || tackPath != _loadedMetronomeTackPath
          || tackNeedsRetry) {
            const MetronomeCustomSample* sample =
                  loadMetronomeSample(tackPath);

            _customMetronomeTackSample.store(sample);

            _loadedMetronomeTackPath = tackPath;
            }

      _loadedMetronomeSampleRate = MScore::sampleRate;

      reclaimRetiredMetronomeSamples();
#else
      Q_UNUSED(force);
#endif
      }

//---------------------------------------------------------
//   reclaimRetiredMetronomeSamples
//---------------------------------------------------------

void Seq::reclaimRetiredMetronomeSamples()
      {
      const MetronomeCustomSample* currentTick =
            _customMetronomeTickSample.load();

      const MetronomeCustomSample* currentTack =
            _customMetronomeTackSample.load();

      const MetronomeCustomSample* tickInUse =
            _tickCustomSampleInUse.load();

      const MetronomeCustomSample* tackInUse =
            _tackCustomSampleInUse.load();

      const MetronomeCustomSample* independentTickInUse =
            _independentTickCustomSampleInUse.load();

      const MetronomeCustomSample* independentTackInUse =
            _independentTackCustomSampleInUse.load();

      auto i = _metronomeSampleStorage.begin();

      while (i != _metronomeSampleStorage.end()) {
            const MetronomeCustomSample* sample = i->get();

            const bool needed =
                  sample == currentTick
                  || sample == currentTack
                  || sample == tickInUse
                  || sample == tackInUse
                  || sample == independentTickInUse
                  || sample == independentTackInUse;

            if (needed)
                  ++i;
            else
                  i = _metronomeSampleStorage.erase(i);
            }
      }

//---------------------------------------------------------
//   independentMetronomeEventVolume
//---------------------------------------------------------

qreal Seq::independentMetronomeEventVolume(const NPlayEvent& event) const
      {
      if (!independentMetronomeBeatAccentsRT
          && event.type() == ME_TICK2) {
            // With beat accents disabled, every non-downbeat
            // has the normal unstressed tack volume
            return 80.0 / 127.0;
            }

      return event.velo()
            ? qreal(event.value()) / 127.0
            : 1.0;
      }

//---------------------------------------------------------
//   acquireMetronomeSample
//---------------------------------------------------------

const Seq::MetronomeCustomSample* Seq::acquireMetronomeSample(
      const std::atomic<const MetronomeCustomSample*>& published,
      std::atomic<const MetronomeCustomSample*>& inUse)
      {
      const MetronomeCustomSample* sample;

      do {
            sample = published.load();

            // Publish the sample as being in use before validating
            // that it is still the currently selected sample
            inUse.store(sample);
            }
      while (sample != published.load());

      return sample;
      }

//---------------------------------------------------------
//   startMetronomeTick
//---------------------------------------------------------

void Seq::startMetronomeTick(qreal volume, bool independent)
      {
      const MetronomeCustomSample* sample;

      if (independent) {
            sample = acquireMetronomeSample(
                  _customMetronomeTickSample,
                  _independentTickCustomSampleInUse);
            }
      else {
            sample = acquireMetronomeSample(
                  _customMetronomeTickSample,
                  _tickCustomSampleInUse);
            }

      if (independent) {
            independentTackRemain = 0;
            _independentTackCustomSampleInUse.store(nullptr);
            }
      else {
            tackRemain = 0;
            _tackCustomSampleInUse.store(nullptr);
            }

      const uint frames = sample
            ? sample->frames()
            : tickLength;

      if (independent) {
            independentTickRemain = frames;
            independentTickVolume = volume;
            }
      else {
            tickRemain = frames;
            tickVolume = volume;
            }
      }

//---------------------------------------------------------
//   startMetronomeTack
//---------------------------------------------------------

void Seq::startMetronomeTack(qreal volume, bool independent)
      {
      const MetronomeCustomSample* sample;

      if (independent) {
            sample = acquireMetronomeSample(
                  _customMetronomeTackSample,
                  _independentTackCustomSampleInUse);
            }
      else {
            sample = acquireMetronomeSample(
                  _customMetronomeTackSample,
                  _tackCustomSampleInUse);
            }

      const uint frames = sample
            ? sample->frames()
            : tackLength;

      if (independent) {
            independentTackRemain = frames;
            independentTackVolume = volume;
            }
      else {
            tackRemain = frames;
            tackVolume = volume;
            }
      }

//---------------------------------------------------------
//   mixMetronomeSample
//---------------------------------------------------------

void Seq::mixMetronomeSample(unsigned n, float* p,
                             uint& remain, qreal volume,
                             const MetronomeCustomSample* customSample,
                             const double* defaultSample,
                             unsigned defaultFrames)
      {
      if (!remain)
            return;

      const unsigned sampleFrames =
            customSample ? customSample->frames() : defaultFrames;

      if (remain > sampleFrames) {
            remain = 0;
            return;
            }

      const unsigned idx = sampleFrames - remain;
      const unsigned nn = qMin(n, remain);
      const qreal gain = volume * metronomeVolume;

      if (customSample) {
            const float* source =
                  customSample->data.data() + size_t(idx) * 2;

            for (unsigned i = 0; i < nn; ++i) {
                  *p++ += *source++ * gain;
                  *p++ += *source++ * gain;
                  }
            }
      else {
            unsigned sourceIdx = idx;

            for (unsigned i = 0; i < nn; ++i) {
                  const qreal value =
                        defaultSample[sourceIdx++] * gain;

                  *p++ += value;
                  *p++ += value;
                  }
            }

      remain -= nn;
      }

//---------------------------------------------------------
//   independentMetronomeClickTicks
//---------------------------------------------------------

int Seq::independentMetronomeClickTicks() const
      {
      const TimeSigFrac timeSig(independentMetronomeNumeratorRT,
                                independentMetronomeDenominatorRT);

      // MuseScore stores tempo in quarter-notes per second:
      const qreal tempo = independentMetronomeBpmRT / 60.0;

      return timeSig.isBeatedCompound(tempo)
            ? timeSig.beatTicks()
            : timeSig.dUnitTicks();
      }

//---------------------------------------------------------
//   independentMetronomeFramesPerClick
//---------------------------------------------------------

double Seq::independentMetronomeFramesPerClick() const
      {
      const qreal tempo = independentMetronomeBpmRT / 60.0;

      const int clickTicks = independentMetronomeClickTicks();
      const qreal ticksPerSecond = tempo * DIVISION;

      if (ticksPerSecond <= 0.0)
            return 1.0;

      return qMax(
            1.0,
            double(MScore::sampleRate) * clickTicks / ticksPerSecond);
      }

//---------------------------------------------------------
//   setIndependentMetronomeEnabled
//---------------------------------------------------------

void Seq::setIndependentMetronomeEnabled(bool enabled)
      {
      if (independentMetronomeEnabledValue == enabled)
            return;

      independentMetronomeEnabledValue = enabled;

      guiToSeq(SeqMsg(SeqMsgId::INDEPENDENT_METRONOME_ENABLED,
                      enabled ? 1 : 0));
      }

//---------------------------------------------------------
//   independentMetronomeEnabled
//---------------------------------------------------------

bool Seq::independentMetronomeEnabled() const
      {
      return independentMetronomeEnabledValue;
      }

//---------------------------------------------------------
//   setIndependentMetronomeBpm
//---------------------------------------------------------

void Seq::setIndependentMetronomeBpm(double bpm)
      {
      if (bpm < 20.0 || bpm > 400.0)
            return;

      if (qFuzzyCompare(1.0 + independentMetronomeBpmValue,
                        1.0 + bpm)) {
            return;
            }

      independentMetronomeBpmValue = bpm;

      preferences.setPreference(
            PREF_APP_PLAYBACK_INDEPENDENT_METRONOME_BPM,
            bpm);

      guiToSeq(SeqMsg(SeqMsgId::INDEPENDENT_METRONOME_BPM,
                      qreal(bpm)));
      }

//---------------------------------------------------------
//   setIndependentMetronomeTimeSignature
//---------------------------------------------------------

void Seq::setIndependentMetronomeTimeSignature(int numerator, int denominator)
      {
      if (numerator < 1 || numerator > 32 || denominator <= 0)
            return;

      // Restrict denominator to powers of two which can be
      // represented exactly by MuseScore's tick resolution
      if ((denominator & (denominator - 1)) != 0
          || (DIVISION * 4) % denominator != 0)
            return;

      if (independentMetronomeNumeratorValue == numerator
          && independentMetronomeDenominatorValue == denominator) {
            return;
            }

      independentMetronomeNumeratorValue = numerator;
      independentMetronomeDenominatorValue = denominator;

      preferences.setPreference(
            PREF_APP_PLAYBACK_INDEPENDENT_METRONOME_NUMERATOR,
            numerator);

      preferences.setPreference(
            PREF_APP_PLAYBACK_INDEPENDENT_METRONOME_DENOMINATOR,
            denominator);

      // Begin the new meter from its downbeat
      guiToSeq(SeqMsg(SeqMsgId::INDEPENDENT_METRONOME_TIME_SIGNATURE,
                      numerator,
                      denominator));
      }

//---------------------------------------------------------
//   setIndependentMetronomeFollowPlayback
//---------------------------------------------------------

void Seq::setIndependentMetronomeFollowPlayback(bool follow)
      {
      if (independentMetronomeFollowPlaybackValue == follow)
            return;

      independentMetronomeFollowPlaybackValue = follow;

      preferences.setPreference(
            PREF_APP_PLAYBACK_INDEPENDENT_METRONOME_FOLLOW,
            follow);

      guiToSeq(SeqMsg(SeqMsgId::INDEPENDENT_METRONOME_FOLLOW,
                      follow ? 1 : 0));
      }

//---------------------------------------------------------
//   setIndependentMetronomeBeatAccents
//---------------------------------------------------------

void Seq::setIndependentMetronomeBeatAccents(bool enabled)
      {
      if (independentMetronomeBeatAccentsValue == enabled)
            return;

      independentMetronomeBeatAccentsValue = enabled;

      preferences.setPreference(
            PREF_APP_PLAYBACK_INDEPENDENT_METRONOME_ACCENTS,
            enabled);

      guiToSeq(SeqMsg(SeqMsgId::INDEPENDENT_METRONOME_ACCENTS,
                      enabled ? 1 : 0));
      }

//---------------------------------------------------------
//   startNoteTimer
//---------------------------------------------------------

void Seq::startNoteTimer(int duration)
      {
      if (duration) {
            noteTimer->setInterval(duration);
            noteTimer->start();
            }
      }

//---------------------------------------------------------
//   stopNoteTimer
//---------------------------------------------------------

void Seq::stopNoteTimer()
      {
      if (noteTimer->isActive()) {
            noteTimer->stop();
            stopNotes();
            }
      }

//---------------------------------------------------------
//   stopNotes
//---------------------------------------------------------

void Seq::stopNotes(int channel, bool realTime)
      {
      auto send = [this, realTime](const NPlayEvent& event) {
            if (realTime)
                  putEvent(event);
            else
                  sendEvent(event);
            };
      // For VSTs/devices that do not support All Notes Off
      // CTRL_ALL_NOTES_OFF should still be evoked after calling this function, even if it seems redundant
      auto turnAllNotesOff = [send](int channel) {
            for (unsigned note = 0; note < 128; note++)
                  send(NPlayEvent(ME_NOTEOFF, channel, note, 0));
            };
      // Stop notes in all channels
      if (channel == -1) {
            for(unsigned ch = 0; ch < cs->midiMapping().size(); ch++) {
                  send(NPlayEvent(ME_CONTROLLER, ch, CTRL_SUSTAIN, 0));
                  turnAllNotesOff(ch);
                  send(NPlayEvent(ME_CONTROLLER, ch, CTRL_ALL_NOTES_OFF, 0));
                  if (cs->midiChannel(ch) != 9)
                        send(NPlayEvent(ME_PITCHBEND,  ch, 0, 64));
                  }
            }
      else {
            send(NPlayEvent(ME_CONTROLLER, channel, CTRL_SUSTAIN, 0));
            turnAllNotesOff(channel);
            send(NPlayEvent(ME_CONTROLLER, channel, CTRL_ALL_NOTES_OFF, 0));
            if (cs->midiChannel(channel) != 9)
                  send(NPlayEvent(ME_PITCHBEND,  channel, 0, 64));
            }
      if (cachedPrefs.useAlsaAudio || cachedPrefs.useJackAudio || cachedPrefs.usePulseAudio || cachedPrefs.usePortAudio) {
            guiToSeq(SeqMsg(SeqMsgId::ALL_NOTE_OFF, channel));
            }
      }

//---------------------------------------------------------
//   setController
//---------------------------------------------------------

void Seq::setController(int channel, int ctrl, int data)
      {
      NPlayEvent event(ME_CONTROLLER, channel, ctrl, data);
      sendEvent(event);
      }

//---------------------------------------------------------
//   sendEvent
//    called from GUI context to send a midi event to
//    midi out or synthesizer
//---------------------------------------------------------

void Seq::sendEvent(const NPlayEvent& ev)
      {
      guiToSeq(SeqMsg(SeqMsgId::PLAY, ev));
      }

//---------------------------------------------------------
//   nextMeasure
//---------------------------------------------------------

void Seq::nextMeasure()
      {
      Measure* m = cs->tick2measure(Fraction::fromTicks(guiPos->first));
      if (m) {
            if (m->nextMeasure())
                  m = m->nextMeasure();
            seek(m->tick().ticks());
            }
      }

//---------------------------------------------------------
//   nextChord
//---------------------------------------------------------

void Seq::nextChord()
      {
      int t = guiPos->first;
      for (auto i = guiPos; i != eventsEnd; ++i) {
            if (i->second.type() == ME_NOTEON && i->first > t && i->second.velo()) {
                  seek(i->first);
                  break;
                  }
            }
      }

//---------------------------------------------------------
//   prevMeasure
//---------------------------------------------------------

void Seq::prevMeasure()
      {
      auto i = guiPos;
      if (i == events.begin())
            return;
      --i;
      Measure* m = cs->tick2measure(Fraction::fromTicks(i->first));
      if (m) {
            if ((i->first == m->tick().ticks()) && m->prevMeasure())
                  m = m->prevMeasure();
            seek(m->tick().ticks());
            }
      }

//---------------------------------------------------------
//   prevChord
//---------------------------------------------------------

void Seq::prevChord()
      {
      int t  = playPos->first;
      //find the chord just before playpos
      EventMap::const_iterator i = events.upper_bound(cs->repeatList().tick2utick(t));
      for (;;) {
            if (i->second.type() == ME_NOTEON) {
                  const NPlayEvent& n = i->second;
                  if (i->first < t && n.velo()) {
                        t = i->first;
                        break;
                        }
                  }
            if (i == events.cbegin())
                  break;
            --i;
            }
      //go the previous chord
      if (i != events.cbegin()) {
            i = playPos;
            for (;;) {
                  if (i->second.type() == ME_NOTEON) {
                        const NPlayEvent& n = i->second;
                        if (i->first < t && n.velo()) {
                              seek(i->first);
                              break;
                              }
                        }
                  if (i == events.cbegin())
                        break;
                  --i;
                  }
            }
      }

//---------------------------------------------------------
//   seekEnd
//---------------------------------------------------------

void Seq::seekEnd()
      {
      qDebug("seek to end");
      }

//---------------------------------------------------------
//   guiToSeq
//---------------------------------------------------------

void Seq::guiToSeq(const SeqMsg& msg)
      {
      if (!_driver || !running)
            return;
      toSeq.enqueue(msg);
      }

//---------------------------------------------------------
//   eventToGui
//---------------------------------------------------------

void Seq::eventToGui(NPlayEvent e)
      {
      fromSeq.enqueue(SeqMsg(SeqMsgId::MIDI_INPUT_EVENT, e));
      }

//---------------------------------------------------------
//   midiInputReady
//---------------------------------------------------------

void Seq::midiInputReady()
      {
      if (_driver)
            _driver->midiRead();
      }

//---------------------------------------------------------
//   SeqMsgFifo
//---------------------------------------------------------

SeqMsgFifo::SeqMsgFifo()
      {
      maxCount = SEQ_MSG_FIFO_SIZE;
      clear();
      }

//---------------------------------------------------------
//   enqueue
//---------------------------------------------------------

void SeqMsgFifo::enqueue(const SeqMsg& msg)
      {
      int i = 0;
      int n = 50;

      QMutex mutex;
      QWaitCondition qwc;
      mutex.lock();
      for (; i < n; ++i) {
            if (!isFull())
                  break;
            qwc.wait(&mutex,100);
            }
      mutex.unlock();
      if (i == n) {
            qDebug("===SeqMsgFifo: overflow");
            return;
            }
      messages[widx] = msg;
      push();
      }

//---------------------------------------------------------
//   dequeue
//---------------------------------------------------------

SeqMsg SeqMsgFifo::dequeue()
      {
      SeqMsg msg = messages[ridx];
      pop();
      return msg;
      }

//---------------------------------------------------------
//   putEvent
//---------------------------------------------------------

void Seq::putEvent(const NPlayEvent& event, unsigned framePos)
      {
      if (!cs)
            return;
      int channel = event.channel();
      if (channel >= int(cs->midiMapping().size())) {
            qDebug("bad channel value %d >= %d", channel, int(cs->midiMapping().size()));
            return;
            }

      // audio
      int syntiIdx= _synti->index(cs->midiMapping(channel)->articulation()->synti());
      _synti->play(event, syntiIdx);

      // midi
      if (_driver != 0 && (cachedPrefs.useJackMidi || cachedPrefs.useAlsaAudio || cachedPrefs.usePortAudio))
            _driver->putEvent(event, framePos);
      }

//---------------------------------------------------------
//   heartBeat
//    update GUI
//---------------------------------------------------------

void Seq::heartBeatTimeout()
      {
      SynthControl* sc = mscore->getSynthControl();
      if (sc && _driver) {
            if (++peakTimer[0] >= peakHold)
                  meterPeakValue[0] *= .7f;
            if (++peakTimer[1] >= peakHold)
                  meterPeakValue[1] *= .7f;
            sc->setMeter(meterValue[0], meterValue[1], meterPeakValue[0], meterPeakValue[1]);
            }

      while (!fromSeq.empty()) {
            SeqMsg msg = fromSeq.dequeue();
            if (msg.id == SeqMsgId::MIDI_INPUT_EVENT) {
                  int type = msg.event.type();
                  if (type == ME_NOTEON)
                        mscore->midiNoteReceived(msg.event.channel(), msg.event.pitch(), msg.event.velo());
                  else if (type == ME_NOTEOFF)
                        mscore->midiNoteReceived(msg.event.channel(), msg.event.pitch(), 0);
                  else if (type == ME_CONTROLLER)
                        mscore->midiCtrlReceived(msg.event.controller(), msg.event.value());
                  }
            }

      if (state != Transport::PLAY || inCountIn)
            return;

      int endFrame = playFrame;

      mutex.lock();
      auto ppos = playPos;
      if (ppos != events.cbegin())
            --ppos;
      mutex.unlock();

      ensureBufferAsync(ppos->first);

      if (cs && cs->sigmap()->timesig(getCurTick()).nominal()!=prevTimeSig) {
            prevTimeSig = cs->sigmap()->timesig(getCurTick()).nominal();
            emit timeSigChanged();
            }
      if (cs && curTempo()!=prevTempo) {
            prevTempo = curTempo();
            emit tempoChanged();
            }

      QRectF r;
      for (;guiPos != eventsEnd; ++guiPos) {
            if (guiPos->first > ppos->first)
                  break;
            if (mscore->loop())
                  if (guiPos->first >= cs->repeatList().tick2utick(cs->loopOutTick().ticks()))
                        break;
            const NPlayEvent& n = guiPos->second;
            if (n.type() == ME_NOTEON) {
                  const Note* note1 = n.note();
                  if (n.velo()) {
                        while (note1) {
                              for (ScoreElement* se : note1->linkList()) {
                                    if (!se->isNote())
                                          continue;
                                    Note* currentNote = toNote(se);
                                    if (mscore->playbackHighlight())
                                          currentNote->setMark(true);
                                    markedNotes.append(currentNote);
                                    r |= currentNote->canvasBoundingRect();
                                    }
                              note1 = note1->tieFor() ? note1->tieFor()->endNote() : 0;
                              }
                        }
                  else {
                        while (note1) {
                              for (ScoreElement* se : note1->linkList()) {
                                    if (!se->isNote())
                                          continue;
                                    Note* currentNote = toNote(se);
                                    currentNote->setMark(false);
                                    r |= currentNote->canvasBoundingRect();
                                    markedNotes.removeOne(currentNote);
                                    }
                              note1 = note1->tieFor() ? note1->tieFor()->endNote() : 0;
                              }
                        }
                  }
            }
      int utick = ppos->first;
      int t = cs->repeatList().utick2tick(utick);
      mscore->currentScoreView()->moveCursor(Fraction::fromTicks(t));
      mscore->setPos(Fraction::fromTicks(t));

      emit(heartBeat(t, utick, endFrame));

      PianorollEditor* pre = mscore->getPianorollEditor();
      if (pre && pre->isVisible())
            pre->heartBeat(this);

      PianoTools* piano = mscore->pianoTools();
      if (piano && piano->isVisible())
            piano->updateAllKeys();

      cv->update(cv->toPhysical(r));
      }

//---------------------------------------------------------
//   updateSynthesizerState
//    collect all controller events between tick1 and tick2
//    and send them to the synthesizer
//    Called from RT thread
//---------------------------------------------------------

void Seq::updateSynthesizerState(int tick1, int tick2)
      {
      if (tick1 > tick2)
            tick1 = 0;
      // Making a local copy of events to avoid touching it
      // from different threads at the same time
      EventMap ev = events;
      EventMap::const_iterator i1 = ev.lower_bound(tick1);
      EventMap::const_iterator i2 = ev.upper_bound(tick2);

      for (; i1 != i2; ++i1) {
            if (i1->second.type() == ME_CONTROLLER)
                  playEvent(i1->second, 0);
            }
      }

//---------------------------------------------------------
//   curTempo
//---------------------------------------------------------

double Seq::curTempo() const
      {
      if (playPos != events.end())
            return cs ? cs->tempomap()->tempo(playPos->first) : 0.0;

      return 0.0;
      }

//---------------------------------------------------------
//   set Loop in position
//---------------------------------------------------------

void Seq::setLoopIn()
      {
      Fraction t;
      if (state == Transport::PLAY) {     // If in playback mode, set the In position where note is being played
            auto ppos = playPos;
            if (ppos != events.cbegin())
                  --ppos;                 // We have to go back one pos to get the correct note that has just been played
            t = Fraction::fromTicks(cs->repeatList().utick2tick(ppos->first));
            }
      else
            t = cs->pos();        // Otherwise, use the selected note.
      if (t >= cs->loopOutTick())         // If In pos >= Out pos, reset Out pos to end of score
            cs->setPos(POS::RIGHT, cs->lastMeasure()->endTick());
      cs->setPos(POS::LEFT, t);
      }

//---------------------------------------------------------
//   set Loop Out position
//---------------------------------------------------------

void Seq::setLoopOut()
      {
      Fraction t;
      if (state == Transport::PLAY) {    // If in playback mode, set the Out position where note is being played
            t = Fraction::fromTicks(cs->repeatList().utick2tick(playPos->first));
            }
      else
            t = cs->pos() + cs->inputState().ticks();   // Otherwise, use the selected note.
      if (t <= cs->loopInTick())                        // If Out pos <= In pos, reset In pos to beginning of score
            cs->setPos(POS::LEFT, Fraction(0,1));
      else
          if (t > cs->lastMeasure()->endTick())
              t = cs->lastMeasure()->endTick();
      cs->setPos(POS::RIGHT, t);
      if (state == Transport::PLAY)
            guiToSeq(SeqMsg(SeqMsgId::SEEK, t.ticks()));
      }

//---------------------------------------------------------
//   setMetronomeGain
//---------------------------------------------------------

void Seq::setMetronomeGain(float val)
      {
      val = qBound(0.0f, val, 10.0f);

      if (qFuzzyCompare(metronomeVolumeValue, qreal(val)))
            return;

      metronomeVolumeValue = val;

      preferences.setPreference(
            PREF_APP_PLAYBACK_METRONOME_VOLUME,
            double(val));

      guiToSeq(SeqMsg(SeqMsgId::METRONOME_GAIN,
                      qreal(val)));
      }

//---------------------------------------------------------
//   setPos
//---------------------------------------------------------

void Seq::setPos(POS, unsigned t)
      {
      qDebug("seq: setPos %d", t);
      }

//---------------------------------------------------------
//   set Loop In/Out position based on the selection
//---------------------------------------------------------

void Seq::setLoopSelection()
      {
      const Score* score = mscore->currentScore();
      Q_ASSERT(!score || score->masterScore() == cs);

      if (score && score->selection().isRange()) {
            cs->setLoopInTick(score->selection().tickStart());
            cs->setLoopOutTick(score->selection().tickEnd());
            }

      // add a dummy event to loop end if it is not already there
      // this is to let the playback reach the end completely before starting again
      if (!events.count(cs->loopOutTick().ticks())) {
            NPlayEvent ev;
            ev.setValue(ME_INVALID);
            events.insert(std::pair<int, Ms::NPlayEvent>(cs->loopOutTick().ticks(), ev));
            }
      }

//---------------------------------------------------------
//   Called after tempo or time signature
//   changed while playback
//---------------------------------------------------------

void Seq::handleTimeSigTempoChanged()
      {
      _driver->handleTimeSigTempoChanged();
      }

//---------------------------------------------------------
//  setInitialMillisecondTimestampWithLatency
//   Called whenever seq->process() starts.
//   Sets a starting reference time for which subsequent PortMidi events will be offset from.
//   Time is relative to the start of PortMidi's initialization.
//---------------------------------------------------------

void Seq::setInitialMillisecondTimestampWithLatency()
      {
     #ifdef USE_PORTMIDI
           initialMillisecondTimestampWithLatency = Pt_Time() + cachedPrefs.portMidiOutputLatencyMilliseconds;
           //qDebug("PortMidi initialMillisecondTimestampWithLatency: %d = %d + %d", initialMillisecondTimestampWithLatency, unsigned(Pt_Time()), preferences.getInt(PREF_IO_PORTMIDI_OUTPUTLATENCYMILLISECONDS));
     #endif
     }

//---------------------------------------------------------
//  getCurrentMillisecondTimestampWithLatency
//   Called when midi messages are sent to PortMidi device.
//   Returns the time in milliseconds of the current play cursor.
//   Time is relative to the start of PortMidi's initialization.
//---------------------------------------------------------

unsigned Seq::getCurrentMillisecondTimestampWithLatency(unsigned framePos) const
      {
#ifdef USE_PORTMIDI
      unsigned playTimeMilliseconds = unsigned(framePos * 1000) / unsigned(MScore::sampleRate);
      //qDebug("PortMidi timestamp = %d + %d", initialMillisecondTimestampWithLatency, playTimeMilliseconds);
      return initialMillisecondTimestampWithLatency + playTimeMilliseconds;
#else
      qDebug("Shouldn't be using this function if not using PortMidi");
      return 0;
#endif
      }
}
