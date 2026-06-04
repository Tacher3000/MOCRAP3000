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
    void addCustomSheet(const Part& part);

signals:
    void requestLoadCustomSheet();

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
    int m_sheetIdCounter = 1;
};

#endif // SHEETLISTWIDGET_H
