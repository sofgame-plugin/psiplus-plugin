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
#include "utils.h"
#include "sender.h"
#include "plugin_core.h"

Q_EXPORT_PLUGIN(SofGamePlugin);

SofGamePlugin::SofGamePlugin() :
	enabled(false),
	currentAccount(-1),
	shortCut("Ctrl+Alt+g"),
	gameJidsWid(NULL),
	chatJidsWid(NULL),
	shortCutWid(NULL),
	accountsWid(NULL),
	psiShortcuts(NULL),
	accInfoHost(NULL)

{
	psiOptions = 0;
	sender_ = 0;
	mySender = 0;
	myPopupHost = 0;
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
	if (!psiOptions || !sender_)
		return false;
	// Инициируем модуль приема/передачи плагина
	mySender = new Sender;
	// Инициируем загрузку ядра плагина
	PluginCore::instance();
	//--
	enabled = true;
	// Получаем данные аккаунта
	QVariant vGameAccount(accountJid);
	vGameAccount = psiOptions->getPluginOption(constGameAccount);
	int newAccount = -1;
	QString newAccJid = "";
	if (!vGameAccount.isNull()) {
		newAccJid = vGameAccount.toString();
		newAccount = getAccountByJid(newAccJid);

	}
	if (currentAccount != newAccount || newAccJid != accountJid) {
	//if (currentAccount != newAccount) {
		currentAccount = newAccount;
		accountJid = newAccJid;
		if (mySender) {
			mySender->changeAccount(currentAccount, accountJid);
		}
	}
	QStringList game_jids;
	QVariant vGameJids(game_jids);
	vGameJids = psiOptions->getPluginOption(constGameJids);
	if (!vGameJids.isNull()) {
		game_jids = vGameJids.toStringList();
	}
	if (game_jids.isEmpty()) {
		mySender->insertGameJid("sof@jabbergames.ru", -1);
		mySender->insertGameJid("sofgm@jabber.ru", -1);
	} else {
		int cnt = game_jids.size();
		for (int i = 0; i < cnt; i++) {
			mySender->insertGameJid(game_jids.at(i), -1);
		}
	}
	QVariant vChatJids(chatJids);
	vChatJids = psiOptions->getPluginOption(constChatJids);
	if (!vChatJids.isNull()) {
		chatJids = vChatJids.toStringList();
	}
	QVariant vShortCut(shortCut);
	vShortCut = psiOptions->getPluginOption(constShortCut);
	if (!vShortCut.isNull()) {
	  shortCut = vShortCut.toString();
	}
	if (psiShortcuts)
		psiShortcuts->connectShortcut(QKeySequence(shortCut), this, SLOT(doShortCut()));
	// --
	if (currentAccount != -1) {
		// Проверяем статус аккаунта
		QString jid_str = accInfoHost->getStatus(currentAccount);
		if (jid_str != "offline") {
			// Отсылаем запросы об активности зеркал
			QStringList jids = mySender->getGameJids();
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
	if (mySender) {
		delete mySender;
		mySender = 0;
	}
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
	QStringList game_jids = mySender->getGameJids();
	gameJidsWid->setText(game_jids.join("\n"));
	QVBoxLayout *gameJidsVertLayout = new QVBoxLayout();
	gameJidsVertLayout->addWidget(new QLabel(QString::fromUtf8("JID-ы игры:"), optionsWid));
	gameJidsVertLayout->addWidget(gameJidsWid);
	// Список Jid-ов чата
	chatJidsWid = new QTextEdit(optionsWid);
	chatJidsWid->setText(chatJids.join("\n"));
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
	shortCutWid->setText(shortCut);
	shortCutWid->setDisabled(true);
	hotkeyHorLayout->addWidget(shortCutWid,200,Qt::AlignLeft);
	QPushButton *shortCutButton = new QPushButton(optionsWid);
	shortCutButton->setText(QString::fromUtf8("Изменить"));
	connect(shortCutButton, SIGNAL(released()), SLOT(selectShortCut()));
	hotkeyHorLayout->addWidget(shortCutButton,400,Qt::AlignLeft);
	//--
	layout->addLayout(hotkeyHorLayout);
	// Получаем список аккаунтов текущего профиля
	QStringList accNames;
	QStringList accJids;
	quint32 accsCount = getAccounts(&accNames, &accJids);
	// Формируем элементы для списка аккаунтов
	QHBoxLayout *accountHorLayout = new QHBoxLayout;
	accountHorLayout->addWidget(new QLabel(QString::fromUtf8("Игровой аккаунт:")), 0, Qt::AlignLeft);
	accountsWid = new QComboBox(optionsWid);
	for (quint32 i = 0; i < accsCount; i++) {
		accountsWid->insertItem(i, accNames[i] + " (" + accJids[i] + ")", accJids[i]);
		if (accJids[i] == accountJid) {
			accountsWid->setCurrentIndex(i);
		}
	}
	accountHorLayout->addWidget(accountsWid, 400, Qt::AlignLeft);
	// Ссылка на сайт игры
	QLabel *gameSiteLink = new QLabel(tr("<a href=\"http://www.jabbergames.ru/\">SoF game site (Online)</a>"), optionsWid);
	gameSiteLink->setOpenExternalLinks(true);
	layout->addLayout(accountHorLayout);
	layout->addWidget(gameSiteLink, 400, Qt::AlignLeft);
	//--
	return optionsWid;
}

void SofGamePlugin::applyOptions() {
	if (!gameJidsWid || !chatJidsWid || !shortCutWid || !accountsWid) {
		return;
	}
	// Аккаунт игры
	qint32 index = accountsWid->currentIndex();
	QString newAccJid = "";
	if (index != -1) {
		newAccJid = accountsWid->itemData(index).toString();
	}
	QVariant vGameAccount(newAccJid);
	psiOptions->setPluginOption(constGameAccount, vGameAccount);
	int newAccount = getAccountByJid(newAccJid);
	if (currentAccount != newAccount || newAccJid != accountJid) {
		currentAccount = newAccount;
		accountJid = newAccJid;
		if (mySender) {
			mySender->changeAccount(currentAccount, accountJid);
		}
	}
	// *** Jid-ы игры ***
	QStringList game_jids = gameJidsWid->toPlainText().split(QRegExp("\\s+"), QString::SkipEmptyParts);
	QVariant vGameJids(game_jids);
	// Сохраняем в настройках
	psiOptions->setPluginOption(constGameJids, vGameJids);
	// Получаем старый набор джидов игры
	QStringList old_game_jids = mySender->getGameJids();
	// Производим изменения
	bool fActive = false;
	if (currentAccount != -1 && accInfoHost->getStatus(currentAccount) != "offline")
		fActive = true;
	while (!game_jids.isEmpty()) {
		QString jid = game_jids.takeFirst().trimmed();
		int i = old_game_jids.indexOf(jid);
		if (i != -1) {
			old_game_jids.removeAt(i);
		} else {
			mySender->insertGameJid(jid, -1);
			if (fActive)
				sendLastActiveQuery(accountJid, jid);
		}
	}
	while (!old_game_jids.isEmpty()) {
		QString jid = old_game_jids.takeFirst();
		mySender->removeGameJid(jid);
	}
	// *** Jid-ы чата ***
	chatJids = chatJidsWid->toPlainText().split(QRegExp("\\s+"), QString::SkipEmptyParts);
	QVariant vChatJids(chatJids);
	psiOptions->setPluginOption(constChatJids, vChatJids);
	// --
	QVariant vShortCut(shortCutWid->text());
	psiOptions->setPluginOption(constShortCut, vShortCut);
	psiShortcuts->disconnectShortcut(QKeySequence(shortCut), this, SLOT(doShortCut()));
	shortCut = vShortCut.toString();
	psiShortcuts->connectShortcut(QKeySequence(shortCut), this, SLOT(doShortCut()));
}

void SofGamePlugin::restoreOptions() {
	if (gameJidsWid == 0 || chatJidsWid == 0 || shortCutWid == 0 || accountsWid == 0) {
		return;
	}
	QStringList game_jids = mySender->getGameJids();
	QVariant vGameJids(game_jids);
	vGameJids = psiOptions->getPluginOption(constGameJids);
	if (!vGameJids.isNull()) {
		game_jids = vGameJids.toStringList();
	}
	gameJidsWid->setText(game_jids.join("\n"));
	// --
	QVariant vChatJids(chatJids);
	vChatJids = psiOptions->getPluginOption(constChatJids);
	if (!vChatJids.isNull()) {
	  chatJids = vChatJids.toStringList();
	}
	chatJidsWid->setText(chatJids.join("\n"));
	// --
	QVariant vShortCut(shortCut);
	vShortCut = psiOptions->getPluginOption(constShortCut);
	if (!vShortCut.isNull()) {
	  shortCutWid->setText(vShortCut.toString());
	} else {
	  shortCutWid->setText(shortCut);
	}
	// Выбранный аккаунт
	qint32 index = accountsWid->findData(accountJid);
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

quint32 SofGamePlugin::getAccounts(QStringList* accNamesPtr, QStringList* accJidsPtr)
{
	quint32 cnt = 0;
	for (int i = 0; ; i++) {
		QString jid = accInfoHost->getJid(i);
		if (jid == "-1")
			break;
		if (!jid.isEmpty()) {
			++cnt;
			QString name = accInfoHost->getName(i);
			if (name.isEmpty()) {
				name = QString("acc %1").arg(cnt);
			}
			accNamesPtr->push_back(name);
			accJidsPtr->push_back(jid);
		}
	}
	return cnt;
}

int SofGamePlugin::getAccountByJid(QString jid)
{
	if (!jid.isEmpty()) {
		// Перебираем аккаунты и находим свой
		for (int i = 0; ; i++) {
			// Проверяем аккаунт
			QString jid_str = accInfoHost->getJid(i);
			if (jid_str == "-1")
				return -1;
			if (jid == jid_str)
				return i;
		}
	}
	return -1;
}

void SofGamePlugin::sendLastActiveQuery(QString from_jid, QString to_jid)
{
	if (currentAccount != -1) {
		QString query = "<iq from=\"" + from_jid + "\" id=\"sofgame_plugin\" to=\"";
		query.append(to_jid);
		query.append("\" type=\"get\"><query xmlns=\"jabber:iq:last\"/></iq>");
		sender_->sendStanza(currentAccount, query);
	}
}


//-- OptionAccessor -------------------------------------------------

void SofGamePlugin::setOptionAccessingHost(OptionAccessingHost* host)
{
	psiOptions = host;
}

void SofGamePlugin::optionChanged(const QString& option)
{
	Q_UNUSED(option);
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
	appInfoHost = host;
}

// ----------------- AccountInfoAccessor -------------------
void SofGamePlugin::setAccountInfoAccessingHost(AccountInfoAccessingHost* host) {
	accInfoHost = host;
}

// ----------------- PopupAccessor -------------------
void SofGamePlugin::setPopupAccessingHost(PopupAccessingHost* host) {
	//qDebug() << "SofGamePlugin::setPopupAccessingHost";
	myPopupHost = host;
}

// ----------------------- StanzaFilter ------------------------------
bool SofGamePlugin::incomingStanza(int account, const QDomElement &stanza)
{
	if (!enabled)
		return false;
	if (currentAccount == -1) {
		// Проверяем JID, от которого пришло сообщение
		if (accountJid != accInfoHost->getJid(account))
			return false;
		// Сохраняем id аккаунта
		currentAccount = account;
		if (mySender)
			mySender->changeAccount(account, accountJid);
	} else if (currentAccount != account) {
		return false;
	}
	// Проверяем JID от которого пришло сообщение
	QString jid = stanza.attribute("from");
	jid = jid.left(jid.indexOf("/")).toLower();
	int i = mySender->getGameJidIndex(jid);
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
		bool res = mySender->doGameAsk(&jid, &message);
		//bool res = pluginCore->textParsing(jid, message);
		return res;
	} else if (sTagName == "presence") {
		if (stanza.attribute("type") == "unavailable") {
			PluginCore::instance()->setGameJidStatus(i, 0);
		} else {
			PluginCore::instance()->setGameJidStatus(i, 1);
		}
	} else if (sTagName == "iq") {
		if (stanza.attribute("type") == "result" && stanza.attribute("id") == "sofgame_plugin") {
			// Это ответ на запрос который посылался при загрузке плагина
			QDomNode queryNode = stanza.firstChild();
			bool fFound = false;
			while (!queryNode.isNull()) {
				QDomElement queryElement = queryNode.toElement();
				if (queryElement.tagName() == "query") {
					if (queryElement.attribute("xmlns") == "jabber:iq:last") {
						if (queryElement.attribute("seconds").toInt() > 0) {
							fFound = true;
						}
					}
					break;
				}
				queryNode = queryNode.nextSibling();
			}
			if (fFound) {
				PluginCore::instance()->setGameJidStatus(i, 1);
			} else {
				PluginCore::instance()->setGameJidStatus(i, 0);
			}
		}
	}
	return false;
}

bool SofGamePlugin::outgoingStanza(int/* account*/, QDomElement&/* stanza*/)
{
	return false;
}

//-- private slots --------------------------------------------------

void SofGamePlugin::doShortCut()
{
	if (enabled) {
		PluginCore::instance()->doShortCut();
	}
}
