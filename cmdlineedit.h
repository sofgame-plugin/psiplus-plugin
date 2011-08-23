/*
 * cmdlineedit.h - Sof Game Psi plugin
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

#ifndef CMDLINEEDIT_H
#define CMDLINEEDIT_H

#include <QTextEdit>
#include <QAction>

class CmdLineEdit : public QTextEdit
{
Q_OBJECT
public:
	CmdLineEdit(QWidget *parent = 0);

	// reimplemented
	QSize minimumSizeHint() const;
	QSize sizeHint() const;

public slots:
	void appendMessageHistory(const QString& text);

private:
	int typedMsgsIndex;
	QAction *actShowMessagePrev;
	QAction *actShowMessageNext;
	QStringList typedMsgsHistory;
	QString currentText;

private:
	void initAction();

protected:
	void showMessageHistory();
	void setEditText(const QString &text);

	// reimplemented
	bool focusNextPrevChild(bool next);
	void resizeEvent(QResizeEvent*);
	void keyPressEvent(QKeyEvent *);
	bool event(QEvent * event);

private slots:
	void recalculateSize();
	void updateScrollBar();

protected slots:
	void showHistoryMessagePrev();
	void showHistoryMessageNext();

signals:
	void returnPressed();

};

#endif // CMDLINEEDIT_H
