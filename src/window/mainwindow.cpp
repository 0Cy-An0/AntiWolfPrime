#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProgressBar>
#include <QWidget>
#include <QListWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QInputDialog>
#include <QGroupBox>
#include <QSpinBox>

#include "ItemWidget.h"
#include "mainwindow.h"

#include <qguiapplication.h>
#include <qscreen.h>
#include <qscrollbar.h>

#include "FilterWidget.h"
#include "overviewPartWidget.h"
#include "apiParser/apiParser.h"
#include "autostart/autostart.h"
#include "dataReader/dataReader.h"
#include "FileAccess/FileAccess.h"

//TODO: Move somewhere better
bool caseInsensitiveFind(const std::string& str, const std::string& search) {
    if (search.empty()) return true; // empty filter matches anything

    std::string strLower = str;
    std::string searchLower = search;

    std::transform(strLower.begin(), strLower.end(), strLower.begin(), ::tolower);
    std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);

    return strLower.find(searchLower) != std::string::npos;
}

Settings MainWindow::settings;
std::mutex MainWindow::settingsMutex;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setMinimumSize(800, 600);

    auto *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    mainLayout = new QHBoxLayout(centralWidget);

    // Left panel: buttons
    auto* buttonPanel = new QWidget(centralWidget);
    auto* buttonLayout = new QVBoxLayout(buttonPanel);
    buttonLayout->setSpacing(0);
    buttonLayout->setContentsMargins(0,0,0,0);

    const QStringList labels = {
        "Overview", "Foundry", "Relics", "Arcanes", "Mods", "Options"
    };

    for (int i = 0; i < labels.size(); ++i) {
        auto* btn = new QPushButton(labels[i].isEmpty() ? QString("Button %1").arg(i+1) : labels[i], buttonPanel);
        btn->setStyleSheet(btnStyle);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        buttonLayout->addWidget(btn);
        connect(btn, &QPushButton::clicked, this, [this, i]() {
            showContentForIndex(i);
        });
    }

    // Right panel: content structure
    mainLayout->addWidget(buttonPanel, /*stretch*/ 1);

    contentArea = new QStackedWidget(this);

    // Add overview
    QWidget* overviewPage = createOverview();
    contentArea->addWidget(overviewPage); // index 0

    // Add shared lazy loading page
    QWidget* lazyPage = createLazyLoadingPage();
    contentArea->addWidget(lazyPage);     // index 1

    QWidget* optionsPage = createOptionsPage();
    contentArea->addWidget(optionsPage);     // index 2

    backgroundThread = new QThread(this);  // MainWindow owns the thread

    backgroundWorker = new BackgroundWorker(this);

    backgroundWorker->moveToThread(backgroundThread);

    connect(backgroundThread, &QThread::started, backgroundWorker, &BackgroundWorker::startWork);
    connect(backgroundThread, &QThread::finished, backgroundWorker, &QObject::deleteLater);
    connect(backgroundThread, &QThread::finished, backgroundThread, &QObject::deleteLater);
    //i was told this should stop it with destruction of MainWindow but still should be done manually to be sure
    connect(this, &MainWindow::destroyed, backgroundWorker, &BackgroundWorker::stopWork);
    connect(this, &MainWindow::destroyed, backgroundThread, &QThread::quit);

    backgroundThread->start();

    mainLayout->addWidget(contentArea, /*stretch*/ 3);

    // Define filter functions (Important to call this after createLazyLoadingPage())
    IDataContainerFilterFunc = [this](const IDataContainer* data) -> bool {
        InventoryCategories cat = data->getMainData().getCategory();

        if (excludeComboBox->getSelectedCategories() != InventoryCategories::None &&
            hasCategory(cat, excludeComboBox->getSelectedCategories()))
            return false;

        if (includeComboBox->getSelectedCategories() != InventoryCategories::None &&
            !hasCategoryStrict(cat, includeComboBox->getSelectedCategories()))
            return false;

        QString searchText = searchBar->text().trimmed();
        if (!searchText.isEmpty()) {
            // Check main data name
            QString mainName = QString::fromStdString(data->getMainData().getName());
            if (mainName.contains(searchText, Qt::CaseInsensitive)) {
                return true;
            }

            // Check sub data array
            const auto& subDataArray = data->getSubData();
            return std::any_of(
                subDataArray.begin(),
                subDataArray.end(),
                [&](const auto& subData) {
                    QString subName = QString::fromStdString(subData->getName());
                    return subName.contains(searchText, Qt::CaseInsensitive);
                }
            );
        }

        return true;
    };

    IModDataFilterFunc = [this](const IModData* data) -> bool {
        InventoryCategories cat = data->getCategory();

        if (excludeComboBox->getSelectedCategories() != InventoryCategories::None &&
            hasCategory(cat, excludeComboBox->getSelectedCategories()))
            return false;

        if (includeComboBox->getSelectedCategories() != InventoryCategories::None &&
            !hasCategoryStrict(cat, includeComboBox->getSelectedCategories()))
            return false;

        QString searchText = searchBar->text().trimmed();
        if (!searchText.isEmpty()) {
            QString mainName = QString::fromStdString(data->getName());
            if (!mainName.contains(searchText, Qt::CaseInsensitive)) {
                return false;
            }
        }

        return true;
    };

    showContentForIndex(0);
}

