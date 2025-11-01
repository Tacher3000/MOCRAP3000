#include "viewerwidget.h"
#include <QGraphicsScene>
#include <QGraphicsLineItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsPathItem>
#include <QGraphicsPolygonItem>
#include <QPen>
#include <cmath>
#include <QPainterPath>
#include <QDebug>
#include "geometrypainter.h"

CustomGraphicsView::CustomGraphicsView(QWidget *parent) : QGraphicsView(parent) {
    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setInteractive(true);
}

void CustomGraphicsView::wheelEvent(QWheelEvent *event) {
    double factor = 1.2;
    if (event->angleDelta().y() > 0) {
        scale(factor, factor);
    } else {
        scale(1.0 / factor, 1.0 / factor);
    }
    event->accept();
}

void CustomGraphicsView::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && (event->modifiers() & Qt::ControlModifier)) {
        rotating = true;
        lastPos = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    QGraphicsView::mousePressEvent(event);
}

void CustomGraphicsView::mouseMoveEvent(QMouseEvent *event) {
    if (rotating) {
        double angle = (event->pos().x() - lastPos.x()) / 2.0;
        rotate(angle);
        lastPos = event->pos();
        event->accept();
        return;
    }
    QGraphicsView::mouseMoveEvent(event);
}

void CustomGraphicsView::mouseReleaseEvent(QMouseEvent *event) {
    if (rotating) {
        rotating = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
        return;
    }
    QGraphicsView::mouseReleaseEvent(event);
}

void CustomGraphicsView::resetView() {
    this->setTransform(QTransform());
}

ViewerWidget::ViewerWidget(QWidget *parent) : QWidget(parent) {
    layout = new QVBoxLayout(this);
    view = new CustomGraphicsView(this);
    view->setScene(new QGraphicsScene(this));
    layout->addWidget(view);
    setLayout(layout);
    view->scale(1, -1);
}

void ViewerWidget::setGeometry(const Geometry& geom) {
    QGraphicsScene* scene = view->scene();
    scene->clear();
    // view->resetView();


    QPen thinPen(Qt::black);
    thinPen.setWidth(0);

    GeometryPainter::drawGeometry(scene, geom, thinPen);

    view->fitInView(scene->itemsBoundingRect(), Qt::KeepAspectRatio);
}
