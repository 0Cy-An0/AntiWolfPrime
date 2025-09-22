#include "FilterWidget.h"

#include <QMouseEvent>
#include <QPoint>
#include <QApplication>

FilterWidget::FilterWidget(QWidget* parent)
    : QPushButton(parent)
{
    setText("(all)");
    setCheckable(false);
    connect(this, &QPushButton::clicked, this, &FilterWidget::showPopup);
}

void FilterWidget::addItem(const QString& text, InventoryCategories cat, bool isAllOption)
{
    Entry e{text, cat, isAllOption, isAllOption}; // isAllOption checked by default if true
    // Only check "All" if it's the first item; unchecked otherwise.
    if (!m_entries.isEmpty() || !isAllOption)
        e.checked = false;
    m_entries.push_back(e);
    updateSummaryText();
    clearDialog(); // Force dialog to rebuild with new item
}

void FilterWidget::clearDialog() {
    if (m_dialog) {
        m_dialog->close();
        delete m_dialog;
        m_dialog = nullptr;
    }
    m_checkboxes.clear();
}

InventoryCategories FilterWidget::getSelectedCategories() const
{
    InventoryCategories combined = InventoryCategories::None;
    for (const auto& entry : m_entries) {
        if (entry.checked && !entry.isAllOption && entry.cat != InventoryCategories::None) {
            using U = std::underlying_type_t<InventoryCategories>;
            combined = static_cast<InventoryCategories>(static_cast<U>(combined) | static_cast<U>(entry.cat));
        }
    }
    // If only "All" is selected, treat as None category
    return combined;
}

QStringList FilterWidget::getSelectedLabels() const
{
    QStringList selected;
    for (const auto& e : m_entries) {
        if (e.checked) {
            selected << e.text;
        }
    }
    return selected;
}

void FilterWidget::mousePressEvent(QMouseEvent* event)
{
    // Override for touch compatibility
    if (event->button() == Qt::LeftButton)
        showPopup();
    QPushButton::mousePressEvent(event);
}

void FilterWidget::showPopup()
{
    if (m_dialog) {
        m_dialog->close();
        delete m_dialog;
        m_dialog = nullptr;
        m_checkboxes.clear();
    }

    m_dialog = new QDialog(this, Qt::Popup);
    QGridLayout* grid = new QGridLayout(m_dialog);
    grid->setSpacing(6);
    grid->setContentsMargins(8, 8, 8, 8);

    constexpr int columns = 2;
    int row = 0, col = 0;
    m_checkboxes.resize(m_entries.size());
    for (int i = 0; i < m_entries.size(); ++i) {
        QCheckBox* box = new QCheckBox(m_entries[i].text, m_dialog);
        box->setChecked(m_entries[i].checked);
        box->setProperty("index", i);
        connect(box, &QCheckBox::stateChanged, this, &FilterWidget::handleCheckboxChanged);
        grid->addWidget(box, row, col);
        m_checkboxes[i] = box;

        if (++col >= columns) { col = 0; ++row; }
    }

    m_dialog->setLayout(grid);

    // Position below the button
    QPoint p = mapToGlobal(QPoint(0, height()));
    m_dialog->move(p);
    m_dialog->adjustSize();
    m_dialog->show();
}

void FilterWidget::handleCheckboxChanged(int /*state*/)
{
    // Use sender index
    QCheckBox* senderBox = qobject_cast<QCheckBox*>(sender());
    if (!senderBox)
        return;
    int idx = senderBox->property("index").toInt();
    if (idx < 0 || idx >= m_entries.size())
        return;

    // Update entry checked state
    m_entries[idx].checked = senderBox->isChecked();

    // Master "All" logic
    if (m_entries[idx].isAllOption && m_entries[idx].checked) {
        // Uncheck all other options
        for (int i = 0; i < m_entries.size(); ++i) {
            if (i != idx) {
                if (m_entries[i].checked) {
                    m_entries[i].checked = false;
                    if (m_checkboxes[i]->isChecked())
                        m_checkboxes[i]->setChecked(false);
                }
            }
        }
    } else if (!m_entries[idx].isAllOption && m_entries[idx].checked) {
        // Uncheck "All" option if any other is selected
        for (int i = 0; i < m_entries.size(); ++i) {
            if (m_entries[i].isAllOption && m_entries[i].checked) {
                m_entries[i].checked = false;
                if (m_checkboxes[i]->isChecked())
                    m_checkboxes[i]->setChecked(false);
            }
        }
    }

    updateSummaryText();
    emit selectionChanged();  // Notify listeners of change
}

void FilterWidget::updateSummaryText()
{
    // Compose the button label
    QStringList selected;
    for (const Entry& e : m_entries) {
        if (e.checked) selected << e.text;
    }
    setText(selected.isEmpty() ? "(all)" : selected.join(", "));
}

void FilterWidget::syncCheckboxes()
{
    // Sync UI checkboxes to Entry state (if ever needed)
    for (int i = 0; i < m_entries.size() && i < m_checkboxes.size(); ++i) {
        m_checkboxes[i]->setChecked(m_entries[i].checked);
    }
}

void FilterWidget::resetToDefault()
{
    for (Entry& e : m_entries) {
        // Check "All" option and uncheck others
        e.checked = e.isAllOption;
    }
    syncCheckboxes();
    updateSummaryText();
    emit selectionChanged();
}