QWidget* MainWindow::createLazyLoadingPage()
{
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);

    // Filters and search bars
    includeComboBox = new FilterWidget(page);
    excludeComboBox = new FilterWidget(page);
    searchBar = new QLineEdit(page);

    // set contents and defaults
    for (const auto& cat : categories) {
        includeComboBox->addItem(cat.name, cat.value);
        excludeComboBox->addItem(cat.name, cat.value);
    }
    includeComboBox->addItem("Include: All Categories", InventoryCategories::None, true);
    excludeComboBox->addItem("Exclude: No Categories", InventoryCategories::None, true);
    searchBar->setPlaceholderText("Search...");

    connect(includeComboBox, &FilterWidget::selectionChanged, this, &MainWindow::onFilterChanged);
    connect(excludeComboBox, &FilterWidget::selectionChanged, this, &MainWindow::onFilterChanged);
    connect(searchBar, &QLineEdit::textChanged, this, &MainWindow::onFilterChanged);

    layout->addWidget(excludeComboBox);
    layout->addWidget(includeComboBox);
    layout->addWidget(searchBar);

    // Create contentField
    contentField = new QWidget(page);
    contentField->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    // Create scrollArea
    scrollArea = new QScrollArea(page);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setWidgetResizable(false);
    scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    scrollArea->setWidget(contentField);

    layout->addWidget(scrollArea);

    // Connect lazy loader
    connect(scrollArea->verticalScrollBar(), &QScrollBar::valueChanged,
            this, &MainWindow::updateVisibleWidgets);

    return page;
}

void MainWindow::showContentForIndex(int index)
{
    includeComboBox->resetToDefault();
    excludeComboBox->resetToDefault();
    searchBar->clear();

    // Clear previous visible widgets
    qDeleteAll(currentVisibleWidgets);
    currentVisibleWidgets.clear();

    currentIndex = index;
    switch (index)
    {
        case 0:
            contentArea->setCurrentIndex(0); // Overview
            break;
        case 1:
            contentArea->setCurrentIndex(1); // Lazy page
            changeToFoundry();
            break;
        case 2:
            contentArea->setCurrentIndex(1); // Lazy page
            changeToRelics();
            break;
        case 3:
            contentArea->setCurrentIndex(1); // Lazy page
            changeToArcanes();
            break;
        case 4:
            contentArea->setCurrentIndex(1); // Lazy page
            changeToMods();
            break;
        case 5:
            contentArea->setCurrentIndex(2); // Options Page
        default:
            break;
    }

    mainLayout->invalidate();
}

void MainWindow::changeToFoundry()
{
    if (recipes.empty())
        recipes = GetRecipes();

    // Fill data vector
    storedIDataVector.clear();
    storedIDataVector.reserve(static_cast<long long>(recipes.size()));
    for (Recipe& r : recipes) {
        if (settings.hideFounder && hasCategory(r.mainItem.category, InventoryCategories::FounderSpecial)) continue;
        storedIDataVector.push_back(&r);
    }

    // Clear the other type just to be safe
    storedIModDataVector.clear();

    // Get dummy size using first element
    if (!storedIDataVector.isEmpty()) {
        ItemWidget dummy(storedIDataVector[0], nullptr, {});
        QSize size = dummy.sizeHint();
        updateLazyLoading(size, {});
        dummy.deleteLater();
    }
}

void MainWindow::changeToRelics()
{
    if (relics.empty())
        relics = GetRelics();

    // Fill data vector
    storedIDataVector.clear();
    storedIDataVector.reserve(static_cast<long long>(relics.size()));
    for (Relic& r : relics)
        storedIDataVector.push_back(&r);

    // Clear the other type just to be safe
    storedIModDataVector.clear();

    if (!storedIDataVector.isEmpty()) {
        ItemWidget dummy(storedIDataVector[0], nullptr, {});
        QSize size = dummy.sizeHint();
        updateLazyLoading(size, {"Intact", "Exceptional", "Flawless", "Radiant"});
        dummy.deleteLater();
    }
}

