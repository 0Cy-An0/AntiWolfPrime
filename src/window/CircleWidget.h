#ifndef CIRCLEWIDGET_H
#define CIRCLEWIDGET_H
#include <QWidget>
#include <QPixmap>
#include <QColor>
#include <QPaintEvent>

class CircleWidget : public QWidget {
    Q_OBJECT
public:
    explicit CircleWidget(QWidget *parent = nullptr);

    // Set the number badge (use -1 to hide)
    void setCount(int count);

    // Set an icon badge (overrides number badge if set)
    void setIcon(const QPixmap& icon);

    // Set background circle color
    void setColor(const QColor& color);

    //Set Pen Color
    void setPen(const QColor &color);

protected:
    void paintEvent(QPaintEvent* /*event*/) override;

    void resizeEvent(QResizeEvent *event) override;

private:
    int m_count;         // number to display, -1 to hide
    QPixmap m_icon;      // icon to display instead of number (optional)
    QColor m_bgColor;
    QColor m_penColor;
    static QString abbreviateNumber(int number);
};

#endif //CIRCLEWIDGET_H
