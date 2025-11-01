#include "geometrypainter.h"
#include <QGraphicsLineItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsPathItem>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QPainterPath>
#include <cmath>
#include <QDebug>
#include <QTransform>

void GeometryPainter::drawPart(QGraphicsScene *scene, const Part& part, const QPen& pen) {
    for (const auto& line : part.lines) {
        QGraphicsLineItem* item = scene->addLine(line.start.x, line.start.y, line.end.x, line.end.y);
        item->setPen(pen);
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
    }

    for (const auto& circle : part.circles) {
        QGraphicsEllipseItem* item = scene->addEllipse(
            circle.center.x - circle.radius,
            circle.center.y - circle.radius,
            circle.radius * 2.0,
            circle.radius * 2.0
            );
        item->setPen(pen);
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
    }
}

void GeometryPainter::drawGeometry(QGraphicsScene *scene, const Geometry& geom, const QPen& pen) {
    if (geom.parts.empty()) {
        qDebug() << "GeometryPainter: No geometry parts to display.";
        return;
    }

    try {
        const Part& singlePart = geom.getSinglePart();
        drawPart(scene, singlePart, pen);
    } catch (const std::runtime_error& e) {
        qWarning() << "Error in GeometryPainter::drawGeometry:" << e.what();
    }
}

void GeometryPainter::drawNestingSolution(QGraphicsScene *scene, const NestingSolution& solution) {
    QPen sheetPen(Qt::darkGray);
    sheetPen.setWidth(5);
    QBrush sheetBrush(QColor(240, 240, 240));

    QPen partPen(Qt::blue);
    partPen.setWidth(0);
    QBrush partBrush(QColor(173, 216, 230, 150));

    double currentSheetOffsetX = 0.0;

    for (const auto& sheet : solution.usedSheets) {
        QGraphicsRectItem* sheetRect = scene->addRect(
            0, 0, sheet.width, sheet.height,
            sheetPen, sheetBrush
            );
        sheetRect->setPos(currentSheetOffsetX, 0);

        QGraphicsTextItem* sheetText = scene->addText(QString("Лист %1\nЭффективность: %2%").arg(sheet.id).arg(solution.utilization * 100.0, 0, 'f', 2));
        sheetText->setPos(currentSheetOffsetX + 10, 10);

        currentSheetOffsetX += sheet.width + 50.0;
    }

    double sheetOffsetX = 0.0;
    if (!solution.usedSheets.empty()) {
        sheetOffsetX = (solution.usedSheets.front().width + 50.0);
    }

    for (const auto& placedPart : solution.placedParts) {
        auto it = solution.partGeometryMap.find(placedPart.originalPartId);
        if (it == solution.partGeometryMap.end()) {
            qWarning() << "Error: Geometry not found for part ID:" << placedPart.originalPartId;
            continue;
        }

        const Polygon& polygon = it->second;

        double currentSheetOffset = 0.0;
        if (placedPart.sheetId > 1) {
            currentSheetOffset = (placedPart.sheetId - 1) * sheetOffsetX;
        }

        QPainterPath path;
        for (const auto& contour : polygon.contours) {
            if (contour.points.empty()) continue;

            path.moveTo(contour.points.front().x, contour.points.front().y);
            for (size_t i = 1; i < contour.points.size(); ++i) {
                path.lineTo(contour.points[i].x, contour.points[i].y);
            }
        }

        QGraphicsPathItem* pathItem = scene->addPath(path);
        pathItem->setPen(partPen);
        pathItem->setBrush(partBrush);

        QTransform transform;
        transform.translate(currentSheetOffset + placedPart.x, placedPart.y);
        transform.rotate(placedPart.rotation);

        pathItem->setTransform(transform);

        double partW = polygon.maxX - polygon.minX;
        double partH = polygon.maxY - polygon.minY;

        QGraphicsTextItem* idText = scene->addText(QString::number(placedPart.originalPartId));
        idText->setPos(currentSheetOffset + placedPart.x + partW / 2.0, placedPart.y + partH / 2.0);
    }
}