void MainWindow::changeToArcanes()
{
    if (arcanes.empty())
        arcanes = GetArcanes();

    // Fill mod data vector
    storedIModDataVector.clear();
    storedIModDataVector.reserve(static_cast<long long>(arcanes.size()));
    for (Arcane& a : arcanes)
        storedIModDataVector.push_back(&a);

    // Clear the other type just to be safe
    storedIDataVector.clear();

    if (!storedIModDataVector.isEmpty()) {
        ItemWidget dummy(storedIModDataVector[0], nullptr);
        QSize size = dummy.sizeHint();
        updateLazyLoading(size, {});
        dummy.deleteLater();
    }
}

void MainWindow::changeToMods()
{
    if (mods.empty())
        mods = GetMods();

    // Fill mod data vector
    storedIModDataVector.clear();
    storedIModDataVector.reserve(static_cast<long long>(mods.size()));
    for (Mod& m : mods)
        storedIModDataVector.push_back(&m);

    // Clear the other type just to be safe
    storedIDataVector.clear();

    if (!storedIModDataVector.isEmpty()) {
        ItemWidget dummy(storedIModDataVector[0], nullptr);
        QSize size = dummy.sizeHint();
        updateLazyLoading(size, {});
        dummy.deleteLater();
    }
}

int GetXPForRank(int rank) {
    if (rank <= 30) {
        return 2500 * rank * rank;
    }
    int legendaryRank = rank - 30;
    return 2250000 + (147500 * legendaryRank);
}

int GetCumulativeXPForRank(int rank) {
    if (rank <= 30) {
        // Use formula for sum of squares: sum(i^2) = n(n+1)(2n+1)/6
        return 2500 * (rank * (rank + 1) * (2 * rank + 1)) / 6;
    }

    // XP required to reach rank 30
    int xpAt30 = 2500 * 30 * 31 * 61 / 6;
    int legendaryRanks = rank - 30;
    return xpAt30 + legendaryRanks * 147500;
}

