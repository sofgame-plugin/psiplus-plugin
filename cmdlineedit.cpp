/*
 * cmdlineedit.cpp - Sof Game Psi plugin
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

#include <QTimer>
#include <QAbstractTextDocumentLayout>

#include "cmdlineedit.h"

#define MAX_MESSAGE_HISTORY 50

CmdLineEdit::CmdLineEdit(QWidget *parent) :
	QTextEdit(parent),
	typedMsgsIndex(0)
{
	setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere); // no need for horizontal scrollbar with this
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setMinimumHeight(0);

	connect(this, SIGNAL(textChanged()), SLOT(recalculateSize()));

	initAction();
}

void CmdLineEdit::initAction()
{
	actShowMessagePrev = new QAction(this);
	actShowMessagePrev->setShortcut(QKeySequence("Ctrl+Up"));
	connect(actShowMessagePrev, SIGNAL(triggered()), this, SLOT(showHistoryMessagePrev()));
	addAction(actShowMessagePrev);
	actShowMessageNext = new QAction(this);
	actShowMessageNext->setShortcut(QKeySequence("Ctrl+Down"));
	connect(actShowMessageNext, SIGNAL(triggered()), this, SLOT(showHistoryMessageNext()));
	addAction(actShowMessageNext);
}

void CmdLineEdit::showHistoryMessagePrev()
{
	if (!typedMsgsHistory.isEmpty() && typedMsgsIndex > 0) {
		if (typedMsgsIndex == typedMsgsHistory.size())
			currentText = toPlainText();
		--typedMsgsIndex;
		showMessageHistory();
	}
}

void CmdLineEdit::showHistoryMessageNext()
{
	int sz = typedMsgsHistory.size();
	if (sz != 0) {
		if (typedMsgsIndex + 1 < sz) {
			++typedMsgsIndex;
			showMessageHistory();
		} else {
			if (typedMsgsIndex != sz) {
				typedMsgsIndex = sz;
				setEditText(currentText);
			}
		}
	}
}

void CmdLineEdit::showMessageHistory()
{
	setEditText(typedMsgsHistory.at(typedMsgsIndex));
}

void CmdLineEdit::setEditText(const QString &text)
{
	setPlainText(text);
	moveCursor(QTextCursor::End);
}

void CmdLineEdit::appendMessageHistory(const QString &text)
{
	if (text.simplified().length() > 2) {
		if (currentText == text)
			// Remove current typed text only if we want to add it to history
			currentText.clear();
		int index = typedMsgsHistory.indexOf(text);
		if (index >= 0) {
			typedMsgsHistory.removeAt(index);
		} else {
			if (typedMsgsHistory.size() >= MAX_MESSAGE_HISTORY)
				typedMsgsHistory.removeFirst();
		}
		typedMsgsHistory.append(text);
		typedMsgsIndex = typedMsgsHistory.size();
	}
}

QSize CmdLineEdit::minimumSizeHint() const
{
	QSize sh = QTextEdit::minimumSizeHint();
	sh.setHeight(fontMetrics().height() + 1);
	sh += QSize(0, QFrame::lineWidth() * 2);
	return sh;
}

QSize CmdLineEdit::sizeHint() const
{
	QSize sh = QTextEdit::sizeHint();
	sh.setHeight(int(document()->documentLayout()->documentSize().height()));
	sh += QSize(0, QFrame::lineWidth() * 2);
	((QTextEdit*)this)->setMaximumHeight(sh.height());
	return sh;
}

bool CmdLineEdit::focusNextPrevChild(bool next)
{
	return QWidget::focusNextPrevChild(next);
}

void CmdLineEdit::resizeEvent(QResizeEvent* e)
{
	QTextEdit::resizeEvent(e);
	QTimer::singleShot(0, this, SLOT(updateScrollBar()));
}

void CmdLineEdit::keyPressEvent(QKeyEvent *e)
{
	if ((e->modifiers() == Qt::NoModifier && e->key() == Qt::Key_Return) ||
	    (e->modifiers() == Qt::KeypadModifier && e->key() == Qt::Key_Enter)) {
		emit returnPressed();
		return;
	}
	QTextEdit::keyPressEvent(e);
}

// Qt text controls are quite greedy to grab key events.
// disable that.
bool CmdLineEdit::event(QEvent * event) {
	if (event->type() == QEvent::ShortcutOverride) {
		return false;
	}
	return QTextEdit::event(event);
}

void CmdLineEdit::recalculateSize()
{
	updateGeometry();
	QTimer::singleShot(0, this, SLOT(updateScrollBar()));
}

void CmdLineEdit::updateScrollBar()
{
	setVerticalScrollBarPolicy(sizeHint().height() > height() ? Qt::ScrollBarAlwaysOn : Qt::ScrollBarAlwaysOff);
	ensureCursorVisible();
}

