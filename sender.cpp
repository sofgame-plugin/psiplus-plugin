/*
 * sender.cpp - Sof Game Psi plugin
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

#include <QtCore>

#include "sender.h"
#include "common.h"

Sender* mySender;

//int currentAccount;

QString alfa = "0123456789abcdefghijklmnopqrstuvwxyz";
StanzaSendingHost* sender_;
QStringList chatJids;
QList<short int> aPrefix = (QList<short int>()
	<< 22 << 13 << 5  << 16 << 29 << 4  << 5  << 34 << 27 << 25
	<< 4  << 5  << 16 << 27 << 29 << 1  << 9  << 17 << 2  << 3
	<< 17 << 13 << 15 << 27 << 2  << 2  << 3  << 4  << 2  << 20
	<< 19) ;

Sender::Sender() :
	currentAccount(-1),
	//currentAccJid(QString()),
	pingMirrors(false),
	jidInterval(600), // Частота отправки запросов, msec.
	waitForReceivePeriod(20000) // Ожидание ответа от игры в течении 20 сек.
{
	prefix = "";
	foreach (short int idx, aPrefix) {
		prefix.append(alfa.at(idx));
	}
	//prefix =      "md5gt45yrp45grt19h23hdfr22342kj";
	fastSendReg.setPattern(QString::fromUtf8("Можно отсылать не чаще .+ команд.+0- обновить."));
	gameSenderTimer.setSingleShot(true);
	connect(&gameSenderTimer, SIGNAL(timeout()), this, SLOT(doSendGameStringJob()));
}

Sender::~Sender()
{
	/**
	* Деструктор
	*/
	disconnect(&gameSenderTimer, SIGNAL(timeout()), this, SLOT(doSendGameStringJob()));
}

void Sender::changeAccount(int accIndex, QString accJid)
{
	currentAccount = accIndex;
	if (currentAccJid != accJid) {
		jidActiveCount = 0;
		currGameJidIndex = -1;
		lastSend = QDateTime::currentDateTime();
		gameMirrorsMode = 0;
		gameJidsEx.clear();
		pingQueue.clear();
		gameQueue.clear();
		systemQueue.clear();
		lastSendIndex = -1; // С какого jid-а последний раз отправляли
		waitingForReceive = false;
		sendCommandRetries = 0; // Количество попыток отправки команды
		lastCommand = "";
	}
	if (currentAccJid != accJid) {
		// Уведомляем другие модули
		currentAccJid = accJid;
		emit accountChanged(accJid);
	}
}

void Sender::setAccountStatus(int curr_status)
{
	// *** Смена статуса игрового аккаунта *** //  И как это радостное событие отловить, а?
	// Если аккаунт отключен, то устанавливаем игровым jid-ам статус отключенных
	if (curr_status == 0) {
		int cnt = gameJidsEx.size();
		for (int i = 0; i < cnt; i++) {
			setGameJidStatus(i, 0);
		}
	}
}

/**
 * Вставка нового джида jid на позицию pos
 * Если pos == -1, то добавляем в конец
 */
void Sender::insertGameJid(QString jid, int pos)
{
	if (jid.isEmpty())
		return;
	// Проверяем наличие такого зеркала
	if (getGameJidIndex(jid) != -1)
		return;
	// Вставляем запись
	struct jid_status jst;
	jst.jid = jid;
	jst.status = 0;
	jst.last_status = QDateTime();
	jst.last_send = QDateTime();
	jst.last_recv_game = QDateTime();
	jst.last_send_ping = QDateTime();
	jst.last_recv_ping = QDateTime();
	jst.probe_count = 0;
	jst.resp_average = 0.0f;
	if (pos < 0 || pos >= gameJidsEx.size()) {
		gameJidsEx.push_back(jst);
	} else {
		gameJidsEx.insert(pos, jst);
	}
}

/**
 * Удаление джида зеркала из списка зеркал
 */
