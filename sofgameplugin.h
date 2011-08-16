/*
 * sof_game.h - Sof Game Psi plugin
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

#ifndef PLUGIN_MAIN_H
#define PLUGIN_MAIN_H

#include <QtGui>
//#include <QtCore>
#include "psiplugin.h"
#include "optionaccessor.h"
//#include "optionaccessinghost.h"
#include "shortcutaccessor.h"
#include "shortcutaccessinghost.h"
#include "stanzasender.h"
#include "stanzasendinghost.h"
#include "stanzafilter.h"
#include "plugininfoprovider.h"
#include "accountinfoaccessor.h"
#include "accountinfoaccessinghost.h"
#include "applicationinfoaccessor.h"
#include "applicationinfoaccessinghost.h"
#include "popupaccessor.h"
#include "popupaccessinghost.h"

#define constGameJids "game-jids"
#define constChatJids "chat-jids"
#define constShortCut "shortcut"
#define constGameAccount "game-account"

class SofGamePlugin: public QObject, public PsiPlugin, public OptionAccessor, public StanzaSender, public ShortcutAccessor, public StanzaFilter, public ApplicationInfoAccessor, public AccountInfoAccessor, public PopupAccessor, public PluginInfoProvider

{
	Q_OBJECT
	Q_INTERFACES(PsiPlugin OptionAccessor StanzaSender ShortcutAccessor StanzaFilter ApplicationInfoAccessor AccountInfoAccessor PopupAccessor PluginInfoProvider)

public:
	SofGamePlugin();
	~SofGamePlugin();
	virtual QString name() const;
	virtual QString shortName() const;
	virtual QString version() const;
	virtual bool enable();
	virtual bool disable();
	virtual QWidget* options();
	virtual void applyOptions();
	virtual void restoreOptions();
	virtual QString pluginInfo();
	// OptionAccessor
	virtual void setOptionAccessingHost(OptionAccessingHost*);
	virtual void optionChanged(const QString&);
	// StanzaSender
	virtual void setStanzaSendingHost(StanzaSendingHost*);
	// ShortcutsAccessor
	virtual void setShortcutAccessingHost(ShortcutAccessingHost*);
	virtual void setShortcuts();
	// ApplicationInfoAccessor
	virtual void setApplicationInfoAccessingHost(ApplicationInfoAccessingHost*);
	// AccountInfoAccessor
	virtual void setAccountInfoAccessingHost(AccountInfoAccessingHost*);
	// StanzaFilter
	virtual bool incomingStanza(int, const QDomElement&);
	virtual bool outgoingStanza(int, QDomElement&);
	// PopupAccessor
	virtual void setPopupAccessingHost(PopupAccessingHost* host);
	//---

private:
	bool enabled;
	int  currentAccount;
	bool currAccActive;
	QString accountJid;
	QString shortCut;
	QTextEdit* gameJidsWid;
	QTextEdit* chatJidsWid;
	QLineEdit* shortCutWid;
	QComboBox* accountsWid;
	//StanzaSendingHost* sender;
	ShortcutAccessingHost* psiShortcuts;
	AccountInfoAccessingHost *accInfoHost;
	//PopupAccessingHost* myPopupHost;

private:
	QList< QPair<QString, QString> > getAccounts() const;
	int getAccountByJid(const QString &) const;
	void sendLastActiveQuery(const QString &, const QString &);

private slots:
	void doShortCut();
	void selectShortCut();
	void onNewShortcutKey(QKeySequence ks);

};

//Q_EXPORT_PLUGIN(SofGamePlugin);

#endif
