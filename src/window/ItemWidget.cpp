#define NOMINMAX
#include "ItemWidget.h"
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QPainter>
#include <QtConcurrent>
#include <QVBoxLayout>
#include <QImageReader>

#include "mainwindow.h"
#include "PartWidget.h"
#include "apiParser/apiParser.h"
#include "dataReader/dataReader.h"
#include "FileAccess/FileAccess.h"

ItemWidget::ItemWidget(IDataContainer* dataContainer,
                       QWidget *parent, QStringList texts)
    : QWidget(parent),
      m_texts(texts),
      rightHidden(!MainWindow::getWindowSettings().fullItems)
{
    setStyleSheet("background: #232323; border-radius: 12px;");

    // Layouts and child setup
    mainLayout = new QHBoxLayout();
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    setLayout(mainLayout);

    m_mainImgWidget = new QLabel();
    m_mainImgWidget->setAlignment(Qt::AlignCenter);
    m_mainImgWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_mainImgWidget->setMaximumHeight(180);
    m_mainImgWidget->setMinimumSize(50, 50);

    // Create a container overlay layout on top of the image
    QWidget* overlayContainer = new QWidget(m_mainImgWidget);
    overlayContainer->setAttribute(Qt::WA_TransparentForMouseEvents);
    overlayContainer->setStyleSheet("background: transparent;");
    overlayContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    overlayContainer->raise(); // Ensure above the image
    overlayContainer->resize(m_mainImgWidget->size());
    overlayContainer->move(0, 0);

    QHBoxLayout* overlayLayout = new QHBoxLayout(overlayContainer);
    overlayLayout->setContentsMargins(0, 0, 0, 0);
    overlayLayout->setSpacing(0);

    m_overlayGeneral = new QWidget(m_mainImgWidget);
    m_overlayGeneral->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_overlayGeneral->setStyleSheet("background: transparent;");
    m_overlayGeneral->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_overlayGeneral->raise(); // ensure it's above image

    m_overlayPossession = new QWidget(m_mainImgWidget);
    m_overlayPossession->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_overlayPossession->setStyleSheet("background: transparent;");
    m_overlayPossession->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_overlayPossession->raise();

    possessionLayout = new QVBoxLayout(m_overlayPossession);
    possessionLayout->setContentsMargins(0, 0, 0, 0);
    possessionLayout->setSpacing(4);

    overlayLayout->addWidget(m_overlayPossession, 1);
    overlayLayout->addWidget(m_overlayGeneral, 10);

    leftHalf = new QFrame(this);
    leftHalf->setFrameShape(QFrame::Box);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftHalf);
    leftLayout->addWidget(m_mainImgWidget, 1);

    mainItemLabel = new QLabel(this);
    mainItemLabel->setAlignment(Qt::AlignCenter);
    leftLayout->addWidget(mainItemLabel, 0);

    if (rightHidden) {
        leftHalf->setMaximumSize(300, 250);
    }
    else {
        leftHalf->setMaximumSize(250, 250);
    }
    leftHalf->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    mainLayout->addWidget(leftHalf, 1);
    createRightHalf();
    if (rightHidden) {
        rightHalf->setMaximumSize(0, 0);
        rightHalf->hide();
    }
    // Load the arbitration unlock image
    auto unlockImgBytesPtr = fetchUrlCached(imgFromId("/Lotus/Types/Items/UnlockArbitrationKeyItem"), FetchType::PNG);
    QByteArray unlockImgBytes(reinterpret_cast<const char*>(unlockImgBytesPtr->data()), unlockImgBytesPtr->size());
    QPixmap unlockImg;
    unlockImg.loadFromData(unlockImgBytes);

    unlockIconLabel = new QLabel(leftHalf);
    unlockIconLabel->setScaledContents(true);
    unlockIconLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    unlockIconLabel->setAttribute(Qt::WA_TranslucentBackground);
    unlockIconLabel->setPixmap(unlockImg);
    unlockIconLabel->hide();

    // Label initialization
    for (const auto& text : texts) {
        QWidget* itemWidget = new QWidget(m_overlayPossession);
        QVBoxLayout* vLayout = new QVBoxLayout(itemWidget);
        vLayout->setContentsMargins(0, 0, 0, 0);
        vLayout->setSpacing(2);

        CircleWidget* circle = new CircleWidget(itemWidget);
        circle->setColor(Qt::black);
        circle->setCount(0);
        vLayout->addWidget(circle);

        QLabel* label = new QLabel(text, itemWidget);
        label->setAlignment(Qt::AlignHCenter);
        label->setStyleSheet("background: transparent;");
        label->setVisible(true);
        label->setMinimumHeight(14);
        vLayout->addWidget(label);

        itemWidget->setVisible(true);
        possessionLayout->addWidget(itemWidget);
        m_possessionItems.append(itemWidget);
    }

    if (dataContainer) {
        resetData(dataContainer);
    }
}

