#ifndef PARTLISTWIDGET_H
#define PARTLISTWIDGET_H

#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>
#include <QGraphicsView>
#include <QMouseEvent>
#include "../core/geometry.h"

// --- Виджет одной детали ---
class PartItemWidget : public QWidget {
    Q_OBJECT
public:
    PartItemWidget(const Part& part, QWidget* parent = nullptr);

    bool isIncluded() const;
    int getQuantity() const;
    Part getPart() const;
    void setSelected(bool selected);

signals:
    void clicked(PartItemWidget* item);

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    Part m_part;
    QCheckBox* m_includeCheck;
    QSpinBox* m_quantitySpin;
    QGraphicsView* m_previewView;
};

// --- Виджет списка деталей ---
class PartListWidget : public QWidget {
    Q_OBJECT
public:
    PartListWidget(QWidget* parent = nullptr);

    void addGeometry(const Geometry& geometry);
    void clear();
    Geometry getGeometryForOptimization() const;

signals:
    void partSelected(const Part& part);
    void showAllPartsRequested();

private slots:
    void onItemClicked(PartItemWidget* item);

private:
    QVBoxLayout* m_listLayout;
    QWidget* m_listContainer;
    std::vector<PartItemWidget*> m_items;
};

#endif // PARTLISTWIDGET_H