QWidget *MainWindow::createOverview() {
    auto *mainWidget = new QWidget(this);
    auto *OverviewLayout = new QVBoxLayout(mainWidget);
    OverviewLayout->setContentsMargins(0, 0, 0, 0);
    OverviewLayout->setSpacing(6);

    // Top: Centered Name Text
    auto *nameLabel = new QLabel("Your Data");
    nameLabel->setAlignment(Qt::AlignCenter);
    QFont nameFont = nameLabel->font();
    nameFont.setPointSize(14);
    nameFont.setBold(true);
    nameLabel->setFont(nameFont);

    // Progress bars container: left and right under the name
    auto *progressBarsContainer = new QWidget(mainWidget);
    auto *progressBarsLayout = new QHBoxLayout(progressBarsContainer);
    progressBarsLayout->setContentsMargins(0, 0, 0, 0);
    progressBarsLayout->setSpacing(10);

    // Left progress bar + label: Xp To Mastery Rank {X}
    auto *leftProgressWidget = new QWidget(progressBarsContainer);
    auto *leftProgressLayout = new QVBoxLayout(leftProgressWidget);
    leftProgressLayout->setContentsMargins(0, 0, 0, 0);
    leftProgressLayout->setSpacing(2);

    // Create label
    xpToMasteryLabel = new QLabel(progressBarsContainer);

    // Create progress bar
    xpToMasteryBar = new QProgressBar(mainWidget);
    xpToMasteryBar->setTextVisible(true);
    xpToMasteryBar->setAlignment(Qt::AlignCenter);

    leftProgressLayout->addWidget(xpToMasteryLabel);
    leftProgressLayout->addWidget(xpToMasteryBar);

    // Right progress bar + label: Xp To Max
    auto *rightProgressWidget = new QWidget(progressBarsContainer);
    auto *rightProgressLayout = new QVBoxLayout(rightProgressWidget);
    rightProgressLayout->setContentsMargins(0, 0, 0, 0);
    rightProgressLayout->setSpacing(2);

    auto *xpToMaxLabel = new QLabel("Xp To Max", progressBarsContainer);
    xpToMaxBar = new QProgressBar(progressBarsContainer);
    xpToMaxBar->setTextVisible(true);
    xpToMaxBar->setAlignment(Qt::AlignCenter);

    rightProgressLayout->addWidget(xpToMaxLabel);
    rightProgressLayout->addWidget(xpToMaxBar);

    progressBarsLayout->addWidget(leftProgressWidget);
    progressBarsLayout->addWidget(rightProgressWidget);

    // Three rectangular fields container
    auto* fieldsWidget = new QWidget(mainWidget);
    auto *fieldsLayout = new QHBoxLayout(fieldsWidget);
    fieldsLayout->setContentsMargins(0, 0, 0, 0);
    fieldsLayout->setSpacing(10);

    equipmentField = new OverviewPartWidget(fieldsWidget);
    starChartField = new OverviewPartWidget(fieldsWidget);
    intrinsicsField = new OverviewPartWidget(fieldsWidget);

    // 1) Collectables: X%
    QVector<QString> sectionHeaders = { "NonPrime", "Prime" };
    QVector<QString> listHeaders   = { "Todo", "Todo" };

    equipmentField->initialize(sectionHeaders, listHeaders);

    // 2) Star Chart: X%
    sectionHeaders   = { "Normal", "Steel Path" };
    listHeaders = { "Todo", "Todo" };

    starChartField->initialize(sectionHeaders, listHeaders);

    // 3) Intrinsics: X%
    // Retrieve the categories
    std::vector<IntrinsicCategory> overviewCategories = GetIntrinsics();

    // clear old data
    sectionHeaders = {};
    listHeaders = {};

    for (const auto &cat : overviewCategories) {
        sectionHeaders.append(QString::fromStdString(cat.name));
        listHeaders.append("Levels");
    };

    intrinsicsField->initialize(sectionHeaders, listHeaders);
    fieldsLayout->addWidget(equipmentField);
    fieldsLayout->addWidget(starChartField);
    fieldsLayout->addWidget(intrinsicsField);

    //this sets all the numbers
    updateOverview();


    // Scrollable area like foundry, but empty for now
    auto *overviewScrollArea = new QScrollArea(mainWidget);
    overviewScrollArea->setWidgetResizable(true);

    auto *emptyContent = new QWidget(mainWidget);
    auto *emptyLayout = new QVBoxLayout(emptyContent);
    emptyContent->setLayout(emptyLayout);

    overviewScrollArea->setWidget(emptyContent);

    // Create a container widget for the first three widgets
    auto *topContainer = new QWidget(mainWidget);
    auto *topLayout = new QVBoxLayout(topContainer);

    topLayout->addWidget(nameLabel);
    topLayout->addWidget(progressBarsContainer);
    topLayout->addWidget(fieldsWidget);
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(0);

    // Add the container and scrollArea to mainLayout with equal stretch
    OverviewLayout->addWidget(topContainer, 1);  // topContainer takes 50%
    OverviewLayout->addWidget(overviewScrollArea, 1);    // scrollArea takes 50%

    return mainWidget;
}

