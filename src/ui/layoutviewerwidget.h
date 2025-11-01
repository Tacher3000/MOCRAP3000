#ifndef LAYOUTVIEWERWIDGET_H
#define LAYOUTVIEWERWIDGET_H

#include <QWidget>
#include <QGraphicsView>
#include <QVBoxLayout>
#include <QGraphicsScene>
#include <QWheelEvent>
#include <QMouseEvent>
#include "../core/layoutstructures.h"
#include "viewerwidget.h"

class LayoutViewerWidget : public QWidget {
    Q_OBJECT

public:
    LayoutViewerWidget(QWidget *parent = nullptr);
    void setLayoutSolution(const NestingSolution& solution);

private:
    CustomGraphicsView *view;
    QVBoxLayout *layout;
};

#endif // LAYOUTVIEWERWIDGET_H