void ItemWidget::createRightHalf() {
    rightHalf = new QWidget(this);
    rightLayout = new QGridLayout(rightHalf);
    rightLayout->setContentsMargins(8, 8, 8, 8);
    rightLayout->setSpacing(4);
    rightHalf->setLayout(rightLayout);

    rightHalf->setMaximumSize(300, 250);
    rightHalf->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    mainLayout->addWidget(rightHalf, 1);
}

ItemWidget::ItemWidget(const IModData* modData,
                       QWidget *parent, bool verticalRanks)
    : QWidget(parent),
      isMod(true)
{
    setStyleSheet("background: #232323; border-radius: 12px;");
    mainLayout = new QHBoxLayout();
    setLayout(mainLayout);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_mainImgWidget = new QLabel();
    m_mainImgWidget->setAlignment(Qt::AlignCenter);
    m_mainImgWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_mainImgWidget->setMaximumHeight(180);
    m_mainImgWidget->setMinimumSize(50, 50);

    // Create a container overlay layout on top of the image
    QWidget* overlayContainer = new QWidget(m_mainImgWidget);
    overlayContainer->setAttribute(Qt::WA_TransparentForMouseEvents);
    overlayContainer->setStyleSheet("background: transparent;");
    overlayContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    overlayContainer->raise(); // Ensure above the image
    overlayContainer->resize(m_mainImgWidget->size());
    overlayContainer->move(0, 0);

    QHBoxLayout* overlayLayout = new QHBoxLayout(overlayContainer);
    overlayLayout->setContentsMargins(0, 0, 0, 0);
    overlayLayout->setSpacing(0);

    m_overlayGeneral = new QWidget(m_mainImgWidget);
    m_overlayGeneral->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_overlayGeneral->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_overlayGeneral->setStyleSheet("background: transparent;");
    m_overlayGeneral->raise(); // ensure it's above image

    m_overlayPossession = new QWidget(m_mainImgWidget);
    m_overlayPossession->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_overlayPossession->setStyleSheet("background: transparent;");
    m_overlayPossession->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_overlayPossession->raise();

    possessionLayout = new QVBoxLayout(m_overlayPossession);
    possessionLayout->setContentsMargins(0, 0, 0, 0);
    possessionLayout->setSpacing(4);

    overlayLayout->addWidget(m_overlayPossession, 1);
    overlayLayout->addWidget(m_overlayGeneral, 10);

    leftHalf = new QFrame(this);
    leftHalf->setFrameShape(QFrame::Box);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftHalf);
    leftLayout->addWidget(m_mainImgWidget, 1);

    mainItemLabel = new QLabel(this);
    mainItemLabel->setAlignment(Qt::AlignCenter);
    leftLayout->addWidget(mainItemLabel, 0);

    mainLayout->addWidget(leftHalf);
    createRightHalf();
    setRight(false);

    leftHalf->setMaximumSize(250, 250);
    leftHalf->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    if (modData) {
        resetData(modData);
    }
}

const QStringList& ItemWidget::getSpecialTags() const {
    return m_texts;
}

