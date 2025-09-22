#ifndef MULTISELECTCOMBOBOX_H
#define MULTISELECTCOMBOBOX_H

#pragma once

#include <QPushButton>
#include <QDialog>
#include <QGridLayout>
#include <QCheckBox>
#include <QVector>
#include <QString>

#include "dataReader/dataReader.h"

class FilterWidget : public QPushButton {
    Q_OBJECT

public:
    explicit FilterWidget(QWidget* parent = nullptr);
    void addItem(const QString& text, InventoryCategories cat, bool isAllOption = false);

    // Returns a combination of selected InventoryCategories
    InventoryCategories getSelectedCategories() const;

    // Returns selected item texts
    QStringList getSelectedLabels() const;

    //resets the filter
    void resetToDefault();

signals:
    void selectionChanged();

protected:
    void mousePressEvent(QMouseEvent* event) override;

private slots:
    void showPopup();
    void handleCheckboxChanged(int state);

private:
    // Each filter entry
    struct Entry {
        QString text;
        InventoryCategories cat;
        bool checked;
        bool isAllOption;
    };
    QVector<Entry> m_entries;
    QVector<QCheckBox*> m_checkboxes;

    QDialog* m_dialog = nullptr;

    void updateSummaryText();
    void syncCheckboxes();
    void clearDialog();
};



#endif // MULTISELECTCOMBOBOX_H
