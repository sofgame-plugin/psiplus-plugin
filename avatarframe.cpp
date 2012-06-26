/*
 * avatarframe.cpp - Sof Game Psi plugin
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

#include <QPixmap>
#include <QMenu>
#include <QFileDialog>
#include <QMessageBox>

#include "avatarframe.h"
#include "pluginhosts.h"
#include "pers.h"
#include "plugin_core.h"

AvatarFrameView::AvatarFrameView(QWidget *parent) :
	QGraphicsView(parent),
	avatarItem(NULL),
	pluginInfoItem(NULL)
{
	scene_ = new QGraphicsScene(this);
	setScene(scene_);
	connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu()));
}

QString AvatarFrameView::avatarFilePath()
{
	QString name = Pers::instance()->name();
	name.replace(QRegExp("[^\\w]+"), "-");
	QString avaFilePath = PluginHosts::appInfoHost->appHomeDir(ApplicationInfoAccessingHost::DataLocation) + QDir::separator()
		+ "sof_game" + QDir::separator() + "avatar-" + name.toLower() + ".png";
	return avaFilePath;
}

void AvatarFrameView::clear()
{
	avatarItem = NULL;
	pluginInfoItem = NULL;
	scene_->clear();
	setSceneRect(QRectF());
}

bool AvatarFrameView::updateAvatar()
{
	QPixmap pix;
	if (pix.load(AvatarFrameView::avatarFilePath())) {
		if (pix.height() <= height() && pix.width() <= width()) {
			clear();
			QPointF newPos(0.0f, 0.0f);
			if (pix.width() < width())
				newPos.setX((width() - pix.width()) / 2.0f);
			if (pix.height() < height())
				newPos.setY((height() - pix.height()) / 2.0f);
			avatarItem = scene_->addPixmap(pix);
			avatarItem->setPos(newPos);
			setSceneRect(0.0f, 0.0f, width(), height());
			return true;
		}
	}
	return false;
}

void AvatarFrameView::showPluginInfo()
{
	clear();
	pluginInfoItem = scene_->addText(QString::fromUtf8("SofGame плагин\nВерсия: %1").arg(cVer));
}

void AvatarFrameView::showContextMenu()
{
	QMenu *menu = new QMenu();
	QAction *actLoadAvatar = new QAction(QString::fromUtf8("Загрузить аватар"), menu);
	menu->addAction(actLoadAvatar);
	QAction *actClearAvatar = new QAction(QString::fromUtf8("Очистить аватар"), menu);
	menu->addAction(actClearAvatar);
	QAction *res = menu->exec(QCursor::pos());
	if (res != NULL) {
		if (res == actLoadAvatar) {
			loadAvatar();
		} else if (res == actClearAvatar) {
			clearAvatar();
		}
	}
	delete menu;
}

void AvatarFrameView::loadAvatar()
{
	QString fileName = QFileDialog::getOpenFileName(this, QString::fromUtf8("Выберите файл с аватарой"), QString(),
		QString::fromUtf8("Файлы картинок (*.bmp *.gif *.jpg *.jpeg *.png *.pbm *.pgm *.ppm *.xbm *.xpm)"));
	if (!fileName.isEmpty()) {
		QPixmap newAva;
		if (newAva.load(fileName)) {
			QRect avaSize = newAva.rect();
			QSize viewSize = size();
			if (avaSize.height() > viewSize.height() || avaSize.width() > viewSize.width()) {
				newAva = newAva.scaled(viewSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
			}
			if (newAva.save(AvatarFrameView::avatarFilePath(), "PNG", -1)) {
				updateAvatar();
				return;
			}
		}
		QMessageBox::warning(this, QString::fromUtf8("Загрузка аватара"), QString::fromUtf8("Ошибка загрузки аватара"));
	}
}

void AvatarFrameView::clearAvatar()
{
	if (QFile::remove(AvatarFrameView::avatarFilePath()))
		clear();
}