void MainWindow::updateOverview() {
    int currentRank = GetMasteryRank();
    int currentXP = GetCurrentXP();
    int totalXP = extraSpecialXp; //GetCurrentXp sets this if it finds something; like the plexus;
    LogThis("added extraSpecialXp: " + std::to_string(extraSpecialXp));
    int xpForNextRank = GetXPForRank(currentRank + 1);
    int xpForCurrentRank = GetXPForRank(currentRank);

    //Update Mastery Text
    xpToMasteryLabel->setText(("XP To Mastery Rank " + std::to_string(currentRank + 1)).c_str());

    // Set the range from 0 to XP needed to next rank
    xpToMasteryBar->setRange(0, xpForNextRank - xpForCurrentRank);

    // Set the current value to the XP progress
    xpToMasteryBar->setValue(currentXP - xpForCurrentRank);

    // Set the text over the bar to show "x / y"
    xpToMasteryBar->setFormat(QString("%1 / %2").arg(currentXP).arg(GetCumulativeXPForRank(currentRank + 1))); //Xp starts from 0 each new mastery rank

    // 1) Collectables: X%
    if (recipes.empty()) recipes = GetRecipes();
    std::vector<std::vector<ItemData>> collectables = SplitEquipment(recipes);
    QVector<QString> sectionHeaders = { "NonPrime", "Prime" };
    QVector completedValues   = { 0, 0 };
    QVector totalValues       = { 0, 0 };
    QVector<QStringList> sectionLists = { {}, {} };
    QVector<QString> listHeaders   = { "Todo", "Todo" };
    for (int sectionIndex = 0; sectionIndex < collectables.size(); ++sectionIndex) {
        auto &section = collectables[sectionIndex];
        // Stable sort items in this section by info.level
        std::stable_sort(section.begin(), section.end(),
            [](const ItemData &a, const ItemData &b) {
                return a.info.level > b.info.level;
            });
        LogThis("section " + std::to_string(sectionIndex) +  " with " + std::to_string(collectables[sectionIndex].size()) + " items");
        for (const ItemData &item : collectables[sectionIndex]) {

            if (settings.hideFounder && hasCategory(item.category, InventoryCategories::FounderSpecial)) {
                LogThis("Hiding: " + item.name);
                continue;
            }

            totalValues[sectionIndex]++;

            const auto &info = item.info;
            int toAdd = info.maxLevel * (info.usedHalfAffinity ? 100 : 200);
            totalXP += toAdd;
            //LogThis(item.craftedId + " : " + std::to_string(toAdd));
            if (info.level >= info.maxLevel) {
                // Completed
                completedValues[sectionIndex]++;
            } else {
                QString entry = QString::fromStdString(item.name) +
                                " " + QString::number(info.level) +
                                "/" + QString::number(info.maxLevel);
                sectionLists[sectionIndex].append(entry);
            }
        }
        //LogThis("sub total: " + std::to_string(totalXP));
    }
    int totalCompleted = 0;
    int totalOverall = 0;
    for (int c : completedValues) totalCompleted += c;
    for (int t : totalValues) totalOverall += t;

    float combinedCompletion = (totalOverall == 0) ? 0.f : (static_cast<float>(totalCompleted) / static_cast<float>(totalOverall) * 100.f);

    //updateField
    equipmentField->updateData(completedValues, totalValues, sectionLists);
    equipmentField->updateTitle(QString("Equipment: %1%").arg(combinedCompletion, 0, 'f', 1));

    // 2) Star Chart: X%
    MissionSummary summary = GetMissionsSummary();
    // Prepare Normal & Steel Path variables
    int normalCompleted = 0;
    int normalTotal = 0;
    QStringList normalIncomplete;

    int steelCompleted = 0;
    int steelTotal = 0;
    QStringList steelIncomplete;

    for (const auto& m : summary.missions) {
        // Always count for both totals
        normalTotal++;
        steelTotal++;
        totalXP += m.baseXp * 2;

        // Normal path
        if (m.isCompleted) {
            normalCompleted++;
        } else {
            normalIncomplete << QString::fromStdString(m.name);
        }

        // Steel Path
        if (m.sp && m.isCompleted) {
            steelCompleted++;
        }
        if (!m.isCompleted) {
            steelIncomplete << QString::fromStdString(m.name);
        }
    }

    // Combined statistics for the title
    totalCompleted = normalCompleted + steelCompleted;
    totalOverall = normalTotal + steelTotal;
    combinedCompletion = (totalOverall == 0)
        ? 0.f
        : (static_cast<float>(totalCompleted) / static_cast<float>(totalOverall) * 100.f);

    // TODO: maybe split this up further into each region?
    sectionHeaders   = { "Normal", "Steel Path" };
    completedValues  = { normalCompleted, steelCompleted };
    totalValues      = { normalTotal, steelTotal };
    sectionLists = { normalIncomplete, steelIncomplete };
    listHeaders = { "Todo", "Todo" };

    //updateField
    starChartField->updateData(completedValues, totalValues, sectionLists);
    starChartField->updateTitle(QString("Star Chart: %1%").arg(combinedCompletion, 0, 'f', 1));

    // 3) Intrinsics: X%
    // Retrieve the categories
    std::vector<IntrinsicCategory> overviewCategories = GetIntrinsics();

    // clear old data
    sectionHeaders = {};
    completedValues = {};
    totalValues = {};
    sectionLists = {};
    listHeaders= {};

    // Helper lambda to populate one category's section data
    for (const auto &cat : overviewCategories) {
        int completed = 0;
        int total = 0;
        QStringList incomplete;

        for (const auto &skill : cat.skills) {
            incomplete.append(
                QString::fromStdString(skill.name + " " + std::to_string(skill.level) + "/10")
            );
            completed += skill.level;
            total += 10; // each skill is out of 10
            totalXP +=  15000;
        }

        sectionHeaders.append(QString::fromStdString(cat.name));
        completedValues.append(completed);
        totalValues.append(total);
        sectionLists.append(incomplete);
        listHeaders.append("Levels");
    };

    // Calculate combined completion percentage for title
    totalCompleted = 0;
    totalOverall = 0;
    for (int c : completedValues) totalCompleted += c;
    for (int t : totalValues) totalOverall += t;

    combinedCompletion = (totalOverall == 0) ? 0.f : (static_cast<float>(totalCompleted) / static_cast<float>(totalOverall) * 100.f);

    //update Field
    intrinsicsField->updateData(completedValues, totalValues, sectionLists);
    intrinsicsField->updateTitle(QString("Intrinsics: %1%").arg(combinedCompletion, 0, 'f', 1));

    xpToMaxBar->setRange(0, totalXP);
    LogThis("Total XP possible: " + std::to_string(totalXP));
    xpToMaxBar->setValue(currentXP);
}

