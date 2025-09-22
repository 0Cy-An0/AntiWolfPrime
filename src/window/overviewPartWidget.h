#pragma once

#include <QLabel>
#include <QVector>
#include <QTableWidget>
#include <QHBoxLayout>

class OverviewPartWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OverviewPartWidget(QWidget* parent = nullptr);

    // Sets up the widget with sections and headers; call once on creation or when structure has changed
    void initialize(const QVector<QString>& sectionHeaders,
                    const QVector<QString>& listHeaders);

    // Updates the data and refreshes visuals; safe to call as often as needed
    void updateData(const QVector<int>& completedValues,
                    const QVector<int>& totalValues,
                    const QVector<QStringList>& incompleteLists);

    //updates the Title
    void updateTitle(const QString &title);

private:
    QLabel* titleLabel;
    QVector<QLabel*> sectionHeaderLabels;
    QVector<QLabel*> sectionCounterLabels;
    QVector<QTableWidget*> sectionTables;
    QHBoxLayout* sectionsLayout;
};

