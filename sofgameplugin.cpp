/*
 * sof_game.cpp - Sof Game Psi plugin
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

#include <QtGui>
//#include <QtCore>

#include "sofgameplugin.h"
#include "pluginhosts.h"
#include "sender.h"
#include "plugin_core.h"

Q_EXPORT_PLUGIN(SofGamePlugin);

SofGamePlugin::SofGamePlugin() :
	enabled(false),
	currentAccount(-1),
	currAccActive(false),
	shortCut("Ctrl+Alt+g"),
	gameJidsWid(NULL),
	chatJidsWid(NULL),
	shortCutWid(NULL),
	accountsWid(NULL),
	psiShortcuts(NULL),
	accInfoHost(NULL)

{
	PluginHosts::psiOptions = 0;
	sender_ = 0;
	PluginHosts::myPopupHost = 0;

	chatJids << tr("sofch@jabber.ru");
}

SofGamePlugin::~SofGamePlugin() {
	disable();
}

QString SofGamePlugin::name() const
{
	return "SofGame Plugin";
}

QString SofGamePlugin::shortName() const
{
	return "sofgame";
}

QString SofGamePlugin::version() const
{
	return cVer;
}

bool SofGamePlugin::enable()
{
	if (PluginHosts::psiOptions == NULL || sender_ == NULL)
		return false;
	// Инициируем модуль приема/передачи плагина
	Sender *senderObj = Sender::instance();
	// Инициируем загрузку ядра плагина
	PluginCore::instance();
	//--
	enabled = true;
	// Получаем данные аккаунта
	QVariant vGameAccount(accountJid);
	vGameAccount = PluginHosts::psiOptions->getPluginOption(constGameAccount);
	int newAccountId = -1;
	QString newAccJid;
	if (!vGameAccount.isNull()) {
		newAccJid = vGameAccount.toString();
		newAccountId = getAccountByJid(newAccJid);

	}
	currentAccount = newAccountId;
	accountJid = newAccJid;
	// Передаем модулю данные об выбранном аккаунте
	senderObj->changeAccount(currentAccount, accountJid);
	// Получаем список игровых джидов из настроек
	QStringList game_jids = PluginHosts::psiOptions->getPluginOption(constGameJids).toStringList();
	// Отправляем модулю
	if (game_jids.isEmpty()) {
		senderObj->insertGameJid("sof@jabbergames.ru", -1);
		senderObj->insertGameJid("sofgm@jabber.ru", -1);
	} else {
		int cnt = game_jids.size();
		for (int i = 0; i < cnt; i++) {
			senderObj->insertGameJid(game_jids.at(i), -1);
		}
	}
	// Данные о джиде чата
	chatJids = PluginHosts::psiOptions->getPluginOption(constChatJids).toStringList();
	// Горячая клавиша для вызова окна планина
	shortCut = PluginHosts::psiOptions->getPluginOption(constShortCut).toString();
	if (psiShortcuts)
		psiShortcuts->connectShortcut(QKeySequence(shortCut), this, SLOT(doShortCut()));
	// --
	if (currentAccount != -1) {
		// Проверяем статус аккаунта
		QString jid_str = accInfoHost->getStatus(currentAccount);
		if (jid_str != "offline") {
			// Отсылаем запросы об активности зеркал
			QStringList jids = senderObj->getGameJids();
			while (!jids.isEmpty()) {
				QString jid = jids.takeFirst();
				sendLastActiveQuery(accountJid, jid);
			}
		}
	}
	return true;
}

bool SofGamePlugin::disable()
{
	if (psiShortcuts) {
		psiShortcuts->disconnectShortcut(QKeySequence(shortCut), this, SLOT(doShortCut()));
		//psiShortcuts = 0;
	}
	PluginCore::reset();
	Sender::reset();
	enabled = false;
	return true;
}

QWidget* SofGamePlugin::options()
{
	if (!enabled) {
		return 0;
	}
	//---
	QWidget *optionsWid = new QWidget();
	QVBoxLayout *layout = new QVBoxLayout(optionsWid);
	// Текст с предупреждением
	layout->addWidget(new QLabel(QString::fromUtf8("Внимание! Перед началом игры, необходимо добавить указанные ниже JID-ы в ростер."), optionsWid));
	// Список Jid-ов игры
	gameJidsWid = new QTextEdit(optionsWid);
	QVBoxLayout *gameJidsVertLayout = new QVBoxLayout();
	gameJidsVertLayout->addWidget(new QLabel(QString::fromUtf8("JID-ы игры:"), optionsWid));
	gameJidsVertLayout->addWidget(gameJidsWid);
	// Список Jid-ов чата
	chatJidsWid = new QTextEdit(optionsWid);
	QVBoxLayout *chatJidsVertLayout = new QVBoxLayout();
	chatJidsVertLayout->addWidget(new QLabel(QString::fromUtf8("JID-ы чата:"), optionsWid));
	chatJidsVertLayout->addWidget(chatJidsWid);
	// --
	QHBoxLayout *jidsHorLayout = new QHBoxLayout();
	jidsHorLayout->addLayout(gameJidsVertLayout);
	jidsHorLayout->addLayout(chatJidsVertLayout);
	layout->addLayout(jidsHorLayout);
	// Горячие клавиши
	QHBoxLayout *hotkeyHorLayout = new QHBoxLayout();
	hotkeyHorLayout->addWidget(new QLabel(QString::fromUtf8("Комбинация клавиш:"), optionsWid),0,Qt::AlignLeft);
	shortCutWid = new QLineEdit(optionsWid);
	shortCutWid->setFixedWidth(100);
	shortCutWid->setDisabled(true);
	hotkeyHorLayout->addWidget(shortCutWid,200,Qt::AlignLeft);
	QPushButton *shortCutButton = new QPushButton(optionsWid);
	shortCutButton->setText(QString::fromUtf8("Изменить"));
	connect(shortCutButton, SIGNAL(released()), SLOT(selectShortCut()));
	hotkeyHorLayout->addWidget(shortCutButton,400,Qt::AlignLeft);
	//--
	layout->addLayout(hotkeyHorLayout);
	// Получаем список аккаунтов текущего профиля
	QList< QPair<QString, QString> > accs = getAccounts();
	// Формируем элементы для списка аккаунтов
	QHBoxLayout *accountHorLayout = new QHBoxLayout;
	accountHorLayout->addWidget(new QLabel(QString::fromUtf8("Игровой аккаунт:")), 0, Qt::AlignLeft);
	accountsWid = new QComboBox(optionsWid);
	accountsWid->clear();
	for (int i = 0, cnt = accs.size(); i < cnt; i++) {
		QString name = accs.at(i).first;
		QString jid = accs.at(i).second;
		accountsWid->addItem(QString("%1 (%2)").arg(name).arg(jid), jid);
	}
	//--
	accountHorLayout->addWidget(accountsWid, 400, Qt::AlignLeft);
	// Ссылка на сайт игры
	QLabel *gameSiteLink = new QLabel(tr("<a href=\"http://www.jabbergames.ru/\">SoF game site (Online)</a>"), optionsWid);
	gameSiteLink->setOpenExternalLinks(true);
	layout->addLayout(accountHorLayout);
	layout->addWidget(gameSiteLink, 400, Qt::AlignLeft);
	//--
	restoreOptions();
	return optionsWid;
}

void SofGamePlugin::applyOptions() {
	if (!gameJidsWid || !chatJidsWid || !shortCutWid || !accountsWid) {
		return;
	}
	Sender *senderObj = Sender::instance();
	// *** Аккаунт игры ***
	int index = accountsWid->currentIndex();
	QString newAccJid;
	if (index != -1) {
		newAccJid = accountsWid->itemData(index).toString();
	}
	// Сохраняем новый джид
	PluginHosts::psiOptions->setPluginOption(constGameAccount, newAccJid);
	// Если он изменился, отсылаем новые данные акка sender-у и обнавляем у себя
	int newAccountId = getAccountByJid(newAccJid);
	if (currentAccount != newAccountId || newAccJid != accountJid) {
		currentAccount = newAccountId;
		accountJid = newAccJid;
		senderObj->changeAccount(currentAccount, accountJid);
	}
	// *** Jid-ы игры ***
	QStringList game_jids = gameJidsWid->toPlainText().split(QRegExp("\\s+"), QString::SkipEmptyParts);
	// Сохраняем в настройках
	PluginHosts::psiOptions->setPluginOption(constGameJids, game_jids);
	// Получаем старый набор джидов игры
	QStringList old_game_jids = senderObj->getGameJids();
	// Производим изменения списка зеркал в модуле отправки
	bool fActive = false;
	if (currentAccount != -1 && accInfoHost->getStatus(currentAccount) != "offline")
		fActive = true;
	while (!old_game_jids.isEmpty()) { // Проходимся по старому списку зеркал игры
		QString jid = old_game_jids.takeFirst();
		if (!game_jids.removeOne(jid)) {
			// В новом списке нет такого зеркала, удаляем в sender-е
			senderObj->removeGameJid(jid);
		}
	}
	while (!game_jids.isEmpty()) { // Добавляем ранее отсутствующие зеркала в sender
		QString jid = game_jids.takeFirst();
		senderObj->insertGameJid(jid, -1);
		if (fActive)
			sendLastActiveQuery(accountJid, jid);
	}
	// *** Jid-ы чата ***
	chatJids = chatJidsWid->toPlainText().split(QRegExp("\\s+"), QString::SkipEmptyParts);
	PluginHosts::psiOptions->setPluginOption(constChatJids, chatJids);
	// --
	psiShortcuts->disconnectShortcut(QKeySequence(shortCut), this, SLOT(doShortCut()));
	shortCut = shortCutWid->text();
	PluginHosts::psiOptions->setPluginOption(constShortCut, shortCut);
	psiShortcuts->connectShortcut(QKeySequence(shortCut), this, SLOT(doShortCut()));
}

void SofGamePlugin::restoreOptions() {
	if (gameJidsWid == 0 || chatJidsWid == 0 || shortCutWid == 0 || accountsWid == 0) {
		return;
	}
	gameJidsWid->setText(Sender::instance()->getGameJids().join("\n"));
	// --
	chatJidsWid->setText(chatJids.join("\n"));
	// --
	shortCutWid->setText(shortCut);
	// Выбранный аккаунт
	int index = accountsWid->findData(accountJid);
	accountsWid->setCurrentIndex(index);
}

QString SofGamePlugin::pluginInfo() {
	return QString::fromUtf8("Автор: liuch\nEmail: liuch@mail.ru\n\n"
	"Этот плагин позволяет играть в jabber игру \"Камень судьбы\" используя графический интерфейс.\n"
	"Официальный сайт игры находится по адресу: http://www.jabbergames.ru/\n"
	"Для начала игры добавьте следующие джиды себе в ростер: sofgm@jabber.ru (джид игры), sof@jabbergames.ru (зеркало игры), sofch@jabber.ru(джид чата игры). "
	"Вызовите окно плагина горячей клавишей и отправьте 0 в игру через плагин. Приятной игры!");
}

void SofGamePlugin::selectShortCut()
{
	if (psiShortcuts) {
		psiShortcuts->requestNewShortcut(this, SLOT(onNewShortcutKey(QKeySequence)));
	}
}

void SofGamePlugin::onNewShortcutKey(QKeySequence ks)
{
	if (shortCutWid) {
		shortCutWid->setText(ks.toString(QKeySequence::NativeText));
	}
}

QList< QPair<QString, QString> > SofGamePlugin::getAccounts() const
{
	QList< QPair<QString, QString> > res;
	for (int i = 0; ; i++) {
		QString jid = accInfoHost->getJid(i);
		if (jid == "-1")
			break;
		if (!jid.isEmpty()) {
			QPair<QString, QString> p;
			QString name = accInfoHost->getName(i);
			if (name.isEmpty()) {
				name = QString("acc %1").arg(i + 1);
			}
			p.first = name;
			p.second = jid;
			res.append(p);
		}
	}
	return res;
}

int SofGamePlugin::getAccountByJid(const QString &jid) const
{
	if (!jid.isEmpty()) {
		// Перебираем аккаунты и находим свой
		for (int i = 0; ; i++) {
			// Проверяем аккаунт
			QString jid_str = accInfoHost->getJid(i);
			if (jid_str == "-1")
				break;
			if (jid == jid_str)
				return i;
		}
	}
	return -1;
}

void SofGamePlugin::sendLastActiveQuery(const QString &from_jid, const QString &to_jid)
{
	if (currentAccount != -1) {
		QString query = QString("<iq from=\"%1\" id=\"sofgame_plugin\" to=\"%2\" type=\"get\"><query xmlns=\"jabber:iq:last\"/></iq>")
			.arg(sender_->escape(from_jid)).arg(sender_->escape(to_jid));
		sender_->sendStanza(currentAccount, query);
	}
}

//-- OptionAccessor -------------------------------------------------

void SofGamePlugin::setOptionAccessingHost(OptionAccessingHost* host)
{
	PluginHosts::psiOptions = host;
}

void SofGamePlugin::optionChanged(const QString& /*option*/)
{
}

