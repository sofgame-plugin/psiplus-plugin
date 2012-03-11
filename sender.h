/*
 * sender.h - Sof Game Psi plugin
 * Copyright (C) 2010  Aleksey Andreev
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * You can also redistribute and/or modify this program under the
 * terms of the Psi License, specified in the accompanied COPYING
 * file, as published by the Psi Project; either dated January 1st,
 * 2005, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef SENDER_H
#define SENDER_H

#include <QtCore>

class Sender: public QObject
{
Q_OBJECT

public:
	struct jid_status {
		QString     jid;
		int         status;
		QDateTime	last_status;
		QDateTime	last_send;
		QDateTime	last_recv_game; // Последнее игровое сообщение (ответ на запрос)
		QDateTime	last_send_ping;
		QDateTime	last_recv_ping;
		int 		probe_count;
		float		resp_average;
	};
	static Sender *instance();
	static void reset();
	void changeAccount(int, const QString &);
	void setAccountStatus(int curr_status);
	void insertGameJid(const QString &, int);
	void removeGameJid(const QString &);
	int  gameJidIndex(const QString &) const;
	QStringList gameJidList() const;
	void insertChatJid(const QString &, int);
	void removeChatJid(const QString &);
	int  chatJidIndex(const QString &) const;
	QStringList chatJidList() const;
	const struct jid_status* getGameJidInfo(int) const;
	bool setGameJidStatus(int, qint32);
	int  getGameMirrorsMode() const {return gameMirrorsMode;}
	bool setGameMirrorsMode(int mirrorsMode);
	bool setSendDelta(int);
	int  getSendDelta() const;
	bool setServerTimeoutDuration(int);
	int  getServerTimeoutDuration() const;
	bool doGameAsk(const QString &mirrorJid, const QString &message);
	bool sendString(const QString &str);
	bool sendSystemString(const QString &stringPtr);
	int  getGameQueueLength() const;
	void resetGameQueue();

private:
	static Sender *instanse_;
	int currentAccount;
	QString currentAccJid;
	QVector<struct jid_status> gameJidsEx;
	QStringList chatJids;
	int jidActiveCount;
	bool pingMirrors;
	int jidInterval;
	QDateTime lastSend;
	//QDateTime lastReceive;
	//QTimer* mirrorsProbeTimer;
	QQueue<int> pingQueue;
	QQueue<QString> gameQueue;
	QQueue<QString> systemQueue;
	int gameMirrorsMode;
	int currGameJidIndex;
	int  lastSendIndex; // TODO объединить с предыдущим
	bool waitingForReceive;
	int  waitForReceivePeriod;
	int  sendCommandRetries;
	QString lastCommand;
	QRegExp fastSendReg;
	QTimer gameSenderTimer;
	QString prefix;

private:
	Sender();
	~Sender();
	void startPingMirrors();
	void stopPingMirrors();
	void clearPingQueue(int mirrorIndex);

private slots:
	void doPingJob();
	void doSendGameStringJob();

signals:
	void accountChanged(const QString);
	bool gameTextReceived(const QString, const QString);
	void errorOccurred(int);
	void queueSizeChanged(int);

};

#endif
