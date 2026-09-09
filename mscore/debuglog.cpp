//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//=============================================================================

#include "debuglog.h"

#include "preferences.h"

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QDateTime>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QMutex>
#include <QMutexLocker>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <cstdio>

namespace Ms {

namespace {

constexpr int MAX_LOG_MESSAGES = 10000;

QMutex debugLogMutex;
QStringList pendingCompactMessages;
QStringList pendingDetailedMessages;

QtMessageHandler previousMessageHandler = nullptr;
bool messageHandlerInstalled = false;

//---------------------------------------------------------
//   messageTypeName
//---------------------------------------------------------

const char* messageTypeName(QtMsgType type)
	  {
	  switch (type) {
			case QtDebugMsg:
				  return "Debug";
			case QtInfoMsg:
				  return "Info";
			case QtWarningMsg:
				  return "Warning";
			case QtCriticalMsg:
				  return "Critical";
			case QtFatalMsg:
				  return "Fatal";
			}
	  return "Log";
	  }

//---------------------------------------------------------
//   debugLogMessageHandler
//---------------------------------------------------------

void debugLogMessageHandler(QtMsgType type,
							const QMessageLogContext& context,
							const QString& msg)
	  {
	  const QString typeName =
			QString::fromLatin1(messageTypeName(type));

	  const QString compactMessage =
			QString("%1: %2").arg(typeName, msg);

	  QStringList contextParts;

	  if (context.file && *context.file) {
			QString source = QString::fromUtf8(context.file);

			if (context.line > 0)
				  source += QString(":%1").arg(context.line);

			contextParts.append(source);
			}

	  if (context.function && *context.function)
			contextParts.append(QString::fromUtf8(context.function));

	  const QString timestamp =
			QDateTime::currentDateTime().toString("HH:mm:ss.zzz");

	  QString detailedMessage =
			QString("%1 %2").arg(timestamp, typeName);

	  if (!contextParts.isEmpty())
			detailedMessage += QString(" [%1]").arg(contextParts.join(" | "));

	  detailedMessage += QString(": %1").arg(msg);

	  {
	  QMutexLocker locker(&debugLogMutex);

	  pendingCompactMessages.append(compactMessage);
	  pendingDetailedMessages.append(detailedMessage);

	  while (pendingCompactMessages.size() > MAX_LOG_MESSAGES) {
			pendingCompactMessages.removeFirst();
			pendingDetailedMessages.removeFirst();
			}
	  }

	  // Preserve the existing message output:
	  if (previousMessageHandler)
			// Windows debug build may already have mscoreMessageHandler()
			previousMessageHandler(type, context, msg);
	  else {
			const QByteArray formatted =
				  qFormatLogMessage(type, context, msg).toLocal8Bit();

			fprintf(stderr, "%s\n", formatted.constData());
			fflush(stderr);
			}
	  }

//---------------------------------------------------------
//   takePendingMessages
//---------------------------------------------------------

void takePendingMessages(QStringList* compactMessages,
						 QStringList* detailedMessages)
	  {
	  QMutexLocker locker(&debugLogMutex);

	  compactMessages->swap(pendingCompactMessages);
	  detailedMessages->swap(pendingDetailedMessages);
	  }

//---------------------------------------------------------
//   clearPendingMessages
//---------------------------------------------------------

void clearPendingMessages()
	  {
	  QMutexLocker locker(&debugLogMutex);

	  pendingCompactMessages.clear();
	  pendingDetailedMessages.clear();
	  }

} // namespace

//---------------------------------------------------------
//   installDebugLogMessageHandler
//---------------------------------------------------------

void setDebugLogMessageHandlerEnabled(bool enabled)
	  {
	  if (enabled) {
			if (messageHandlerInstalled)
				  return;

			previousMessageHandler =
				  qInstallMessageHandler(debugLogMessageHandler);

			messageHandlerInstalled = true;
			}
	  else {
			if (!messageHandlerInstalled)
				  return;

			qInstallMessageHandler(previousMessageHandler);

			previousMessageHandler = nullptr;
			messageHandlerInstalled = false;
			}
	  }

//---------------------------------------------------------
//   debugLogMessageHandlerEnabled
//---------------------------------------------------------

bool debugLogMessageHandlerEnabled()
	  {
	  return messageHandlerInstalled;
	  }

//---------------------------------------------------------
//   DebugLogDock
//---------------------------------------------------------

DebugLogDock::DebugLogDock(QWidget* parent)
   : QDockWidget(parent)
	  {
	  setObjectName("debug-log");
	  setWindowTitle("Debug Log");

	  QWidget* content = new QWidget(this);
	  QVBoxLayout* layout = new QVBoxLayout(content);
	  layout->setContentsMargins(4, 4, 4, 4);
	  layout->setSpacing(4);

	  QHBoxLayout* controls = new QHBoxLayout;

	  QPushButton* clearButton = new QPushButton(tr("Clear"), content);
	  QPushButton* copyButton = new QPushButton(tr("Copy All"), content);

	  _enabledCheck = new QCheckBox(tr("Enabled"), content);
	  _enabledCheck->setChecked(preferences.getBool(PREF_APP_DEBUG_LOG_ENABLED));

	  QCheckBox* detailsCheck = new QCheckBox(tr("Details"), content);
	  detailsCheck->setChecked(false);

	  QCheckBox* autoScrollCheck = new QCheckBox(tr("Autoscroll"), content);
	  autoScrollCheck->setChecked(true);

	  controls->addWidget(clearButton);
	  controls->addWidget(copyButton);
	  controls->addStretch();
	  controls->addWidget(_enabledCheck);
	  controls->addWidget(detailsCheck);
	  controls->addWidget(autoScrollCheck);

	  layout->addLayout(controls);

	  _output = new QPlainTextEdit(content);
	  _output->setReadOnly(true);
	  _output->setLineWrapMode(QPlainTextEdit::NoWrap);
	  _output->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

	  layout->addWidget(_output);

	  setWidget(content);

	  connect(clearButton, &QPushButton::clicked, this, [this]() {
			clearPendingMessages();

			_compactMessages.clear();
			_detailedMessages.clear();

			_output->clear();
			});

	  connect(copyButton, &QPushButton::clicked, this, [this]() {
			QApplication::clipboard()->setText(_output->toPlainText());
			});

	  connect(_enabledCheck, &QCheckBox::toggled, this, [](bool enabled) {
			if (preferences.getBool(PREF_APP_DEBUG_LOG_ENABLED) != enabled)
				  preferences.setPreference(PREF_APP_DEBUG_LOG_ENABLED, enabled);
			});

	  connect(detailsCheck, &QCheckBox::toggled, this, [this](bool checked) {
			flushMessages();

			_showDetails = checked;
			refreshOutput();
			});

	  connect(autoScrollCheck, &QCheckBox::toggled, this, [this](bool checked) {
			_autoScroll = checked;

			if (_autoScroll) {
				  QScrollBar* scrollBar = _output->verticalScrollBar();
				  scrollBar->setValue(scrollBar->maximum());
				  }
			});

	  _flushTimer = new QTimer(this);
	  _flushTimer->setInterval(100);

	  connect(_flushTimer, &QTimer::timeout, this, [this]() {
			flushMessages();
			});

	  _preferenceListenerId =
			preferences.addOnSetListener([this](const QString& key,
												const QVariant& value) {
				  if (key != PREF_APP_DEBUG_LOG_ENABLED)
						return;

				  const bool enabled = value.toBool();

				  if (_enabledCheck->isChecked() != enabled) {
						QSignalBlocker blocker(_enabledCheck);
						_enabledCheck->setChecked(enabled);
						}

				  setLoggingEnabled(enabled);
				  });

	  setLoggingEnabled(preferences.getBool(PREF_APP_DEBUG_LOG_ENABLED));
	  }

//---------------------------------------------------------
//   setLoggingEnabled
//---------------------------------------------------------

void DebugLogDock::setLoggingEnabled(bool enabled)
	  {
	  if (enabled) {
			setDebugLogMessageHandlerEnabled(true);

			// Immediately display anything collected between
			// startup and creation the DebugLogDock:
			flushMessages();

			if (_flushTimer && !_flushTimer->isActive())
				  _flushTimer->start();
			}
	  else {
			// Stop new messages entering the queue
			setDebugLogMessageHandlerEnabled(false);

			// Flush anything that reached the handler before it was removed
			flushMessages();

			if (_flushTimer)
				  _flushTimer->stop();
			}
	  }

//---------------------------------------------------------
//   ~DebugLogDock
//---------------------------------------------------------

DebugLogDock::~DebugLogDock()
	  {
	  if (_preferenceListenerId)
			preferences.removeOnSetListener(_preferenceListenerId);

	  setDebugLogMessageHandlerEnabled(false);
	  }

//---------------------------------------------------------
//   flushMessages
//---------------------------------------------------------

void DebugLogDock::flushMessages()
	  {
	  QStringList compactMessages;
	  QStringList detailedMessages;

	  takePendingMessages(&compactMessages, &detailedMessages);

	  if (compactMessages.isEmpty())
			return;

	  QScrollBar* scrollBar = _output->verticalScrollBar();
	  const int oldScrollValue = scrollBar->value();

	  _compactMessages.append(compactMessages);
	  _detailedMessages.append(detailedMessages);

	  bool removedOldMessages = false;

	  while (_compactMessages.size() > MAX_LOG_MESSAGES) {
			_compactMessages.removeFirst();
			_detailedMessages.removeFirst();
			removedOldMessages = true;
			}

	  if (removedOldMessages) {
            // Synchronize the retained message lists and the widget
			refreshOutput();
			return;
			}

	  const QStringList& messages =
			_showDetails ? detailedMessages : compactMessages;

	  _output->appendPlainText(messages.join('\n'));

	  if (_autoScroll)
			scrollBar->setValue(scrollBar->maximum());
	  else
			scrollBar->setValue(oldScrollValue);
	  }

//---------------------------------------------------------
//   refreshOutput
//---------------------------------------------------------

void DebugLogDock::refreshOutput()
	  {
	  QScrollBar* scrollBar = _output->verticalScrollBar();
	  const int oldScrollValue = scrollBar->value();

	  const QStringList& messages =
			_showDetails ? _detailedMessages : _compactMessages;

	  _output->setPlainText(messages.join('\n'));

	  if (_autoScroll)
			scrollBar->setValue(scrollBar->maximum());
	  else
			scrollBar->setValue(oldScrollValue);
	  }

} // namespace Ms