void ItemWidget::resetData(IDataContainer* newData) {
    if (!newData) return;

    isMod = false;
    dataContainer = newData;
    m_mastered = newData->getMainData().getMastered();

    if (m_mastered) {
        int smallerDim = std::min(leftHalf->width(), leftHalf->height());
        // Scale unlock image
        int unlockSize = smallerDim / 3;

        unlockIconLabel->setFixedSize(unlockSize, unlockSize);

        // Position unlock icon at top-right corner with margin inside leftHalf frame
        int margin = 4;
        unlockIconLabel->move(leftHalf->width() - unlockSize - margin, margin);
        unlockIconLabel->show();
    }
    else {
        unlockIconLabel->hide();
    }

    mainItemLabel->setText(QString::fromStdString(newData->getMainData().getName()));

    const auto& subData = dataContainer->getSubData();

    int neededSize = static_cast<int>(subData.size());
    int currentSize = m_circles.size();

    if (currentSize < neededSize) {
        for (int i = currentSize; i < neededSize; ++i) {
            PartWidget* partWidget = new PartWidget(QPixmap(), ItemData(), rightHalf);
            m_circles.append(partWidget);
        }
    }
    else if (currentSize > neededSize) { //remove unsued but keep stored
        for (int i = neededSize; i < currentSize; ++i) {
            m_circles[i]->reset();
            rightLayout->removeWidget(m_circles[i]);
            m_circles[i]->hide();
        }
    }

    for (int i = 0; i < neededSize; ++i) {
        const auto& itemData = *subData[i];
        m_circles[i]->setPixmap(QPixmap());
        m_circles[i]->setItemData(itemData);
        m_circles[i]->show();
    }

    updateMainCount();
    updateRightLayout();
    loadImagesAsync();
}

void ItemWidget::resetData(const IModData* newModData) {
    if (!newModData) return;

    isMod = true;
    setRight(false);
    modData = newModData;
    m_texts.clear();
    m_mastered = false;
    if (unlockIconLabel) unlockIconLabel->hide();

    for (QWidget* w : m_possessionItems)
        delete w;
    m_possessionItems.clear();

    mainItemLabel->setText(QString::fromStdString(modData->getName()));

    std::vector<RankCount> mods = modData->getPossessionCounts();

    for (const auto& rc : mods) {
        QString text = QString("Rank: %1").arg(rc.rank);

        QWidget* itemWidget = new QWidget(m_overlayPossession);
        QVBoxLayout* vLayout = new QVBoxLayout(itemWidget);
        vLayout->setContentsMargins(0, 0, 0, 0);
        vLayout->setSpacing(2);

        CircleWidget* circle = new CircleWidget(itemWidget);
        circle->setColor(Qt::black);
        circle->setCount(rc.count);
        vLayout->addWidget(circle);

        QLabel* label = new QLabel(text, itemWidget);
        label->setAlignment(Qt::AlignHCenter);
        label->setStyleSheet("background: transparent;");
        label->setVisible(true);
        label->setMinimumHeight(14);
        vLayout->addWidget(label);

        possessionLayout->addWidget(itemWidget);
        m_possessionItems.append(itemWidget);
    }

    loadImagesAsyncMod();
}

