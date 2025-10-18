#include "viewerwidget.h"
#include <QGraphicsScene>
#include <QGraphicsLineItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsPathItem>
#include <cmath>

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

ViewerWidget::ViewerWidget(QWidget *parent) : QWidget(parent) {
    layout = new QVBoxLayout(this);
    view = new CustomGraphicsView(this);
    view->setScene(new QGraphicsScene(this));
    layout->addWidget(view);
    setLayout(layout);
}

void ViewerWidget::setGeometry(const Geometry& geom) {
    QGraphicsScene* scene = view->scene();
    scene->clear();

    for (const auto& line : geom.lines) {
        scene->addLine(line.start.x, line.start.y, line.end.x, line.end.y);
    }

    for (const auto& arc : geom.arcs) {
        QPainterPath path;
        double pi = acos(-1.0);
        double startAngle = arc.startAngle * 180.0 / pi;
        double spanAngle = (arc.endAngle - arc.startAngle) * 180.0 / pi;
        if (!arc.isCounterClockwise) {
            spanAngle = -spanAngle;
        }
        path.arcMoveTo(arc.center.x - arc.radius, arc.center.y - arc.radius, arc.radius * 2.0, arc.radius * 2.0, startAngle);
        path.arcTo(arc.center.x - arc.radius, arc.center.y - arc.radius, arc.radius * 2.0, arc.radius * 2.0, startAngle, spanAngle);
        scene->addPath(path);
    }

    for (const auto& circle : geom.circles) {
        scene->addEllipse(circle.center.x - circle.radius, circle.center.y - circle.radius, circle.radius * 2.0, circle.radius * 2.0);
    }

    for (const auto& poly : geom.lwpolylines) {
        QPolygonF qpoly;
        for (const auto& vert : poly.vertices) {
            qpoly << QPointF(vert.x, vert.y);
        }
        scene->addPolygon(qpoly);
    }
}