void Sender::removeGameJid(QString jid)
{
	int i = getGameJidIndex(jid);
	if (i != -1) {
		gameJidsEx.remove(i);
		if (lastSendIndex == i)
			lastSendIndex = -1;
	}
}

int Sender::getGameJidIndex(QString jid)
{
	int cnt = gameJidsEx.size();
	for (int i = 0; i < cnt; i++) {
		if (gameJidsEx.at(i).jid == jid)
			return i;
	}
	return -1;
}

QStringList Sender::getGameJids()
{
	QStringList res;
	int cnt = gameJidsEx.size();
	for (int i = 0; i < cnt; i++) {
		res.push_back(gameJidsEx.at(i).jid);
	}
	return res;
}

const struct Sender::jid_status* Sender::getGameJidInfo(int jid_index)
{
	if (jid_index < 0 || jid_index >= gameJidsEx.size())
		return 0;
	return &gameJidsEx.at(jid_index);
}

bool Sender::setGameJidStatus(int jid_index, qint32 status)
{
	if (jid_index < 0 || jid_index >= gameJidsEx.size())
		return false;
	bool res = false;
	int oldJidActiveCount = jidActiveCount;
	struct jid_status* jstat = &gameJidsEx[jid_index];
	if (status == 0) {
		if (jstat->status != 0) {
			jstat->probe_count = 0;
			jstat->status = 0;
			jstat->last_status = QDateTime::currentDateTime();
			jidActiveCount--;
			res = true;
			// Отменяем пинг отключенного зеркала
			clearPingQueue(jid_index);
		}
	} else {
		if (jstat->status == 0) {
			jstat->status = status;
			jstat->last_status = QDateTime::currentDateTime();
			jidActiveCount++;
			res = true;
		}
	}
	if (oldJidActiveCount == 0 && jidActiveCount != 0) {
		doPingJob();
	}
	return res;
}

bool Sender::changeGameMirrorsMode(int mirrorsMode)
{
	/**
	* Устанавливает режим использования зеркал игры
	**/
	if (gameMirrorsMode != mirrorsMode) {
		gameMirrorsMode = mirrorsMode;
		if (mirrorsMode == 1) {
			startPingMirrors();
		} else {
			stopPingMirrors();
		}
	}
	return true;
}

bool Sender::setSendDelta(int delta)
{
	/**
	* Установка паузы между отправками пакетов серверу игры
	**/
	if (delta >= 0) {
		jidInterval = delta;
		return true;
	}
	return false;
}

int Sender::getSendDelta()
{
	/**
	* Возращает значение паузы между отправками пакетов серверу игры
	**/
	return jidInterval;
}

/**
 * Устанавливает таймаут ожидания ответа от сервера, в секундах
 */
bool Sender::setServerTimeoutDuration(int duration)
{
	if (duration > 5 && duration <= 60) {
		waitForReceivePeriod = duration * 1000;
		return true;
	}
	return false;
}

/**
 * Возвращает длительность ожидания ответа от сервера, в секундах
 */
int Sender::getServerTimeoutDuration()
{
	return (waitForReceivePeriod / 1000);
}