//-- StanzaSender ---------------------------------------------------

void SofGamePlugin::setStanzaSendingHost(StanzaSendingHost *host)
{
	sender_ = host;
}

//-- ShortcutAccessor -----------------------------------------------

void SofGamePlugin::setShortcutAccessingHost(ShortcutAccessingHost* host)
{
	psiShortcuts = host;
}

void SofGamePlugin::setShortcuts()
{
	if (enabled && psiShortcuts) {
		psiShortcuts->connectShortcut(QKeySequence(shortCut), this, SLOT(doShortCut()));
	}
}

// ----------------- ApplicationInfoAccessor -------------------
void SofGamePlugin::setApplicationInfoAccessingHost(ApplicationInfoAccessingHost* host) {
	PluginHosts::appInfoHost = host;
}

// ----------------- AccountInfoAccessor -------------------
void SofGamePlugin::setAccountInfoAccessingHost(AccountInfoAccessingHost* host) {
	accInfoHost = host;
}

// ----------------- PopupAccessor -------------------
void SofGamePlugin::setPopupAccessingHost(PopupAccessingHost* host) {
	//qDebug() << "SofGamePlugin::setPopupAccessingHost";
	PluginHosts::myPopupHost = host;
}

// ----------------------- StanzaFilter ------------------------------
bool SofGamePlugin::incomingStanza(int account, const QDomElement &stanza)
{
	if (!enabled)
		return false;
	Sender *senderObj = Sender::instance();
	if (currentAccount == -1) {
		// Проверяем JID, от которого пришло сообщение
		if (accountJid != accInfoHost->getJid(account))
			return false;
		// Сохраняем id аккаунта
		currentAccount = account;
		senderObj->changeAccount(account, accountJid);
	} else if (currentAccount != account) {
		return false;
	}
	// Проверяем JID от которого пришло сообщение
	QString jid = stanza.attribute("from");
	jid = jid.left(jid.indexOf("/")).toLower();
	int i = senderObj->getGameJidIndex(jid);
	if (i == -1) { // JID не найден
		return false;
	}
	// Просматриваем сообщение
	QString sTagName = stanza.tagName();
	if (sTagName == "message") {
		// Интересуют только сообщения в чате
		if (stanza.attribute("type") != "chat") {
			return false;
		}
		// Получаем текст сообщения
		QDomElement eBody = stanza.firstChildElement("body");
		if (eBody.isNull()) {
			return false;
		}
		QString message = eBody.text();
		// Передаем jid отправителя и текст обработчику
		bool res = senderObj->doGameAsk(jid, message);
		return res;
	} else if (sTagName == "presence") {
		if (stanza.attribute("type") == "unavailable") {
			PluginCore::instance()->setGameJidStatus(i, 0);
		} else {
			currAccActive = true;
			PluginCore::instance()->setGameJidStatus(i, 1);
		}
	} else if (sTagName == "iq") {
		if (stanza.attribute("type") == "result" && stanza.attribute("id") == "sofgame_plugin") {
			// Это ответ на запрос который посылался при загрузке плагина
			QDomElement queryElement = stanza.firstChildElement("query");
			if (!queryElement.isNull() && queryElement.attribute("xmlns") == "jabber:iq:last") {
				bool fOk = false;
				int secs = queryElement.attribute("seconds").toInt(&fOk);
				if (fOk) {
					if (secs == 0) {
						currAccActive = true;
						PluginCore::instance()->setGameJidStatus(i, 1);
					} else {
						PluginCore::instance()->setGameJidStatus(i, 0);
					}
				}
			}
		}
	}
	return false;
}

bool SofGamePlugin::outgoingStanza(int account, QDomElement& stanza)
{
	if (enabled && account == currentAccount) {
		if (stanza.tagName() == "presence" && stanza.attribute("to").isEmpty()) {
			if (stanza.attribute("type") == "unavailable") {
				if (currAccActive) {
					// Произошло отключение аккаунта пользователем
					currAccActive = false;
					PluginCore::instance()->setAccountStatus(0);
				}
			} else {
				if (!currAccActive) {
					// Аккаунт стал активен
					currAccActive = true;
					PluginCore::instance()->setAccountStatus(1);
				}
			}
		}
	}
	return false;
}

//-- private slots --------------------------------------------------

void SofGamePlugin::doShortCut()
{
	if (enabled) {
		PluginCore::instance()->doShortCut();
	}
}
