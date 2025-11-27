#include "geometrypainter.h"
#include "geometryutils.h"
#include <QGraphicsLineItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsPathItem>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QPainterPath>
#include <cmath>
#include <QDebug>
#include <QTransform>

void GeometryPainter::drawPart(QGraphicsScene *scene, const Part& part, const QPen& pen, const QTransform& transform) {
    for (const auto& line : part.lines) {
        QGraphicsLineItem* item = scene->addLine(line.start.x, line.start.y, line.end.x, line.end.y);
        item->setPen(pen);
        item->setTransform(transform);
    }

    for (const auto& arc : part.arcs) {
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
        item->setPen(pen);
        item->setTransform(transform);
    }

    for (const auto& circle : part.circles) {
        QGraphicsEllipseItem* item = scene->addEllipse(
            circle.center.x - circle.radius,
            circle.center.y - circle.radius,
            circle.radius * 2.0,
            circle.radius * 2.0
            );
        item->setPen(pen);
        item->setTransform(transform);
    }

    for (const auto& poly : part.lwpolylines) {
        QPainterPath path;
        if (poly.vertices.empty()) continue;

        path.moveTo(poly.vertices[0].x, poly.vertices[0].y);

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

                double r = chord / (2 * std::sin(theta / 2.0));
                double h = r * std::cos(theta / 2.0);

                double mx = (v1.x + v2.x) / 2.0;
                double my = (v1.y + v2.y) / 2.0;

                double ux = -dy / chord;
                double uy = dx / chord;

                if (!ccw) {
                    ux = -ux;
                    uy = -uy;
                }

                double cx = mx + h * ux;
                double cy = my + h * uy;

                double sa = std::atan2(v1.y - cy, v1.x - cx);

                double span = theta * 180.0 / pi;

                if (!ccw) {
                    span = -span;
                }

                path.arcTo(cx - r, cy - r, 2 * r, 2 * r, sa * 180.0 / pi, span);
            }
        }
        QGraphicsPathItem* item = scene->addPath(path);
        item->setPen(pen);
        item->setTransform(transform);
    }
}

// void GeometryPainter::drawGeometry(QGraphicsScene *scene, const Geometry& geom, const QPen& pen) {
//     if (geom.parts.empty()) {
//         qDebug() << "GeometryPainter: No geometry parts to display.";
//         return;
//     }

//     try {
//         const Part& singlePart = geom.getSinglePart();
//         drawPart(scene, singlePart, pen);
//     } catch (const std::runtime_error& e) {
//         qWarning() << "Error in GeometryPainter::drawGeometry:" << e.what();
//     }
// }

void GeometryPainter::drawGeometry(QGraphicsScene *scene, const Geometry& geom, const QPen& pen) {
    if (geom.parts.empty()) return;
    drawPart(scene, geom.parts[0], pen, QTransform());
}

void GeometryPainter::drawNestingSolution(QGraphicsScene *scene, const NestingSolution& solution) {
    if (solution.usedSheets.empty()) return;

    QPen sheetPen(Qt::black); sheetPen.setWidth(0);
    QBrush sheetBrush(Qt::NoBrush);

    QPen partPen(Qt::blue); partPen.setWidth(0);

    double currentSheetOffsetX = 0.0;
    const double margin = 50.0;

    // 1. Рисуем листы
    for (const auto& sheet : solution.usedSheets) {
        scene->addRect(currentSheetOffsetX, 0, sheet.width, sheet.height, sheetPen, sheetBrush);
        QGraphicsTextItem* t = scene->addText(QString("Лист %1").arg(sheet.id));
        t->setPos(currentSheetOffsetX, -25);
        currentSheetOffsetX += sheet.width + margin;
    }

    double sheetStepX = 0;
    if (!solution.usedSheets.empty()) sheetStepX = solution.usedSheets[0].width + margin;

    for (const auto& placedPart : solution.placedParts) {
        auto it = solution.partsMap.find(placedPart.originalPartId);
        if (it == solution.partsMap.end()) continue;

        const Part& originalPart = it->second;

        QPainterPath tempPath = geometry::partToPath(originalPart);
        QRectF origRect = tempPath.boundingRect();

        QTransform transform;

        transform.translate(-origRect.left(), -origRect.top());

        QTransform rotationMatrix;
        rotationMatrix.rotate(placedPart.rotation);

        QRectF rotatedRect = rotationMatrix.mapRect(QRectF(0, 0, origRect.width(), origRect.height()));
        QPainterPath normPath = QTransform::fromTranslate(-origRect.left(), -origRect.top()).map(tempPath);
        QPainterPath rotPath = rotationMatrix.map(normPath);
        QRectF rRect = rotPath.boundingRect();

        double alignX = -rRect.left();
        double alignY = -rRect.top();

        QTransform finalTransform;

        double sheetOffset = (placedPart.sheetId - 1) * sheetStepX;
        finalTransform.translate(sheetOffset + placedPart.x, placedPart.y);

        finalTransform.translate(alignX, alignY);

        finalTransform.rotate(placedPart.rotation);

        finalTransform.translate(-origRect.left(), -origRect.top());

        drawPart(scene, originalPart, partPen, finalTransform);
    }
}