bool Sender::doGameAsk(QString* mirrorJid, QString* message)
{
	/**
	* Обработка ответа игры
	* Возвращает true если пакет полностью обработан функцией
	**/
	int jidIndex = getGameJidIndex(*mirrorJid);
	if (jidIndex != -1) {
		const struct jid_status* jstat = getGameJidInfo(jidIndex);
		if (jstat != 0) {
			// Отмечаем общее время приема пакета
			QDateTime lastReceive = QDateTime::currentDateTime();
			if (message->trimmed() == "000") {
				// Пришел пакет в результате пинга зеркала
				if (jstat->last_send_ping.isValid() && (!jstat->last_recv_ping.isValid() || jstat->last_send_ping > jstat->last_recv_ping)) { // TODO Надо продумать, что делать при значительном запоздании пакета
					// Похоже что ping послан нами
					// Отмечаем время получения пакета
					gameJidsEx[jidIndex].last_recv_ping = lastReceive;
					// Обсчитываем среднее
					int elapsed = gameJidsEx[jidIndex].last_send_ping.time().elapsed();
					int probe_cnt = gameJidsEx[jidIndex].probe_count;
					if (probe_cnt > 0) {
						float average_temp = gameJidsEx[jidIndex].resp_average;
						if (probe_cnt >= 10) {
							gameJidsEx[jidIndex].resp_average = (average_temp * 2.0f + elapsed) / 3.0f;
							probe_cnt = 3;
						} else {
							gameJidsEx[jidIndex].resp_average = (average_temp * (float)probe_cnt + elapsed) / ((float)probe_cnt + 1.0f);
							++probe_cnt;
						}
					} else {
						gameJidsEx[jidIndex].resp_average = elapsed;
						probe_cnt = 1;
					}
					gameJidsEx[jidIndex].probe_count = probe_cnt;
					//return true; // Это наш пинг и мы его обработали
				}
				return true; // блокируем вывод "000" в любом случае
			} else {
				// TODO сделать проверки на сообщения сервера без запроса
				// Это сообщение игры
				bool fDoSend = false;
				bool fDoCore = true;
				if (waitingForReceive && jidIndex == lastSendIndex) {
					// Это ожидаемый нами пакет
					waitingForReceive = false;
					// Проверяем на наличие ошибки частых команд
					if (fastSendReg.indexIn(*message, 0) == -1) {
						sendCommandRetries = 0;
						// Отмечаем дату приема сообщения
						gameJidsEx[jidIndex].last_recv_game = lastReceive;
					} else {
						fDoCore = false;
					}
					fDoSend = true;
				}
				// Отправляем текст ядру
				bool notShowText = true;
				if (fDoCore) {
					notShowText = emit gameTextReceived(mirrorJid, message);
				}
				if (fDoSend) {
					// Запускаем обработчик очереди
					doSendGameStringJob();
				}
				if (notShowText)
					return true;
			}
		}
	}
	return false;
}

/**
 * Добавляет команду для игры в очередь
 * Если других команд в очереди нет, инициирует процедуру отправки
 */
bool Sender::sendString(const QString &str)
{
	if (currentAccount == -1) {
		emit errorOccurred(ERROR_INCORRECT_ACCOUNT);
		return false;
	}
	bool is_empty = gameQueue.isEmpty();
	is_empty &= systemQueue.isEmpty();
	gameQueue.enqueue(str);
	emit queueSizeChanged(gameQueue.size());
	if (is_empty && sendCommandRetries == 0) {
		doSendGameStringJob();
	}
	return true;
}

/**
* Добавляет системную команду для игры в очередь системных команд с максимальным приоритетом
* Если других команд в очередях нет, инициирует процедуру отправки
*/
bool Sender::sendSystemString(QString* stringPtr)
{
	bool is_empty = gameQueue.isEmpty();
	is_empty &= systemQueue.isEmpty();
	systemQueue.enqueue(*stringPtr);
	if (is_empty && sendCommandRetries == 0) {
		doSendGameStringJob();
	}
	return true;
}

void Sender::startPingMirrors()
{
	/**
	* Запускает таймер для пинга зеркал, если хотя бы одно из зеркал активно
	**/
	if (!pingMirrors) {
		pingMirrors = true;
		if (jidActiveCount != 0) {
			doPingJob();
		}
	}
}

void Sender::stopPingMirrors()
{
	/**
	* Останавливает таймер пинга зеркал
	*/
	if (pingMirrors) {
		pingMirrors = false;
		doPingJob();
	}
}

void Sender::clearPingQueue(int mirrorIndex)
{
	/**
	* Очищаем очередь от пинга зеркала с указанным индексом
	* Пока очищается не вся очередь, а только те элементы, что у выхода
	**/
	while (!pingQueue.isEmpty()) {
		if (pingQueue.head() != mirrorIndex) {
			break;
		}
		pingQueue.dequeue();
	}
}