// ReSharper disable once CppPassValueParameterByConstReference
void MainWindow::updateLazyLoading(const QSize dummySize, const QStringList specialTags)
{
    lastUsedSize = dummySize;
    lastUsedTags = specialTags;
    // Clear visible widgets
    qDeleteAll(currentVisibleWidgets);
    currentVisibleWidgets.clear();

    itemWidth = dummySize.width();
    itemHeight = dummySize.height();

    if (!storedIDataVector.isEmpty()) {
        isIData = true;
    } else {
        isIData = false;
    }

    //apply filters
    filteredIndices.clear();
    if (isIData) {
        for (int i = 0; i < storedIDataVector.size(); ++i) {
            if (IDataContainerFilterFunc(storedIDataVector[i])) {
                filteredIndices.append(i);
            }
        }
    } else {
        for (int i = 0; i < storedIModDataVector.size(); ++i) {
            if (IModDataFilterFunc(storedIModDataVector[i])) {
                filteredIndices.append(i);
            }
        }
    }

    //check if we have any data at all
    int dataSize = static_cast<int>(filteredIndices.size());
    if (dataSize == 0)
        return;

    int availableWidth = scrollArea->viewport()->width();

    columnCount = qMax(1, (availableWidth + spacing) / (itemWidth + spacing));
    totalRows = (dataSize + columnCount - 1) / columnCount;

    contentField->setFixedSize(availableWidth, totalRows * (itemHeight + spacing));

    int viewportHeight = scrollArea->viewport()->height();
    int visibleRows = viewportHeight / (itemHeight + spacing);
    int bufferRows = 2;
    int estimatedVisibleWidgets = columnCount * (visibleRows + 2 * bufferRows);

    int currentSize = static_cast<int>(widgetPool.size());

    if (currentSize < estimatedVisibleWidgets) {
        // Need to add widgets
        int widgetsToAdd = estimatedVisibleWidgets - currentSize;
        for (int i = 0; i < widgetsToAdd; ++i) {
            auto* widget = new ItemWidget(nullptr, contentField, specialTags);
            widget->hide();
            widgetPool.append(widget);
        }
    } else if (currentSize > estimatedVisibleWidgets) {
        // Need to remove widgets
        int widgetsToRemove = currentSize - estimatedVisibleWidgets;
        for (int i = 0; i < widgetsToRemove; ++i) {
            ItemWidget* widget = widgetPool.takeLast();
            widget->deleteLater();
        }
    }

    scrollArea->verticalScrollBar()->setValue(0);
    updateVisibleWidgets();
}

void MainWindow::updateVisibleWidgets()
{
    if (!scrollArea || !contentField)
        return;

    if (filteredIndices.isEmpty())
        return;

    int dataSize = static_cast<int>(filteredIndices.size());

    int availableWidth = scrollArea->viewport()->width();
    columnCount = qMax(1, (availableWidth + spacing) / (itemWidth + spacing));
    totalRows = (dataSize + columnCount - 1) / columnCount;

    int scrollTop = scrollArea->verticalScrollBar()->value();
    int viewportHeight = scrollArea->viewport()->height();
    int visibleRows = viewportHeight / (itemHeight + spacing);
    int bufferRows = 2;
    int startRow = qMax(0, scrollTop / (itemHeight + spacing) - bufferRows);
    int endRow = qMin(totalRows, startRow + visibleRows + 2 * bufferRows);

    // Calculate adjusted spacing
    int gaps = columnCount + 1;
    int totalGapsWidth = availableWidth - (columnCount * itemWidth);
    int adjustedSpacing = (gaps > 0) ? totalGapsWidth / gaps : 0;

    QSet<int> visibleIndices;

    for (int row = startRow; row < endRow; ++row) {
        for (int col = 0; col < columnCount; ++col) {
            int visibleIndex = row * columnCount + col;
            if (visibleIndex >= dataSize)
                break;

            int actualIndex = filteredIndices[visibleIndex]; // This is index in stored vector

            visibleIndices.insert(visibleIndex);

            ItemWidget* widget = nullptr;

            if (!currentVisibleWidgets.contains(visibleIndex)) {
                if (!widgetPool.isEmpty()) {
                    widget = widgetPool.takeLast();
                    widget->setParent(contentField);
                } else {
                    widget = isIData
                        ? new ItemWidget(storedIDataVector[actualIndex], contentField)
                        : new ItemWidget(storedIModDataVector[actualIndex], contentField);
                }

                if (isIData) {
                    widget->resetData(storedIDataVector[actualIndex]);
                } else {
                    widget->resetData(storedIModDataVector[actualIndex]);
                }

                currentVisibleWidgets[visibleIndex] = widget;
            } else {
                widget = currentVisibleWidgets[visibleIndex];
            }

            int x = adjustedSpacing + col * (itemWidth + adjustedSpacing);
            int y = row * (itemHeight + spacing);
            widget->setGeometry(x, y, itemWidth, itemHeight);
            widget->show();
        }
    }


    // Hide and recycle widgets that are no longer visible
    for (auto it = currentVisibleWidgets.begin(); it != currentVisibleWidgets.end();) {
        if (!visibleIndices.contains(it.key())) {
            it.value()->hide();
            widgetPool.append(it.value());
            it = currentVisibleWidgets.erase(it);
        } else {
            ++it;
        }
    }

}

