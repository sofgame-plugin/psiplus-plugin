
#include "fontlabel.h"


//----------------------------------------------------------------------------
// FontLabel
//----------------------------------------------------------------------------

FontLabel::FontLabel(QWidget* parent) : QLineEdit(parent)
{
	setReadOnly(true);

	QPalette palette = this->palette();
	palette.setColor(backgroundRole(), parent->palette().color(parent->backgroundRole()));
	setPalette(palette);

	m_defaultHeight = QLineEdit::sizeHint().height();
}

void FontLabel::setFont(QString fontName)
{
	QFont f;
	f.fromString(fontName);
	m_font = fontName;
	setText( tr("%1 %2").arg( f.family() ).arg( f.pointSize() ) );
	QLineEdit::setFont(f);
}

QString FontLabel::fontName() const
{
	return m_font;
}

QSize FontLabel::sizeHint() const
{
	return QSize(QLineEdit::sizeHint().width(), m_defaultHeight);
}