void Sender::doPingJob()
{
	/**
	* Контролирует очередь пингов зеркал
	**/
	if (!pingMirrors || jidActiveCount == 0) {
		pingQueue.clear();
		return;
	}
	QDateTime currTime = QDateTime::currentDateTime();
	if (pingQueue.isEmpty()) { // Новый пинг ставим в очередь, только если предыдущие отправлены
		// Получаем индекс текущего jid-а
		if (currGameJidIndex != -1 && gameJidsEx[currGameJidIndex].status != 0 && gameJidsEx[currGameJidIndex].last_send.isValid()) { // Игровой jid активен
			int timeDelta = gameJidsEx[currGameJidIndex].last_send.secsTo(currTime);
			if (timeDelta > 5 && timeDelta < 600) { // Пингуем только если простой в игре более 5 сек, но меньше чем 10 мин.
				// Проверяем, подошло ли время пинга зеркал
				int indexForPing = -1;
				QDateTime lastPing = currTime;
				int cnt = gameJidsEx.size();
				for (int i = 0; i < cnt; i++) {
					if (gameJidsEx[i].status != 0) { // Зеркало должно быть активно
						if (!gameJidsEx[i].last_send_ping.isValid()) {
							// Небыло ни одного пинга зеркала. Пингуем безусловно.
							indexForPing = i;
							gameJidsEx[i].probe_count = 0;
							break;
						}
						if (indexForPing == -1 || gameJidsEx[i].last_send_ping < lastPing) {
							// Это зеркало - кандидат на ping
							timeDelta = gameJidsEx[i].last_send_ping.secsTo(currTime);
							if (timeDelta >= 60) { // Время между пингами одного зеркала не должно быть меньше 1 мин.
								if (gameJidsEx[i].probe_count < 10 || timeDelta >= 1200) { // Менее 10ти пингов или прошло 20 мин.
									indexForPing = i;
									lastPing = gameJidsEx[i].last_send_ping;
								}
							}
						}
					}
				}
				if (indexForPing != -1) {
					if (gameJidsEx[indexForPing].last_send_ping.isValid()) {
						if (timeDelta >= 3600) {
							// Если последний замер был час назад, то сбрасываем результаты замеров
							gameJidsEx[indexForPing].probe_count = 0;
						} else if (!gameJidsEx[indexForPing].last_recv_ping.isValid() || gameJidsEx[indexForPing].last_recv_ping < gameJidsEx[indexForPing].last_send_ping) {
							// В последний раз ответа от зеркала не было
							gameJidsEx[indexForPing].probe_count = 0;
						} else if (gameJidsEx[indexForPing].probe_count >= 10) {
							gameJidsEx[indexForPing].probe_count = 3; // Принижаем ценность замеров
						}
					}
					// Добавляем в очередь
					pingQueue.enqueue(indexForPing);
				}
			}
		}
	}
	// По-умолчанию следующая проверка для пинга через 5 сек.
	int timeout = 5000;
	if (!pingQueue.isEmpty()) {
		if (gameQueue.isEmpty() && systemQueue.isEmpty() && sendCommandRetries == 0) {
			int timeDeltaMsec = lastSend.time().msecsTo(currTime.time());
			//int timeDeltaMsec2 = lastReceive.time().msecsTo(currTime.time());
			//if (timeDeltaMsec2 ? timeDeltaMsec) {
			//	timeDeltaMsec = timeDeltaMsec2;
			//}
			if (timeDeltaMsec < 0) {
				timeDeltaMsec += 86400000;
			}
			if (timeDeltaMsec >= jidInterval) {
				// Можно отправлять данные
				int i = pingQueue.dequeue();
				if (gameJidsEx[i].status != 0) {
					gameJidsEx[i].last_send_ping = currTime;
					gameJidsEx[i].last_send_ping.time().start();
					lastSend = currTime;
					sender_->sendMessage(currentAccount, gameJidsEx[i].jid, "000", "", "chat");
				}
			} else {
				timeout = jidInterval - timeDeltaMsec;
			}
		}
	}
	if (timeout != 0) {
		QTimer::singleShot(timeout, this, SLOT(doPingJob()));
	}
}

