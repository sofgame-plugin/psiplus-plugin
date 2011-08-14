#ifndef FONTLABEL_H
#define FONTLABEL_H

#include <qlineedit.h>

class FontLabel : public QLineEdit
{
	Q_OBJECT
public:
	FontLabel(QWidget *parent);

	void setFont(QString);
	QString fontName() const;

	QSize sizeHint() const;

private:
	QString m_font;
	int m_defaultHeight;
};

#endif
