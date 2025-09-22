#pragma once

#include "CircleWidget.h"
#include "dataReader/dataReader.h"

class QLabel;

class PartWidget : public QWidget {
    Q_OBJECT
public:
    PartWidget(const QPixmap &icon, const IData &itemData, QWidget *parent);

    void setItemData(const IData &itemData);

    void updateLabelFromName(const std::string &name);

    void updateCountCircles();
    void setPixmap(const QPixmap &pixmap);

    void reset();

private:

    void updateIcon(int size);
    QLabel *iconLabel;
    const IData* m_itemData;
    QPixmap m_icon;
    CircleWidget* m_iconCircle{};
    CircleWidget* m_craftedCircle;
    CircleWidget* m_blueprintCircle;

protected:
    void resizeEvent(QResizeEvent* event) override;

};