/**
 * Просматривает очередь команд игры и отправляет команду из очереди серверу.
 * Если прошло слишком мало времени после последней команды, то инициируется таймер.
 */
void Sender::doSendGameStringJob() {
	if (gameSenderTimer.isActive())
		gameSenderTimer.stop();
	if (gameQueue.isEmpty() && sendCommandRetries == 0 && systemQueue.isEmpty()) {
		// Выходим без инициализации таймера, т.к. нечего отсылать
		return;
	}
	if (sendCommandRetries >= 3) {
		// Команды идут слишком часто
		systemQueue.clear();
		if (!gameQueue.isEmpty()) {
			gameQueue.clear();
			emit queueSizeChanged(0);
		}
		sendCommandRetries = 0;
		emit errorOccurred(ERROR_QUICK_COMMAND);
		return;
	}
	// Получаем текущее время
	QDateTime currTime = QDateTime::currentDateTime();
	// Проверяем, пришел ли ответ на предыдущий пакет
	if (waitingForReceive) {
		// Ответа пока не было
		int timeDeltaMsec = gameJidsEx[lastSendIndex].last_send.time().msecsTo(currTime.time());
		if (timeDeltaMsec < 0) {
			timeDeltaMsec += 86400000;
		}
		timeDeltaMsec = waitForReceivePeriod - timeDeltaMsec;
		if (timeDeltaMsec > 0) {
			// Продлеваем ожидание ответа от сервера
			if (timeDeltaMsec < 10) {
				timeDeltaMsec = 10;
			}
			gameSenderTimer.start(timeDeltaMsec);
			return;
		}
		// Долго нет ответа на предыдущую посылку
		systemQueue.clear();
		waitingForReceive = false;
		sendCommandRetries = 0;
		if (!gameQueue.isEmpty()) {
			gameQueue.clear();
			emit queueSizeChanged(0);
		}
		emit errorOccurred(ERROR_SERVER_TIMEOUT);
		return;
	}
	// Проверка, сколько прошло времени после последней отправки
	int timeDeltaMsec = lastSend.time().msecsTo(currTime.time());
	if (timeDeltaMsec < 0) {
		timeDeltaMsec += 86400000;
	}
	int timeout = 0;
	if (timeDeltaMsec >= jidInterval) {
		// *** Ищем подходящее зеркало из доступных ***
		// Получаем количество доступных зеркал
		int cnt = gameJidsEx.size();
		// Инициируем индекс и вес
		int jidIndex = -1;
		qint32 jidWeight = QINT32_MIN;
		for (int i = 0; i < cnt; i++) {
			if (gameJidsEx[i].status == 1) {
				// Зеркало подключено
				if (gameMirrorsMode == 0) {
					// Используем первый доступный Джид
					jidIndex = i;
					break;
				}
				qint32 weight = 0; // Эквивалентно: статус не менялся в теч. 10мин, не играли в течении часа, нет тестов
				// Учитываем когда зеркало последний раз меняло статус
				if (gameJidsEx[i].last_status.isValid()) {
					int timeDelta = gameJidsEx[i].last_status.secsTo(currTime);
					if (timeDelta < 10) {
						weight -= 600;
					} else if (timeDelta < 60) {
						weight -= 350;
					} else if (timeDelta < 600) {
						weight -= 150;
					}
				} else {
					weight -= 20;
				}
				//qDebug() << "--Index: " << i << ", weight: " << weight;
				// Проверяем отклик и последний пакет с зеркала
				if (gameJidsEx[i].last_send.isValid()) {
					if (gameJidsEx[i].last_recv_game.isValid() && gameJidsEx[i].last_recv_game >= gameJidsEx[i].last_send) {
						int timeDelta = gameJidsEx[i].last_recv_game.secsTo(currTime);
						if (timeDelta <= 2) {
							// Активная игра, jid без особой надобности не меняем.
							weight += 300;
						} else if (timeDelta <= 10) {
							// Игра средней скорости
							weight += 200;
						} else if (timeDelta <= 60) {
							// Неторопливая игра с небольшими паузами
							weight += 100;
						} else if (timeDelta <= 600) {
							// Игра с большими паузами
							weight += 40;
						} else if (timeDelta <= 3600) {
							// Возможно отходили или ждали восстановления
							weight += 10;
						}
					} else {
						int timeDelta = gameJidsEx[i].last_send.secsTo(currTime);
						weight -= 50;
						if (timeDelta > 10) {
							weight -= 50;
						}
					}
				}
				//qDebug() << "--Index: " << i << ", weight: " << weight;
				// Учитываем результаты подсчета среднего отклика зеркала
				if (gameJidsEx[i].probe_count > 0) {
					float average = gameJidsEx[i].resp_average;
					int weight_tmp = 0;
					if (average <= 300) {
						weight_tmp = 250;
					} else if (average <= 500) {
						weight_tmp = 130;
					} else if (average <= 1000) {
						weight_tmp = 20;
					} else if (average <= 2000) {
						weight_tmp = -30;
					} else {
						weight_tmp = -250;
					}
					if (gameJidsEx[i].probe_count < 3) {
						weight_tmp = weight_tmp / 2;
					} else if (gameJidsEx[i].probe_count < 5) {
						weight_tmp = weight_tmp * 7 / 10 ;
					}
					weight += weight_tmp;
				}
				//qDebug() << "--Index: " << i << ", weight: " << weight << ", probe: " << gameJidsStatus[i].probe_count << ", average: " << gameJidsStatus[i].resp_average;
				// Сверяем весовые коэффициенты
				if (jidWeight < weight) {
					jidWeight = weight;
					jidIndex = i;
				}
			}
		}
		if (jidIndex == -1) {
			// Нет ни одного доступного зеркала
			waitingForReceive = false;
			systemQueue.clear();
			sendCommandRetries = 0;
			if (!gameQueue.isEmpty()) {
				gameQueue.clear();
				emit queueSizeChanged(0);
			}
			emit errorOccurred(ERROR_NO_MORE_MIRRORS);
			return; // И выходим
		}
		//qDebug() << "Index: " << jidIndex << ", weight: " << jidWeight;
		// Отмечаем время отправки
		gameJidsEx[jidIndex].last_send = currTime;
		lastSend = currTime;
		// Отсылаем строку
		if (sendCommandRetries == 0) {
			lastCommand = prefix;
			if (!systemQueue.isEmpty()) {
				lastCommand.append(systemQueue.dequeue());
			} else {
				lastCommand.append(gameQueue.dequeue());
				emit queueSizeChanged(gameQueue.size());
			}
		}
		sender_->sendMessage(currentAccount, gameJidsEx[jidIndex].jid, lastCommand, "", "chat");
		lastSendIndex = jidIndex;
		waitingForReceive = true;
		sendCommandRetries++;
		currGameJidIndex = jidIndex;
		// Проверяем наличие команд в очереди если не пустая, взводим таймер
//		if (!gameQueue.isEmpty()) {
//			timeout = jidInterval;
//		} else {
			timeout = waitForReceivePeriod;
//		}
	} else {
		timeout = jidInterval - timeDeltaMsec;
	}
	if (timeout > 0) {
		gameSenderTimer.start(timeout);
	}
}

/**
 * Возвращает длину очереди команд игры
 */
int  Sender::getGameQueueLength()
{
	int res = gameQueue.size() + systemQueue.size();
	if (sendCommandRetries != 0)
		res++;
	return res;
}

/**
 * Сбрасывает очередь команд игры
 */
void Sender::resetGameQueue()
{
	waitingForReceive = false;
	systemQueue.clear();
	sendCommandRetries = 0;
	if (!gameQueue.isEmpty()) {
		gameQueue.clear();
		emit queueSizeChanged(0);
	}
}