//Mutex to be extra safe; i think its actually needed, given the user can modify them at any time
Settings MainWindow::getWindowSettings() {
    std::lock_guard lock(settingsMutex);
    return settings;
}

//TODO: add export OwnedItems
QWidget* MainWindow::createOptionsPage() {
    auto* page = new QWidget(this);
    auto* gridLayout = new QGridLayout(page);

    // Load settings
    settings = LoadSettings();

    // ---------- Top-Left: Visual Settings ----------
    auto* visualGroup = new QGroupBox("UI / Appearance", page);
    auto* visualLayout = new QVBoxLayout(visualGroup);

    auto fullItemsCheckbox = new QCheckBox("Show Full Items", visualGroup);
    auto founderCheckbox = new QCheckBox("Hide Founder Items", visualGroup);
    founderCheckbox->setChecked(settings.hideFounder);
    fullItemsCheckbox->setChecked(settings.fullItems);
    visualLayout->addWidget(fullItemsCheckbox);
    visualLayout->addWidget(founderCheckbox);

    connect(fullItemsCheckbox, &QCheckBox::toggled, this, [=](bool checked) {
        settings.fullItems = checked;
        WriteSettings(settings);
    });

    connect(founderCheckbox, &QCheckBox::toggled, this, [this](bool checked) {
        settings.hideFounder = checked;
        WriteSettings(settings);
        updateOverview();
    });

    // ---------- Top-Right: Performance ----------
    auto* performanceGroup = new QGroupBox("Performance / Optimization", page);
    auto* performanceLayout = new QVBoxLayout(performanceGroup);

    auto saveDownloadsCheckbox = new QCheckBox("Save loaded Pictures", performanceGroup);
    saveDownloadsCheckbox->setChecked(settings.saveDownload);
    performanceLayout->addWidget(saveDownloadsCheckbox);

    connect(saveDownloadsCheckbox, &QCheckBox::toggled, this, [=](bool checked) {
        settings.saveDownload = checked;
        WriteSettings(settings);
    });

    // ---------- Bottom-Left: Data Input ----------
    auto* dataGroup = new QGroupBox("Warframe Data", page);
    auto* dataLayout = new QVBoxLayout(dataGroup);

    // Explanation label for alternative method
    auto* altMethodLabel = new QLabel(
        "You can safely request your inventory data directly from DE.\n"
        "Go to the Warframe Support site, choose 'Submit a Request' → 'My Account', and select\n"
        "'CCPA or GDPR - General Data Protection Regulation' as the category. Once they reply,\n"
        "you should receive a full data export including your inventory. Place the received file as:\n\n"
        "data/Player/player_data.json",
        dataGroup
    );
    altMethodLabel->setWordWrap(true);
    altMethodLabel->setStyleSheet("font-size: 11px; color: #888;");
    dataLayout->addWidget(altMethodLabel);

    auto authTokenInput = new QLineEdit(dataGroup);
    authTokenInput->setPlaceholderText("will try to download your inv. data by link: 'api.warframe.com/api/inventory.php?accountId=' + this string");

    auto gameUpdateBtn = new QPushButton("Update Game Data", dataGroup);
    gameUpdateBtn->setStyleSheet(btnStyle);

    auto* inputLayout = new QHBoxLayout();
    inputLayout->addWidget(authTokenInput);
    dataLayout->addLayout(inputLayout);
    dataLayout->addWidget(gameUpdateBtn);

    connect(gameUpdateBtn, &QPushButton::clicked, [this]() {
        updateGameData();
    });
    // ---------- Bottom-Right: Automation ----------
    auto* automationGroup = new QGroupBox("Automation / Startup", page);
    auto* automationLayout = new QVBoxLayout(automationGroup);

    auto autoStartCheckbox = new QCheckBox("Start with System", automationGroup);
    autoStartCheckbox->setChecked(settings.startWithSystem);

    auto autoSyncCheckbox = new QCheckBox("Enable Auto Inventory Sync", automationGroup);
    autoSyncCheckbox->setChecked(settings.autoSync);

    auto manualSyncBtn = new QPushButton("Sync inventory", dataGroup);

    auto intervalLabel = new QLabel("Auto-Sync Interval (minutes):", automationGroup);
    auto intervalSpinBox = new QSpinBox(automationGroup);
    intervalSpinBox->setRange(1, 120);
    intervalSpinBox->setValue(settings.syncTime);

    automationLayout->addWidget(autoStartCheckbox);
    automationLayout->addWidget(autoSyncCheckbox);
    automationLayout->addWidget(manualSyncBtn);
    automationLayout->addWidget(intervalLabel);
    automationLayout->addWidget(intervalSpinBox);

    connect(autoStartCheckbox, &QCheckBox::toggled, this, [=](bool checked) {
        settings.startWithSystem = checked;
        WriteSettings(settings);
        bool success = setAutoStart(checked);
        if (success) {
            LogThis(std::string("Successfully ") + (checked ? "enabled" : "disabled") + " autostart");
        }
        else {
            LogThis(std::string("Tried to ") + (checked ? "enable" : "disable") + " autostart and failed");
            // Revert the checkbox state without triggering the toggled signal again
            QSignalBlocker blocker(autoStartCheckbox);
            autoStartCheckbox->setChecked(!checked);
        }
    });

    connect(autoSyncCheckbox, &QCheckBox::toggled, this, [=](bool checked) {
        settings.autoSync = checked;
        WriteSettings(settings);
    });

    connect(intervalSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [=](int val) {
        settings.syncTime = val;
        WriteSettings(settings);
    });

    connect(authTokenInput, &QLineEdit::textChanged, this, [=](const QString& val) {
        settings.nonce = val.toStdString();
        WriteSettings(settings);
    });

    connect(manualSyncBtn, &QPushButton::clicked, this, &MainWindow::updatePlayerData);

    // ---------- Final Layout Assembly ----------
    gridLayout->addWidget(visualGroup, 0, 0);
    gridLayout->addWidget(performanceGroup, 0, 1);
    gridLayout->addWidget(dataGroup, 1, 0);
    gridLayout->addWidget(automationGroup, 1, 1);

    return page;
}