void ItemWidget::updateMainCount() {
    if (isMod) {
        const std::vector<RankCount>& counts = modData->getPossessionCounts();

        if (counts.size() != m_possessionItems.size()) {
            // Rebuild if mismatch
            for (QWidget* w : m_possessionItems)
                delete w;
            m_possessionItems.clear();

            for (const auto& rc : counts) {
                QWidget* itemWidget = new QWidget(m_overlayPossession);
                QVBoxLayout* vLayout = new QVBoxLayout(itemWidget);
                vLayout->setContentsMargins(0, 0, 0, 0);
                vLayout->setSpacing(2);

                CircleWidget* circle = new CircleWidget(itemWidget);
                circle->setColor(Qt::black);
                circle->setCount(rc.count);
                vLayout->addWidget(circle);

                QLabel* label = new QLabel(QString("Rank: %1").arg(rc.rank), itemWidget);
                label->setAlignment(Qt::AlignHCenter);
                label->setStyleSheet("background: transparent;");
                label->setVisible(true);
                vLayout->addWidget(label);

                possessionLayout->addWidget(itemWidget);
                m_possessionItems.append(itemWidget);
            }
        } else {
            for (int i = 0; i < counts.size(); ++i) {
                auto* itemWidget = m_possessionItems[i];
                auto* circle = itemWidget->findChild<CircleWidget*>();
                auto* label = itemWidget->findChild<QLabel*>();

                if (!circle || !label) continue;

                if (counts[i].count > 0) {
                    circle->setCount(counts[i].count);
                    itemWidget->setVisible(true);
                    label->setVisible(true);
                } else {
                    itemWidget->setVisible(false);
                }
            }
        }
        return;
    }

    auto& counts = dataContainer->getMainData().getPossessionCounts();
    ItemPossessionType types[] = {
        ItemPossessionType::Intact,
        ItemPossessionType::Exceptional,
        ItemPossessionType::Flawless,
        ItemPossessionType::Radiant
    };

    for (int i = 0; i < m_possessionItems.size(); ++i) {
        auto* itemWidget = m_possessionItems[i];
        auto* circle = itemWidget->findChild<CircleWidget*>();
        auto* label = itemWidget->findChild<QLabel*>();

        if (!circle || !label) continue;
        if (i >= 4) break; // Only 4 types defined

        int count = 0;
        auto it = counts.find(types[i]);
        count = it != counts.end() ? it->second : 0;
        if (count > 0) {
            circle->setCount(count);
            itemWidget->setVisible(true);
            label->setVisible(true);
        } else {
            itemWidget->setVisible(false);
        }
    }
}


void ItemWidget::updateRightLayout() {
    if (isMod) return;
    int rightWidth = rightHalf->width();
    int circleSize = 64; // desired min circle diameter
    int spacing = rightLayout->spacing();
    int circlesPerRow = std::max(1, (rightWidth + spacing) / (circleSize + spacing));

    // Remove all widgets from layout
    while (rightLayout->count() > 0) {
        QLayoutItem* item = rightLayout->takeAt(0);
        if (item->widget())
            rightLayout->removeWidget(item->widget());
        delete item;
    }

    // Add IngredientWidgets again in new grid
    int row = 0, col = 0;
    for (PartWidget *circle : m_circles) {
        rightLayout->addWidget(circle, row, col);
        ++col;
        if (col >= circlesPerRow) {
            col = 0;
            ++row;
        }
    }
}

void ItemWidget::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    if (!rightHidden) {
        updateRightLayout();
    }

    if (m_mastered) {
        int smallerDim = std::min(leftHalf->width(), leftHalf->height());
        // Scale unlock image
        int unlockSize = smallerDim / 3;

        unlockIconLabel->setFixedSize(unlockSize, unlockSize);

        // Position unlock icon at top-right corner with margin inside leftHalf frame
        int margin = 4;
        unlockIconLabel->move(leftHalf->width() - unlockSize - margin, margin);
        unlockIconLabel->show();
    }

    updateMainCount();
}

const IDataContainer& ItemWidget::getDataContainer() const {
    return *dataContainer;
}

const IModData& ItemWidget::getModData() const {
    return *modData;
}

// Size hints to keep the widget visually square
QSize ItemWidget::minimumSizeHint() const
{
    return QSize(240, 240);
}

QSize ItemWidget::sizeHint() const
{
    if (rightHidden) {
        return QSize(250, 320);  // adjust width since right half is hidden
    }
    // Default full size hint
    return QSize(400, 320);
}

void ItemWidget::toggleRight() {
    if (MainWindow::getWindowSettings().fullItems) return;
    if (isMod) return;
    setRight(rightHidden);
}

void ItemWidget::setRight(bool show) {
    rightHidden = !show;
    if (rightHidden) {
        rightHalf->setFixedSize(0, 0);
        rightHalf->hide();
        leftHalf->setMaximumSize(300, 250);
        leftHalf->show();
    } else {
        rightHalf->setMaximumSize(300, 250);
        rightHalf->show();
        leftHalf->setMaximumSize(0, 0);
        leftHalf->hide();
        //executes this after current function finish making rightHalf size correctly first
        QTimer::singleShot(0, this, [this]() {
            updateRightLayout();
        });
    }
}

void ItemWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        toggleRight();
    }
    QWidget::mousePressEvent(event);
}

