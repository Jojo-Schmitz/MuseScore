//=============================================================================
//  MusE Score
//  Linux Music Score Editor
//
//  Copyright (C) 2002-2009 Werner Schweer and others
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

#ifndef __SEQ_H__
#define __SEQ_H__

#include "libmscore/rendermidi.h"
#include "libmscore/sequencer.h"
#include "libmscore/fraction.h"
#include "libmscore/fifo.h"

#include "audio/midi/event.h"
#include "audiodrivers/driver.h"

#include <atomic>
#include <memory>
#include <vector>

class QTimer;

namespace Ms {

class Note;
class MasterScore;
class Score;
class Painter;
class Measure;
class Fraction;
class Driver;
class Part;
class Channel;
class ScoreView;
class MasterSynthesizer;
class Segment;
enum class POS : char;

//---------------------------------------------------------
//   SeqMsg
//    message format for gui -> sequencer messages
//---------------------------------------------------------

enum class SeqMsgId : char {
      NO_MESSAGE,
      TEMPO_CHANGE,
      PLAY, SEEK,
      ALL_NOTE_OFF,
      MIDI_INPUT_EVENT,

      INDEPENDENT_METRONOME_ENABLED,
      INDEPENDENT_METRONOME_BPM,
      INDEPENDENT_METRONOME_TIME_SIGNATURE,
      INDEPENDENT_METRONOME_FOLLOW,
      INDEPENDENT_METRONOME_ACCENTS,
      METRONOME_GAIN
      };

struct SeqMsg {
      SeqMsgId id;
      union {
            int intVal;
            qreal realVal;
            };
      int intVal2 { 0 };
      NPlayEvent event;

      SeqMsg() {}
      SeqMsg(SeqMsgId _id, int val) : id(_id), intVal(val) {}
      SeqMsg(SeqMsgId _id, int val, int val2) : id(_id), intVal(val), intVal2(val2) {}
      SeqMsg(SeqMsgId _id, qreal val) : id(_id), realVal(val) {}
      SeqMsg(SeqMsgId _id, const NPlayEvent& e) : id(_id), event(e) {}
      };

//---------------------------------------------------------
//   SeqMsgFifo
//---------------------------------------------------------

static const int SEQ_MSG_FIFO_SIZE = 1024*8;

class SeqMsgFifo : public FifoBase {
      SeqMsg messages[SEQ_MSG_FIFO_SIZE];

   public:
      SeqMsgFifo();
      virtual ~SeqMsgFifo()     {}
      void enqueue(const SeqMsg&);        // put object on fifo
      SeqMsg dequeue();                   // remove object from fifo
      };

// this are also the jack audio transport states:
enum class Transport : char {
      STOP=0,
      PLAY=1,
      STARTING=3,
      NET_STARTING=4
      };

//---------------------------------------------------------
//   Seq
//    sequencer
//---------------------------------------------------------

class Seq : public QObject, public Sequencer {
      Q_OBJECT

      mutable QMutex mutex;

      MasterScore* cs;
      ScoreView* cv;
      bool running;                       // true if sequencer is available
      Transport state;                    // STOP, PLAY, STARTING=3
      bool inCountIn;
                                          // When we begin playing count in, JACK should play the ticks, but shouldn't run
                                          // JACK Transport to prevent playing in other applications. Before playing
                                          // count in we have to disconnect from JACK Transport by switching to the fake transport.
                                          // Also we save current preferences.useJackTransport value to useJackTransportSavedFlag
                                          // to restore it when count in ends. After this all applications start playing in sync.
      bool useJackTransportSavedFlag;
      int maxMidiOutPort;                 // Maximum count of midi out ports in all opened scores
      Fraction prevTimeSig;
      double prevTempo;

      bool oggInit;
      bool playlistChanged;

      SeqMsgFifo toSeq;
      SeqMsgFifo fromSeq;
      Driver* _driver;
      MasterSynthesizer* _synti;

      double meterValue[2];
      double meterPeakValue[2];
      int peakTimer[2];

      EventMap events;                    // playlist for playback mode
      EventMap::const_iterator eventsEnd;
      EventMap renderEvents;              // event list that is rendered in background
      RangeMap renderEventsStatus;
      MidiRenderer midi;
      QFuture<void> midiRenderFuture;
      bool allowBackgroundRendering = false; // should be set to true only when playing, so no
                                             // score changes are possible.
      EventMap countInEvents;             // playlist of any metronome countin clicks
      QQueue<NPlayEvent> _liveEventQueue; // playlist for score editing and note entry (rendered live)