void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    if (!scrollArea || !contentField)
        return;

    updateLazyLoading(lastUsedSize, lastUsedTags);
}

void MainWindow::onFilterChanged()
{
    updateLazyLoading(lastUsedSize, lastUsedTags);
}


void MainWindow::updateGameData() {
    FetchGameUpdate();
    recipes.clear();
    relics.clear();
    arcanes.clear();
    mods.clear();
    RefreshImgMap();
    RefreshNameMap();
    RefreshResultBpMap();
}

//TODO: last thing to do before alpha test: test this
void MainWindow::updatePlayerData(bool skipFetch) {
    LogThis("Updating data");
    if (!skipFetch) {
        bool success = UpdatePlayerData(settings.nonce); //TODO: maybe do something on fail idk put a warning banner up
        if (!success) {
            LogThis("player data failed to download; check if auth is set correctly");
        }
        RefreshCountMap();
        RefreshUpgradeMap();
    }

    RefreshXPMap();

    // Update recipes (IDataContainer)
    for (Recipe& recipe : recipes) {
        UpdateCounts(recipe, /*isRelic=*/false);
    }

    // Update relics (IDataContainer)
    for (Relic& relic : relics) {
        UpdateCounts(relic, /*isRelic=*/true);
    }

    // Update arcanes (IModData)
    for (Arcane& arcane : arcanes) {
        UpdateCounts(arcane);
    }

    // Update mods (IModData)
    for (Mod& mod : mods) {
        UpdateCounts(mod);
    }

    updateOverview();
}

MainWindow::~MainWindow() {
    if (backgroundWorker) {
        backgroundWorker->stopWork();
    }
    if (backgroundThread) {
        backgroundThread->quit();
        backgroundThread->wait();
    }
}
