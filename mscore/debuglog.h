//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//=============================================================================

#ifndef __DEBUGLOG_H__
#define __DEBUGLOG_H__

#include <QDockWidget>
#include <QStringList>

#include <cstdint>

class QCheckBox;
class QPlainTextEdit;
class QTimer;

namespace Ms {

//---------------------------------------------------------
//   DebugLogDock
//---------------------------------------------------------

class DebugLogDock : public QDockWidget {
	  QPlainTextEdit* _output { nullptr };
	  QCheckBox* _enabledCheck { nullptr };
	  QTimer* _flushTimer { nullptr };

	  QStringList _compactMessages;
	  QStringList _detailedMessages;

	  bool _autoScroll { true };
	  bool _showDetails { false };

	  uint32_t _preferenceListenerId { 0 };

	  void flushMessages();
	  void refreshOutput();
	  void setLoggingEnabled(bool enabled);

   public:
	  explicit DebugLogDock(QWidget* parent = nullptr);
	  ~DebugLogDock();
	  };

void setDebugLogMessageHandlerEnabled(bool enabled);
bool debugLogMessageHandlerEnabled();

} // namespace Ms

#endif
