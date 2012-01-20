/*
 * textview.cpp - Sof Game Psi plugin
 * Copyright (C) 2011  Aleksey Andreev
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

#include <QTextDocumentFragment>
#include <QTime>
#include <QIcon>
#include <QPixmap>
#include <QScrollBar>
#include <QMenu>
#include <QContextMenuEvent>

#include "textview.h"
#include "pluginhosts.h"
#include "pers.h"

TextView::TextView(QWidget *parent) :
	QTextEdit(parent),
	maxBlockActive(true),
	maxBlockCount(0)
{
	setUndoRedoEnabled(false);
	setLogIcons();
	connect(this, SIGNAL(textChanged()), this, SLOT(firstLoadResources()));
}

void TextView::changeEvent(QEvent *e)
{
	if (e->type() == QEvent::FontChange) {
		document()->setDefaultFont(font());
		setLogIcons();
	}
}

QMimeData *TextView::createMimeDataFromSelection() const
{
	QTextDocument *doc = new QTextDocument();
	QTextCursor cursor(doc);
	cursor.insertFragment(textCursor().selection());
	QString text = convertToPlainText(doc);
	delete doc;
	QMimeData *data = new QMimeData();
	data->setText(text);
	data->setHtml(Qt::convertFromPlainText(text));
	return data;
}

void TextView::contextMenuEvent(QContextMenuEvent *e)
{
	QMenu *menu = createStandardContextMenu();
	menu->addSeparator();
	menu->addAction(QString::fromUtf8("Очистить"), this, SLOT(clear()));
	menu->exec(e->globalPos());
	delete menu;
}

void TextView::appendText(const QString &text, TextType type)
{
	bool doScrollToBottom = atBottom();
	// prevent scrolling back to selected text when
	// restoring selection
	int scrollbarValue = verticalScrollBar()->value();

	// Формируем строку с временем и типом сообщения
	QTextCursor cursor = textCursor();
	// Сохраняем выделение
	int startSelection = -1;
	int endSelection = -1;
	if (cursor.hasSelection()) {
		startSelection = cursor.selectionStart();
		endSelection = cursor.selectionEnd();
	}
	//---
	cursor.beginEditBlock();
	cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
	cursor.clearSelection();
	if (!cursor.atBlockStart()) {
		cursor.insertBlock();
		QTextBlockFormat blockFormat = cursor.blockFormat();
		blockFormat.clearProperty(QTextFormat::BlockTrailingHorizontalRulerWidth);
		cursor.setBlockFormat(blockFormat);
	}
	// Вставка текста
	int cursorPos = cursor.position();
	cursor.insertFragment(QTextDocumentFragment::fromHtml(logTimeString(type) + ((type == LocalText)?QString():"<br>") + text));
	cursor.setPosition(cursorPos);
	//--
	cursor.endEditBlock();
	// Восстанавливаем выделение
	if (startSelection != -1 && endSelection != -1) {
		cursor.setPosition(startSelection, QTextCursor::MoveAnchor);
		cursor.setPosition(endSelection, QTextCursor::KeepAnchor);
	}
	setTextCursor(cursor);

	if (doScrollToBottom || type == LocalText) {
		if (!maxBlockActive) {
			maxBlockActive = true;
			document()->setMaximumBlockCount(maxBlockCount);
		}
		scrollToBottom();
	} else {
		if (maxBlockActive) {
			maxBlockActive = false;
			document()->setMaximumBlockCount(0);
		}
		verticalScrollBar()->setValue(scrollbarValue);
	}
}

void TextView::setMaximumBlockCount(int n)
{
	maxBlockCount = n;
	if (maxBlockActive)
		document()->setMaximumBlockCount(n);
}

QString TextView::logTimeString(TextType type)
{
	QString iconString;
	QString nickString;
	if (type == LocalText) {
		iconString = "<span><img src=\"icon:log-icon-sent\" /><font color=\"red\">";
		nickString = QString::fromUtf8("&lt;%1&gt; ").arg(Pers::instance()->name());
	} else if (type == GameText) {
		iconString = "<span><img src=\"icon:log-icon-receive\" /><font color=\"blue\">";
		nickString = QString::fromUtf8("&lt;Игра&gt; ");
	} else {
		iconString = "<span><img src=\"icon:log-icon-info\" /><font color=\"green\">";
		nickString = QString::fromUtf8("&lt;Плагин&gt; ");
	}
	iconString.append("[" + QTime::currentTime().toString("hh:mm:ss") + "] ");
	iconString.append(nickString + "</font></span>");
	return iconString;
}

void TextView::setLogIcons()
{
	int logIconsSize = QFontInfo(document()->defaultFont()).pixelSize() * 0.93;
	QIcon sentIcon = PluginHosts::psiIcon->getIcon("psi/notification_chat_delivery_ok");
	QPixmap sentPixmap = sentIcon.pixmap(sentIcon.availableSizes().first()).scaledToHeight(logIconsSize, Qt::SmoothTransformation);
	QIcon receiveIcon = PluginHosts::psiIcon->getIcon("psi/notification_chat_receive");
	QPixmap receivePixmap = receiveIcon.pixmap(receiveIcon.availableSizes().first()).scaledToHeight(logIconsSize, Qt::SmoothTransformation);
	QIcon infoIcon = PluginHosts::psiIcon->getIcon("psi/notification_chat_info");
	QPixmap infoPixmap = infoIcon.pixmap(infoIcon.availableSizes().first()).scaledToHeight(logIconsSize, Qt::SmoothTransformation);
	document()->addResource(QTextDocument::ImageResource, QUrl("icon:log-icon-sent"), sentPixmap);
	document()->addResource(QTextDocument::ImageResource, QUrl("icon:log-icon-receive"), receivePixmap);
	document()->addResource(QTextDocument::ImageResource, QUrl("icon:log-icon-info"), infoPixmap);
}

void TextView::firstLoadResources()
{
	if (document()->isEmpty())
		setLogIcons();
}

QString TextView::convertToPlainText(const QTextDocument *doc) const
{
	QString obrepl = QString(QChar::ObjectReplacementCharacter);
	QString text = doc->toPlainText();
	text.replace(obrepl, QString());
	return text;
}

/**
 * This function returns true if vertical scroll bar is
 * at its maximum position.
 */
bool TextView::atBottom() const
{
	// '32' is 32 pixels margin, which was used in the old code
	return (verticalScrollBar()->maximum() - verticalScrollBar()->value()) <= 32;
}

/**
 * Scrolls the vertical scroll bar to its maximum position i.e. to the bottom.
 */
void TextView::scrollToBottom()
{
	verticalScrollBar()->setValue(verticalScrollBar()->maximum());
}
