#include "layoutviewerwidget.h"
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QPen>
#include <QBrush>
#include <QColor>
#include <QDebug>
#include "geometrypainter.h"
#include "viewerwidget.h"

LayoutViewerWidget::LayoutViewerWidget(QWidget *parent) : QWidget(parent) {
    layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    view = new CustomGraphicsView(this);
    view->setScene(new QGraphicsScene(this));
    layout->addWidget(view);

    setLayout(layout);
}

void LayoutViewerWidget::setLayoutSolution(const NestingSolution& solution) {
    QGraphicsScene* scene = view->scene();
    scene->clear();
    view->resetView();

    GeometryPainter::drawNestingSolution(scene, solution);

    view->fitInView(scene->itemsBoundingRect(), Qt::KeepAspectRatio);
}