      int playFrame;                      // current play position in samples, relative to the first frame of playback
      int countInPlayFrame;               // current play position in samples, relative to the first frame of countin
      int endUTick;                       // the final tick of midi events collected by collectEvents()

      EventMap::const_iterator playPos;   // moved in real time thread
      EventMap::const_iterator countInPlayPos;
      EventMap::const_iterator guiPos;    // moved in gui thread

      QList<const Note*> markedNotes;     // notes marked as sounding

      struct MetronomeCustomSample {
            std::vector<float> data;       // interleaved stereo

            unsigned frames() const
                  {
                  return unsigned(data.size() / 2);
                  }
            };

      std::atomic<const MetronomeCustomSample*> _customMetronomeTickSample { nullptr };
      std::atomic<const MetronomeCustomSample*> _customMetronomeTackSample { nullptr };

      // Samples are immutable after loading. Retired generations remain
      // alive while referenced by the realtime thread and are reclaimed
      // on a later sample reload
      std::vector<std::unique_ptr<MetronomeCustomSample>> _metronomeSampleStorage;

      std::atomic<const MetronomeCustomSample*> _tickCustomSampleInUse { nullptr };
      std::atomic<const MetronomeCustomSample*> _tackCustomSampleInUse { nullptr };
      std::atomic<const MetronomeCustomSample*> _independentTickCustomSampleInUse { nullptr };
      std::atomic<const MetronomeCustomSample*> _independentTackCustomSampleInUse { nullptr };

      QString _loadedMetronomeTickPath;
      QString _loadedMetronomeTackPath;
      int _loadedMetronomeSampleRate { 0 };

      uint tackRemain;        // metronome state (remaining audio samples)
      uint tickRemain;
      qreal tackVolume;       // relative volumes
      qreal tickVolume;
      qreal metronomeVolume;       // realtime-thread value
      qreal metronomeVolumeValue;  // GUI/persisted value

      uint independentTackRemain;
      uint independentTickRemain;
      qreal independentTackVolume;
      qreal independentTickVolume;

      unsigned initialMillisecondTimestampWithLatency; // millisecond timestamp (relative to PortAudio's initialization) of start of playback

      QTimer* heartBeatTimer;
      QTimer* noteTimer;

      bool independentMetronomeEnabledValue;
      double independentMetronomeBpmValue;
      int independentMetronomeNumeratorValue;
      int independentMetronomeDenominatorValue;
      bool independentMetronomeFollowPlaybackValue;
      bool independentMetronomeBeatAccentsValue;

      // Realtime-thread copies of the configuration above
      bool independentMetronomeEnabledRT;
      double independentMetronomeBpmRT;
      int independentMetronomeNumeratorRT;
      int independentMetronomeDenominatorRT;
      bool independentMetronomeFollowPlaybackRT;
      bool independentMetronomeBeatAccentsRT;

      // True only while Follow Playback is controlling the
      // independent metronome
      bool independentMetronomeFollowPlaybackActiveRT;

      // Realtime-owned free-running clock state
      int independentMetronomeRtick;
      double independentMetronomeFramesUntilNextClick;

      /**
       * Preferences cached for faster access in realtime context.
       * Using QSettings-based Ms::Preferences directly results in
       * audible glitches on some systems (esp. MacOS, see #280493).
       */
      struct CachedPreferences {
            int portMidiOutputLatencyMilliseconds = 0;
            bool jackTimeBaseMaster = false;
            bool useJackTransport = false;
            bool useJackMidi = false;
            bool useJackAudio = false;
            bool useAlsaAudio = false;
            bool usePortAudio = false;
            bool usePulseAudio = false;

            void update();
            };
      CachedPreferences cachedPrefs;

      void startTransport();
      void stopTransport();

      void renderChunk(const MidiRenderer::Chunk&, EventMap*);
      void updateEventsEnd();

      void setPos(int);
      void playEvent(const NPlayEvent&, unsigned framePos);
      void guiToSeq(const SeqMsg& msg);
      void metronome(unsigned n, float* l, bool force);
      void independentMetronome(unsigned n, float* l);
      void mixIndependentMetronomeSamples(unsigned n, float* l);

      void mixMetronomeSample(unsigned n, float* p,
                              uint& remain, qreal volume,
                              const MetronomeCustomSample* customSample,
                              const double* defaultSample,
                              unsigned defaultFrames);

      void startMetronomeTick(qreal volume, bool independent);
      void startMetronomeTack(qreal volume, bool independent);
      qreal independentMetronomeEventVolume(const NPlayEvent& event) const;

