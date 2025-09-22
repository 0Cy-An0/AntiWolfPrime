#include "CircleWidget.h"
#include <QWidget>
#include <QPixmap>
#include <QColor>
#include <QPainter>

CircleWidget::CircleWidget(QWidget *parent)
    : QWidget(parent), m_count(-1), m_bgColor(Qt::transparent)
{
    m_penColor = Qt::white;
    // Makes widget background transparent and lets mouse events pass through
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_TranslucentBackground);
    //hope this forces same height and width
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
}

void CircleWidget::setCount(int count) {
    if (m_count != count) {
        m_count = count;
        update();
        setVisible(m_count > 0 || !m_icon.isNull());
    }
}

void CircleWidget::setIcon(const QPixmap& icon) {
    m_icon = icon;
    if (!m_icon.isNull()) {
        m_count = -1;
    }
    update();
    setVisible(!m_icon.isNull() || m_count > 0);
}

void CircleWidget::setColor(const QColor& color) {
    m_bgColor = color;
    update();
}

void CircleWidget::setPen(const QColor& color) {
    m_penColor = color;
    update();
}

QString CircleWidget::abbreviateNumber(const int number) {
    if (number < 1000)
        return QString::number(number);

    const char* suffixes[] = {"K", "M", "B", "T"}; //should be integer capped but just to be sure
    double value = number;
    int suffixIndex = -1;

    while (value >= 1000 && suffixIndex < 3) {
        value /= 1000.0;
        ++suffixIndex;
    }


    // precision of 0 decimals if value >= 10, else 1 decimal
    int precision = (value >= 10) ? 0 : 1;

    // Explicit rounding
    double factor = std::pow(10, precision);
    value = std::round(value * factor) / factor;

    return QString::number(value, 'f', precision) + suffixes[suffixIndex];
}

void CircleWidget::paintEvent(QPaintEvent* /*event*/) {
    if (m_count < 0 && m_icon.isNull()) {
        return; // nothing to paint
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.setBrush(m_bgColor);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(rect());

    if (m_count > 0) {
        QString abbrev = abbreviateNumber(m_count);
        int length = abbrev.length();

        qreal fontFactor;

        switch (length) {
            case 1: fontFactor = 0.6;   break;
            case 2: fontFactor = 0.45;  break;
            case 3: fontFactor = 0.35;  break;
            case 4:
            default: fontFactor = 0.275; break;
        }

        painter.setPen(m_penColor);
        QFont font = painter.font();
        font.setBold(true);
        font.setPointSizeF(height() * fontFactor);
        painter.setFont(font);

        painter.drawText(rect(), Qt::AlignCenter, abbrev);
    } else if (!m_icon.isNull()) {
        QSize iconSize = m_icon.size();
        QSize targetSize = size() * 0.7;
        iconSize.scale(targetSize, Qt::KeepAspectRatio);

        QPoint iconTopLeft((width() - iconSize.width()) / 2, (height() - iconSize.height()) / 2);
        QRect iconRect(iconTopLeft, iconSize);

        painter.drawPixmap(iconRect, m_icon);
    }
}

void CircleWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    int side = qMin(width(), height());
    setFixedSize(side, side);
}

