#pragma once

#include <qfuture.h>
#include <QGridLayout>
#include <QVector>
#include <QString>
#include <QLabel>
#include <QPushButton>

#include "dataReader/dataReader.h"

class CircleWidget;
class PartWidget;

class ItemWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ItemWidget(IDataContainer* dataContainer,
                       QWidget *parent = nullptr, QStringList texts = QStringList());

    void createRightHalf();

    explicit ItemWidget(const IModData* modData,
                        QWidget *parent, bool verticalRanks = false);

    const QStringList &getSpecialTags() const;

    void resetData(IDataContainer *newData);

    void resetData(const IModData *newModData);

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;
    ~ItemWidget() override;

    [[nodiscard]] const IDataContainer& getDataContainer() const;
    [[nodiscard]] const IModData& getModData() const;
    void updateMainCount();

private:
    QHBoxLayout *mainLayout;
    QLabel *mainItemLabel;
    QPixmap m_mainImg;
    QLabel *m_mainImgWidget;
    QWidget *m_overlayGeneral;
    QWidget *m_overlayPossession;
    QList<QWidget*> m_possessionItems;
    QVBoxLayout* possessionLayout;

    QVector<PartWidget*> m_circles;
    int desiredCircleDiameter = 70;

    bool m_mastered = false;
    QLabel *unlockIconLabel = nullptr;

    QWidget *rightHalf = nullptr;
    QFrame  *leftHalf = nullptr;
    QGridLayout *rightLayout = nullptr;

    bool rightHidden = true;

    const IDataContainer* dataContainer = nullptr;
    const IModData* modData = nullptr;
    bool isMod = false;

    QStringList m_texts;


    void toggleRight();

    void setRight(bool show);

    void updateRightLayout();

protected:
    void mousePressEvent(QMouseEvent *event) override;

    void loadImagesAsync();

    void loadImagesAsyncMod();

    void resizeEvent(QResizeEvent *event) override;
};
