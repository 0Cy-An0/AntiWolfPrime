#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QScrollArea>
#include <QStackedWidget>
#include <QProgressBar> //this is used even if the IDE tells you its not

#include "FilterWidget.h"
#include "ItemWidget.h"
#include "overviewPartWidget.h"
#include "background/BackgroundWorker.h"
#include "FileAccess/FileAccess.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    QWidget *createLazyLoadingPage();
    static Settings getWindowSettings();
    void updatePlayerData(bool skipFetch = false);

    ~MainWindow() override;

private:
    BackgroundWorker* backgroundWorker = nullptr;
    QThread* backgroundThread = nullptr;

    static std::mutex settingsMutex;
    static Settings settings;

    QScrollArea* scrollArea = nullptr;
    QWidget* contentField = nullptr;
    QGridLayout* scrollLayout = nullptr;
    QStackedWidget *contentArea = nullptr;
    QHBoxLayout *mainLayout = nullptr;
    FilterWidget* includeComboBox{};
    FilterWidget* excludeComboBox{};
    QLineEdit* searchBar{};

    QLabel* xpToMasteryLabel = nullptr;
    QProgressBar* xpToMasteryBar = nullptr;
    QProgressBar* xpToMaxBar = nullptr;
    OverviewPartWidget* equipmentField = nullptr;
    OverviewPartWidget* starChartField = nullptr;
    OverviewPartWidget* intrinsicsField = nullptr;
    ItemWidget* dummyWidget = nullptr;

    int itemWidth = 300;
    int itemHeight = 150;
    int columnCount = 1;
    int spacing = 6;
    int totalRows = 0;
    QSize lastUsedSize = QSize();
    QStringList lastUsedTags = QStringList();
    bool isIData = false;
    int currentIndex = 0;
    QVector<int> filteredIndices;

    // Unified visible widgets container
    QMap<int, ItemWidget*> currentVisibleWidgets;
    QList<ItemWidget*> widgetPool;

    //Vector with Pointers to current data
    QVector<IDataContainer*> storedIDataVector;
    QVector<IModData*> storedIModDataVector;

    // Filter functions
    std::function<bool(const IDataContainer*)> IDataContainerFilterFunc;
    std::function<bool(const IModData*)> IModDataFilterFunc;

    std::vector<Recipe> recipes; //this is important otherwise these get deleted after creation and all filtering etc. causes invalid access violation
    std::vector<Relic> relics;
    std::vector<Arcane> arcanes;
    std::vector<Mod> mods;

    //struct for filter setup
    struct CategoryOption {
        QString name;
        InventoryCategories value;
    };

    const QVector<CategoryOption> categories = {
        { "Prime",        InventoryCategories::Prime },
        { "Warframe",     InventoryCategories::Warframe },
        { "Weapon",       InventoryCategories::Weapon },
        { "Companion",    InventoryCategories::Companion },
        { "Necramech",    InventoryCategories::Necramech },
        { "Archwing",     InventoryCategories::Archwing },
        { "Archweapon",   InventoryCategories::Archweapon },
        { "Component",    InventoryCategories::Component },
        { "Nemesis Weapon",      InventoryCategories::Nemesis },
        //{ "Dojo Resource", InventoryCategories::DojoSpecial },
        //{ "Landing Craft",InventoryCategories::LandingCraft },
        { "Modular Weapon (only shows mastery relevant part)",      InventoryCategories::Modular }
    };;

    QString btnStyle = R"(
        QPushButton {
            background: #333333;
            color: white;
            border: none;
            border-radius: 0px;
            padding: 0px;
            min-height: 48px;
        }
        QPushButton:hover { background: #222222; }
    )";

    /*static QWidget* createField(const QString& title,
                                const QVector<QString>& sectionHeaders,
                                const QVector<int>& completedValues,
                                const QVector<int>& totalValues,
                                const QVector<QStringList>& incompleteLists,
                                const QVector<QString>& incompleteHeaders, QWidget *parent);

    static QWidget* createSectionWidget(const QString& sectionHeader,
                                        int completed,
                                        int total,
                                        const QStringList& list,
                                        const QString& listHeader, QWidget* parent);*/

    QWidget *createOverview();

    void updateOverview();

    void showContentForIndex(int index);

    void changeToFoundry();

    void changeToRelics();

    void changeToArcanes();

    void changeToMods();

    void updateLazyLoading(QSize dummySize, QStringList specialTags);

    QWidget *createOptionsPage();

protected:
    void resizeEvent(QResizeEvent* event) override;
    void onFilterChanged();

    void updateGameData();

    void updateVisibleWidgets();

};
#endif // MAINWINDOW_H
