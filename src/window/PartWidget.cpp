#include "PartWidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "FileAccess/FileAccess.h"

PartWidget::PartWidget(const QPixmap &icon, const IData &itemData, QWidget *parent)
    : QWidget(parent),
      m_icon(icon),
      m_itemData(&itemData)
{
    // Main layout: Vertical - Image + Label
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(5); // Spacing between image and label
    mainLayout->setAlignment(Qt::AlignCenter);

    // -------- Image Container --------
    QWidget *imageContainer = new QWidget(this);
    imageContainer->setStyleSheet("background-color: transparent;");
    imageContainer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    imageContainer->setMinimumSize(100, 100); // Placeholder; updated on resize

    m_iconCircle = new CircleWidget(imageContainer);
    m_iconCircle->setCount(-1);

    // Two count circles inside the image container
    m_craftedCircle = new CircleWidget(imageContainer);
    m_craftedCircle->setColor(Qt::green);

    m_blueprintCircle = new CircleWidget(imageContainer);
    m_blueprintCircle->setColor(Qt::blue);

    mainLayout->addWidget(imageContainer);

    // -------- Label --------
    iconLabel = new QLabel(this);
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setStyleSheet("background-color: transparent;");
    iconLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    mainLayout->addWidget(iconLabel);

    updateLabelFromName(itemData.getName());
    updateIcon(imageContainer->width());
    updateCountCircles();
}

void PartWidget::updateLabelFromName(const std::string& name) {
    QString text = QString::fromStdString(name);
    QStringList words = text.split(' ');
    QString iconWord;
    int charCount = 0;
    const int maxChars = 10;

    for (const QString& word : words) {
        int wordLength = word.length();
        if (charCount + wordLength > maxChars && !iconWord.isEmpty()) {
            iconWord += '\n';
            charCount = 0;
        } else if (!iconWord.isEmpty()) {
            iconWord += ' ';
        }
        iconWord += word;
        charCount += wordLength;
    }

    iconLabel->setText(iconWord);
}

void PartWidget::updateCountCircles() {
    if (!m_itemData || !m_iconCircle)
        return;

    int size = m_iconCircle->width();
    int countDiameter = int(size * 0.4);
    int margin = int(size * 0.05);

    const auto& counts = m_itemData->getPossessionCounts();

    m_craftedCircle->setCount(counts.count(ItemPossessionType::Crafted) ? counts.at(ItemPossessionType::Crafted) : 0);
    m_craftedCircle->setPen(Qt::black);
    m_craftedCircle->resize(countDiameter, countDiameter);
    m_craftedCircle->move(size - margin - countDiameter, margin);

    m_blueprintCircle->setCount(counts.count(ItemPossessionType::Blueprint) ? counts.at(ItemPossessionType::Blueprint) : 0);
    m_blueprintCircle->resize(countDiameter, countDiameter);
    m_blueprintCircle->move(size - margin - countDiameter, size - margin - countDiameter);
}

void PartWidget::updateIcon(int size) {
    if (size <= 0) return;

    if (m_icon.isNull()) {
        m_iconCircle->hide();
        return;
    }

    m_iconCircle->show();
    m_iconCircle->setFixedSize(size, size);
    m_iconCircle->move(0, 0); // position inside its parent (imageContainer)
    m_iconCircle->lower(); // keep it at the back
    QPixmap scaledIcon = m_icon.scaled(size, size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    m_iconCircle->setIcon(scaledIcon);

    updateCountCircles();
}

void PartWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    int iconSize = std::min(width(), height() - iconLabel->height());
    m_iconCircle->setFixedSize(iconSize, iconSize);
    m_iconCircle->parentWidget()->setFixedSize(iconSize, iconSize);
    updateIcon(iconSize);
}

void PartWidget::setPixmap(const QPixmap &pixmap) {
    m_icon = pixmap;
    updateIcon(m_iconCircle->width());
}

void PartWidget::setItemData(const IData& itemData) {
    m_itemData = &itemData;
    updateLabelFromName(itemData.getName());
    updateCountCircles();
}

void PartWidget::reset() {
    m_icon = QPixmap();
    m_itemData = nullptr;
    m_iconCircle->hide();
    m_craftedCircle->setCount(0);
    m_blueprintCircle->setCount(0);
    iconLabel->clear();
}
