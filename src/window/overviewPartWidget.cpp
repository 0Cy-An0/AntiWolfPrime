#include "OverviewPartWidget.h"
#include <QHeaderView>

#include "FileAccess/FileAccess.h"

OverviewPartWidget::OverviewPartWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* fieldLayout = new QVBoxLayout(this);
    fieldLayout->setContentsMargins(6, 6, 6, 6);
    fieldLayout->setSpacing(4);

    titleLabel = new QLabel(this);
    QFont titleFont = titleLabel->font();
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    fieldLayout->addWidget(titleLabel, 0, Qt::AlignHCenter);

    auto* sectionsWidget = new QWidget(this);
    sectionsLayout = new QHBoxLayout(sectionsWidget);
    sectionsLayout->setContentsMargins(0, 0, 0, 0);
    sectionsLayout->setSpacing(5);
    fieldLayout->addWidget(sectionsWidget);

    // Border styling
    setStyleSheet("QWidget { border: 1px solid gray; border-radius: 4px; }");
}

void OverviewPartWidget::initialize(const QVector<QString>& sectionHeaders,
                                    const QVector<QString>& listHeaders)
{
    Q_ASSERT(sectionHeaders.size() == listHeaders.size());
    // Clean up old widgets if necessary
    QLayoutItem* item;
    while ((item = sectionsLayout->takeAt(0)) != nullptr) {
        if (QWidget* w = item->widget())
            w->deleteLater();
        delete item;
    }
    sectionHeaderLabels.clear();
    sectionCounterLabels.clear();
    sectionTables.clear();

    for (int i = 0; i < sectionHeaders.size(); ++i) {
        QWidget* section = new QWidget(this);
        QVBoxLayout* sectionLayout = new QVBoxLayout(section);
        sectionLayout->setContentsMargins(4, 4, 4, 4);
        sectionLayout->setSpacing(2);

        QLabel* headerLabel = new QLabel(sectionHeaders[i], section);
        QFont headerFont = headerLabel->font();
        headerFont.setBold(true);
        headerLabel->setFont(headerFont);
        headerLabel->setAlignment(Qt::AlignCenter);
        headerLabel->setMaximumHeight(28);
        headerLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        sectionLayout->addWidget(headerLabel);
        sectionHeaderLabels.append(headerLabel);

        QLabel* counterLabel = new QLabel(section);
        counterLabel->setMaximumHeight(28);
        sectionLayout->addWidget(counterLabel);
        sectionCounterLabels.append(counterLabel);

        QTableWidget* tableWidget = new QTableWidget(section);
        tableWidget->setColumnCount(1);
        tableWidget->setHorizontalHeaderLabels(QStringList() << listHeaders[i]);
        tableWidget->setMinimumHeight(10);
        tableWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        tableWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        tableWidget->setFocusPolicy(Qt::NoFocus);
        tableWidget->setSelectionMode(QAbstractItemView::NoSelection);
        tableWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        tableWidget->verticalHeader()->setVisible(false);
        tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        sectionLayout->addWidget(tableWidget);
        sectionTables.append(tableWidget);

        section->setMinimumHeight(0);

        sectionsLayout->addWidget(section);
        sectionsLayout->setStretch(i, 1);
    }
}

//TODO: maybe optimize this
void OverviewPartWidget::updateData(const QVector<int>& completedValues,
                                    const QVector<int>& totalValues,
                                    const QVector<QStringList>& incompleteLists)
{
    Q_ASSERT(completedValues.size() == sectionCounterLabels.size());
    Q_ASSERT(totalValues.size() == sectionCounterLabels.size());
    Q_ASSERT(incompleteLists.size() == sectionTables.size());

    for (int i = 0; i < sectionCounterLabels.size(); ++i) {
        int completed = completedValues[i];
        int total = totalValues[i];
        int percent = (total == 0) ? 0 : static_cast<int>(100.0 * completed / total);
        QString counterText = QString("%1 / %2 (%3%)").arg(completed).arg(total).arg(percent);
        sectionCounterLabels[i]->setText(counterText);

        QTableWidget* table = sectionTables[i];
        table->clearContents();
        table->setRowCount(incompleteLists[i].size());
        for (int row = 0; row < incompleteLists[i].size(); ++row) {
            auto* item = new QTableWidgetItem(incompleteLists[i][row]);
            item->setFlags(item->flags() & ~Qt::ItemIsEditable); // read-only
            table->setItem(row, 0, item);
        }
        sectionCounterLabels[i]->update();
        sectionTables[i]->viewport()->update(); // refresh table content
    }
}

void OverviewPartWidget::updateTitle(const QString& title) {
    titleLabel->setText(title);
}
