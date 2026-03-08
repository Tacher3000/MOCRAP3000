#ifndef SHEETLISTWIDGET_H
#define SHEETLISTWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <vector>
#include "../core/layoutstructures.h"

class SheetListWidget : public QWidget {
    Q_OBJECT
public:
    explicit SheetListWidget(QWidget *parent = nullptr);
    std::vector<SheetRequest> getSheets() const;

private slots:
    void addSheet();

private:
    QDoubleSpinBox *spinWidth;
    QDoubleSpinBox *spinHeight;
    QSpinBox *spinQty;
    QCheckBox *checkInfinite;
    QVBoxLayout *listLayout;

    struct SheetEntry {
        SheetRequest data;
        QWidget* widget;
    };
    std::vector<SheetEntry> m_sheets;
};

#endif // SHEETLISTWIDGET_H
