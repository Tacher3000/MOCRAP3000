#include "viewerwidget.h"
#include <QGraphicsScene>
#include <QGraphicsLineItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsPathItem>
#include <QGraphicsPolygonItem>
#include <QPen>
#include <cmath>
#include <QPainterPath>

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
    view->scale(1, -1);
}

void ViewerWidget::setGeometry(const Geometry& geom) {
    QGraphicsScene* scene = view->scene();
    scene->clear();

    QPen thinPen(Qt::black);
    thinPen.setWidth(0);

    for (const auto& line : geom.lines) {
        QGraphicsLineItem* item = scene->addLine(line.start.x, line.start.y, line.end.x, line.end.y);
        item->setPen(thinPen);
    }

    for (const auto& arc : geom.arcs) {
        QPainterPath path;

        double startAngleDeg = -arc.startAngle * 180.0 / pi;
        double endAngleDeg = -arc.endAngle * 180.0 / pi;

        double spanAngleDeg;
        if (arc.isCounterClockwise) {
            spanAngleDeg = endAngleDeg - startAngleDeg;
        } else {
            spanAngleDeg = startAngleDeg - endAngleDeg;
            spanAngleDeg = -spanAngleDeg;
        }
        QRectF rect(arc.center.x - arc.radius,
                    arc.center.y - arc.radius,
                    arc.radius * 2.0,
                    arc.radius * 2.0);

        path.arcMoveTo(rect, startAngleDeg);
        path.arcTo(rect, startAngleDeg, spanAngleDeg);

        QGraphicsPathItem* item = scene->addPath(path);
        item->setPen(thinPen);
    }

    for (const auto& circle : geom.circles) {
        QGraphicsEllipseItem* item = scene->addEllipse(circle.center.x - circle.radius, circle.center.y - circle.radius, circle.radius * 2.0, circle.radius * 2.0);
        item->setPen(thinPen);
    }

    for (const auto& poly : geom.lwpolylines) {
        QPainterPath path;
        if (!poly.vertices.empty()) {
            path.moveTo(poly.vertices[0].x, poly.vertices[0].y);
        }
        bool closed = (poly.flags & 1) != 0;
        size_t numSegments = closed ? poly.vertices.size() : poly.vertices.size() - 1;
        for (size_t i = 0; i < numSegments; ++i) {
            size_t j = (i + 1) % poly.vertices.size();
            const auto& v1 = poly.vertices[i];
            const auto& v2 = poly.vertices[j];
            double dx = v2.x - v1.x;
            double dy = v2.y - v1.y;
            if (std::abs(v1.bulge) < 1e-10) {
                path.lineTo(v2.x, v2.y);
            } else {
                double bulge = v1.bulge;
                bool ccw = bulge > 0;
                double absBulge = std::abs(bulge);
                double theta = 4 * std::atan(absBulge);
                double chord = std::sqrt(dx * dx + dy * dy);
                double r = chord / (2 * std::sin(theta / 2));
                double h = r * std::cos(theta / 2);
                double mx = (v1.x + v2.x) / 2;
                double my = (v1.y + v2.y) / 2;
                double ux = -dy / chord;
                double uy = dx / chord;
                if (!ccw) {
                    ux = -ux;
                    uy = -uy;
                }
                double cx = mx - h * ux;
                double cy = my - h * uy;
                double sa = std::atan2(v1.y - cy, v1.x - cx);
                double span = theta * 180 / acos(-1.0);
                if (!ccw) {
                    span = -span;
                }
                path.arcTo(cx - r, cy - r, 2 * r, 2 * r, sa * 180 / acos(-1.0), span);
            }
        }
        QGraphicsPathItem* item = scene->addPath(path);
        item->setPen(thinPen);
    }

    // for (const auto& ellipse : geom.ellipses) {
    //     QPainterPath path;
    //     double startAngle = ellipse.startParam * 180.0 / pi;
    //     double spanAngle = (ellipse.endParam - ellipse.startParam) * 180.0 / pi;
    //     if (!ellipse.isCounterClockwise) {
    //         spanAngle = -spanAngle;
    //     }
    //     double major = std::hypot(ellipse.majorAxisEnd.x, ellipse.majorAxisEnd.y);
    //     double minor = major * ellipse.ratio;
    //     double rotation = std::atan2(ellipse.majorAxisEnd.y, ellipse.majorAxisEnd.x) * 180.0 / pi;
    //     QTransform transform;
    //     transform.rotate(rotation);
    //     transform.translate(ellipse.center.x, ellipse.center.y);
    //     path = transform.map(path);
    //     QGraphicsPathItem* item = scene->addPath(path);
    //     item->setPen(thinPen);
    // }

    // for (const auto& poly : geom.polylines) {
    //     QPainterPath path;
    //     if (!poly.vertices.empty()) {
    //         path.moveTo(poly.vertices[0].location.x, poly.vertices[0].location.y);
    //     }
    //     bool closed = (poly.flags & 1) != 0;
    //     size_t numSegments = closed ? poly.vertices.size() : poly.vertices.size() - 1;
    //     for (size_t i = 0; i < numSegments; ++i) {
    //         size_t j = (i + 1) % poly.vertices.size();
    //         const auto& v1 = poly.vertices[i];
    //         const auto& v2 = poly.vertices[j];
    //         double dx = v2.location.x - v1.location.x;
    //         double dy = v2.location.y - v1.location.y;
    //         if (std::abs(v1.bulge) < 1e-10) {
    //             path.lineTo(v2.location.x, v2.location.y);
    //         } else {
    //             double bulge = v1.bulge;
    //             bool ccw = bulge > 0;
    //             double absBulge = std::abs(bulge);
    //             double theta = 4 * std::atan(absBulge);
    //             double chord = std::sqrt(dx * dx + dy * dy);
    //             double r = chord / (2 * std::sin(theta / 2));
    //             double h = r * std::cos(theta / 2);
    //             double mx = (v1.location.x + v2.location.x) / 2;
    //             double my = (v1.location.y + v2.location.y) / 2;
    //             double ux = -dy / chord;
    //             double uy = dx / chord;
    //             if (!ccw) {
    //                 ux = -ux;
    //                 uy = -uy;
    //             }
    //             double cx = mx - h * ux;
    //             double cy = my - h * uy;
    //             double sa = std::atan2(v1.location.y - cy, v1.location.x - cx);
    //             double span = theta * 180 / acos(-1.0);
    //             if (!ccw) {
    //                 span = -span;
    //             }
    //             path.arcTo(cx - r, cy - r, 2 * r, 2 * r, sa * 180 / acos(-1.0), span);
    //         }
    //     }
    //     QGraphicsPathItem* item = scene->addPath(path);
    //     item->setPen(thinPen);
    // }

    // for (const auto& spline : geom.splines) {
    //     QPainterPath path;

    //     if (!spline.fitPoints.empty()) {
    //         path.moveTo(spline.fitPoints[0].x, spline.fitPoints[0].y);
    //         for (size_t i = 1; i < spline.fitPoints.size(); ++i) {
    //             path.lineTo(spline.fitPoints[i].x, spline.fitPoints[i].y);
    //         }
    //     }
    //     else if (!spline.controlPoints.empty()) {
    //         path.moveTo(spline.controlPoints[0].x, spline.controlPoints[0].y);
    //         for (size_t i = 1; i < spline.controlPoints.size(); ++i) {
    //             path.lineTo(spline.controlPoints[i].x, spline.controlPoints[i].y);
    //         }
    //     }

    //     QGraphicsPathItem* item = scene->addPath(path);
    //     item->setPen(thinPen);
    // }


    view->fitInView(scene->itemsBoundingRect(), Qt::KeepAspectRatio);
}