//TODO: move both into a real background thread class/make another one that handles just this
void ItemWidget::loadImagesAsync() {
    if (!dataContainer) return;

    // Main image loading
    std::string imageUrl = dataContainer->getMainData().getImage();
    QPointer mainWidget = m_mainImgWidget;
    //Clion complains this is unused if [[maybe_unused]] is not set
    [[maybe_unused]] bool saveDownload = MainWindow::getWindowSettings().saveDownload;

    (void) QtConcurrent::run([imageUrl, mainWidget, saveDownload]() {
        std::shared_ptr<const std::vector<uint8_t>> imgBytesPtr;
        if (saveDownload) {
            imgBytesPtr = GetImageByFileOrDownload(imageUrl);
        } else {
            imgBytesPtr = fetchUrlCached(imageUrl, FetchType::PNG);
        }

        if (!imgBytesPtr || imgBytesPtr->empty()) return;

        QByteArray imgBytes(reinterpret_cast<const char*>(imgBytesPtr->data()), static_cast<long long>(imgBytesPtr->size()));
        QPixmap pixmap;
        if (!pixmap.loadFromData(imgBytes)) return;

        QMetaObject::invokeMethod(mainWidget, [pixmap, mainWidget]() {
            if (!mainWidget) return;
            mainWidget->setPixmap(pixmap.scaledToHeight(180, Qt::SmoothTransformation));
        }, Qt::QueuedConnection);
    });

    // Sub-images loading
    const auto& subData = dataContainer->getSubData();
    auto count = std::min(subData.size(), static_cast<size_t>(m_circles.size()));

    for (int i = 0; i < count; ++i) {
        auto part = subData[i];
        QPointer partWidget = m_circles[i];
        if (!partWidget) continue;

        std::string partImgUrl = part->getImage();

        (void) QtConcurrent::run([partImgUrl, partWidget, saveDownload]() {
            std::shared_ptr<const std::vector<uint8_t>> imgBytesPtr;
            if (saveDownload) {
                imgBytesPtr = GetImageByFileOrDownload(partImgUrl);
            } else {
                imgBytesPtr = fetchUrlCached(partImgUrl, FetchType::PNG);
            }

            if (!imgBytesPtr || imgBytesPtr->empty()) return;

            QByteArray imgBytes(reinterpret_cast<const char*>(imgBytesPtr->data()), static_cast<long long>(imgBytesPtr->size()));
            QPixmap pixmap;
            if (!pixmap.loadFromData(imgBytes)) return;

            QMetaObject::invokeMethod(partWidget, [pixmap, partWidget]() {
                if (!partWidget) return;
                partWidget->setPixmap(pixmap);
            }, Qt::QueuedConnection);
        });
    }
}


void ItemWidget::loadImagesAsyncMod() {
    if (!modData) return;

    std::string imageUrl = modData->getImage();
    QPointer mainWidget = m_mainImgWidget;

    (void) QtConcurrent::run([imageUrl, mainWidget]() {
        std::shared_ptr<const std::vector<uint8_t>> imgBytesPtr;
        if (MainWindow::getWindowSettings().saveDownload) {
            imgBytesPtr = GetImageByFileOrDownload(imageUrl);
        }
        else {
            imgBytesPtr = fetchUrlCached(imageUrl, FetchType::PNG);
        }

        if (!imgBytesPtr || imgBytesPtr->empty()) return;

        QByteArray imgBytes(reinterpret_cast<const char*>(imgBytesPtr->data()), static_cast<long long>(imgBytesPtr->size()));
        QPixmap pixmap;
        if (!pixmap.loadFromData(imgBytes)) {
            return;
        }

        QMetaObject::invokeMethod(mainWidget, [pixmap, mainWidget]() {
            if (!mainWidget) return;
            mainWidget->setPixmap(pixmap.scaledToHeight(180, Qt::SmoothTransformation));
        }, Qt::QueuedConnection);
    });
}

ItemWidget::~ItemWidget() {
    for (QWidget* w : m_possessionItems)
        delete w;
    m_texts.clear();
    m_circles.clear();
}