      const MetronomeCustomSample* loadMetronomeSample(const QString& path);
      void reloadMetronomeSamples(bool force = false);

      const MetronomeCustomSample* acquireMetronomeSample(
            const std::atomic<const MetronomeCustomSample*>& published,
            std::atomic<const MetronomeCustomSample*>& inUse);

      void reclaimRetiredMetronomeSamples();

      void resetIndependentMetronomeRealtimeState();
      void syncIndependentMetronomeRealtimeConfig();
      void updateIndependentMetronomeFollowState();
      int independentMetronomeClickTicks() const;
      double independentMetronomeFramesPerClick() const;

      void seekCommon(int utick);
      void unmarkNotes();
      void updateSynthesizerState(int tick1, int tick2);
      void addCountInClicks();

      int getPlayStartUtick();

      inline QQueue<NPlayEvent>* liveEventQueue() { return &_liveEventQueue; }

   private slots:
      void seqMessage(int msg, int arg = 0);
      void heartBeatTimeout();
      void midiInputReady();
      void setPlaylistChanged() { playlistChanged = true; }
      void handleTimeSigTempoChanged();

   public slots:
      void setRelTempo(double);
      void seek(int utick);
      void seekRT(int utick);
      void stopNotes(int channel = -1, bool realTime = false);
      void start();
      void stop();
      void setPos(POS, unsigned);
      void setMetronomeGain(float val);

   signals:
      void started();
      void stopped();
      int toGui(int, int arg = 0);
      void heartBeat(int, int, int);
      void tempoChanged();
      void timeSigChanged();

   public:
      Seq();
      ~Seq();
      bool canStart();
      void rewindStart();
      void loopStart();
      void seekEnd();
      void nextMeasure();
      void nextChord();
      void prevMeasure();
      void prevChord();

      void collectEvents(int utick);
      void ensureBufferAsync(int utick);
      void guiStop();
      void stopWait();
      void setLoopIn();
      void setLoopOut();
      void setLoopSelection();

      bool init(bool hotPlug = false);
      void exit();
      bool isRunning() const    { return running; }
      bool isPlaying() const    { return state == Transport::PLAY; }
      bool isStopped() const    { return state == Transport::STOP; }

      void processMessages();
      void process(unsigned framesPerPeriod, float* buffer);
      int getEndUTick() const   { return endUTick;  }
      bool isRealtime() const   { return true;     }
      void sendMessage(SeqMsg&) const;

      void setController(int, int, int);
      virtual void sendEvent(const NPlayEvent&) override;
      void setScoreView(ScoreView*);
      MasterScore* score() const   { return cs; }
      ScoreView* viewer() const { return cv; }
      void initInstruments(bool realTime = false);

      Driver* driver()                                 { return _driver; }
      void setDriver(Driver* d)                        { _driver = d;    }
      MasterSynthesizer* synti() const                 { return _synti;  }
      void setMasterSynthesizer(MasterSynthesizer* ms) { _synti = ms;    }

      int getCurTick();
      double curTempo() const;

      void putEvent(const NPlayEvent&, unsigned framePos = 0);
      void startNoteTimer(int duration);
      virtual void startNote(int channel, int, int, double nt) override;
      virtual void startNote(int channel, int, int, int, double nt) override;
      virtual void playMetronomeBeat(BeatType type) override;

      void setIndependentMetronomeEnabled(bool enabled);
      bool independentMetronomeEnabled() const;

      void setIndependentMetronomeBpm(double bpm);
      double independentMetronomeBpm() const
            { return independentMetronomeBpmValue; }

      void setIndependentMetronomeTimeSignature(int numerator, int denominator);
      int independentMetronomeNumerator() const
            { return independentMetronomeNumeratorValue; }
      int independentMetronomeDenominator() const
            { return independentMetronomeDenominatorValue; }

      void setIndependentMetronomeFollowPlayback(bool follow);
      bool independentMetronomeFollowPlayback() const
            { return independentMetronomeFollowPlaybackValue; }

      void setIndependentMetronomeBeatAccents(bool enabled);
      bool independentMetronomeBeatAccents() const
            { return independentMetronomeBeatAccentsValue; }

      void eventToGui(NPlayEvent);
      void stopNoteTimer();
      void recomputeMaxMidiOutPort();
      float metronomeGain() const { return metronomeVolumeValue; }

      void setInitialMillisecondTimestampWithLatency();
      unsigned getCurrentMillisecondTimestampWithLatency(unsigned framePos) const;

      void preferencesChanged();
      };

extern Seq* seq;
extern void initSequencer();
extern bool initMidi();

} // namespace Ms
#endif

