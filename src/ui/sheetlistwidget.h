#ifndef SHEETLISTWIDGET_H
#define SHEETLISTWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <vector>
#include <QGraphicsView>
#include "../core/layoutstructures.h"

class SheetItemWidget : public QWidget {
    Q_OBJECT
public:
    SheetItemWidget(const SheetRequest& req, QWidget* parent = nullptr);
    SheetRequest getRequest() const;

signals:
    void removeRequested(SheetItemWidget* widget);

private:
    SheetRequest m_request;
};

class SheetListWidget : public QWidget {
    Q_OBJECT
public:
    explicit SheetListWidget(QWidget *parent = nullptr);
    std::vector<SheetRequest> getSheets() const;
    void addCustomSheet(const Part& part);

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
    std::vector<SheetItemWidget*> m_sheetWidgets;
    int m_sheetIdCounter = 1;
};

#endif // SHEETLISTWIDGET_H
