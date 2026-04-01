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

        double mathStart = arc.startAngle;
        double mathEnd = arc.endAngle;
        double mathSpan;

        if (arc.isCounterClockwise) {
            mathSpan = mathEnd - mathStart;
            if (mathSpan <= 0) mathSpan += 2 * pi;
        } else {
            mathSpan = mathEnd - mathStart;
            if (mathSpan >= 0) mathSpan -= 2 * pi;
        }

        double startAngleQt = -mathStart * 180.0 / pi;
        double spanQt = -mathSpan * 180.0 / pi;

        QRectF rect(arc.center.x - arc.radius, arc.center.y - arc.radius,
                    arc.radius * 2.0, arc.radius * 2.0);

        path.arcMoveTo(rect, startAngleQt);
        path.arcTo(rect, startAngleQt, spanQt);

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

            if (std::abs(v1.bulge) < 1e-10) {
                path.lineTo(v2.x, v2.y);
            } else {
                double bulge = v1.bulge;
                double dx = v2.x - v1.x;
                double dy = v2.y - v1.y;
                bool ccw = bulge > 0;
                double absBulge = std::abs(bulge);

                double theta = 4 * std::atan(absBulge);
                double chord = std::sqrt(dx * dx + dy * dy);

                if (chord < 1e-9) {
                    path.lineTo(v2.x, v2.y);
                    continue;
                }

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

                double sa_math = std::atan2(v1.y - cy, v1.x - cx);
                double mathSpan = theta;
                if (!ccw) {
                    mathSpan = -mathSpan;
                }

                double startAngleQt = -sa_math * 180.0 / pi;
                double spanQt = -mathSpan * 180.0 / pi;

                path.arcTo(cx - r, cy - r, 2 * r, 2 * r, startAngleQt, spanQt);
            }
        }
        if (closed) {
            path.closeSubpath();
        }
        QGraphicsPathItem* item = scene->addPath(path);
        item->setPen(pen);
        item->setTransform(transform);
    }
}

void GeometryPainter::drawGeometry(QGraphicsScene *scene, const Geometry& geom, const QPen& pen) {
    if (geom.parts.empty()) return;

    double currentX = 0.0;
    double currentY = 0.0;
    double maxRowHeight = 0.0;
    const double spacing = 50.0;
    const double maxRowWidth = 1500.0;

    for (const auto& part : geom.parts) {
        QPainterPath path = geometry::partToPath(part);
        QRectF rect = path.boundingRect();

        if (currentX > 0 && currentX + rect.width() > maxRowWidth) {
            currentX = 0.0;
            currentY += maxRowHeight + spacing;
            maxRowHeight = 0.0;
        }

        QTransform transform;
        transform.translate(currentX - rect.left(), currentY - rect.top());

        drawPart(scene, part, pen, transform);

        currentX += rect.width() + spacing;
        if (rect.height() > maxRowHeight) {
            maxRowHeight = rect.height();
        }
    }
}

void GeometryPainter::drawNestingSolution(QGraphicsScene *scene, const NestingSolution& solution) {
    if (solution.usedSheets.empty()) return;

    QPen sheetPen(Qt::black, 0);
    QPen partPen(Qt::black, 0);

    double currentSheetOffsetX = 0.0;
    const double margin = 50.0;

    double sheetVizStep = 0;
    if (!solution.usedSheets.empty()) {
        sheetVizStep = solution.usedSheets[0].width + margin;
    }

    for (const auto& sheet : solution.usedSheets) {
        scene->addRect(currentSheetOffsetX, 0, sheet.width, sheet.height, sheetPen, Qt::NoBrush);

        QGraphicsTextItem* t = scene->addText(QStringLiteral("Лист %1").arg(sheet.id));
        t->setPos(currentSheetOffsetX, -25);
        t->setTransform(QTransform::fromScale(1, -1), true);

        currentSheetOffsetX += sheet.width + margin;
    }

    std::map<int, QColor> colorMap;

    for (const auto& placedPart : solution.placedParts) {
        auto it = solution.partsMap.find(placedPart.originalPartId);
        if (it == solution.partsMap.end()) continue;

        const Part& originalPart = it->second;
        QPainterPath path = geometry::toQPainterPath(originalPart.polygon);
        QRectF bRect = path.boundingRect();

        QTransform t1;
        t1.translate(-bRect.left(), -bRect.top());

        QTransform r;
        r.rotate(placedPart.rotation);

        QRectF rotatedRect = (t1 * r).map(path).boundingRect();

        QTransform t2;
        t2.translate(-rotatedRect.left(), -rotatedRect.top());

        QTransform t3;
        double sheetOffset = (placedPart.sheetId - 1) * sheetVizStep;
        t3.translate(sheetOffset + placedPart.x, placedPart.y);

        QTransform finalTransform = t1 * r * t2 * t3;

        QPainterPath finalPath = finalTransform.map(path);

        int partId = originalPart.id;
        if (colorMap.find(partId) == colorMap.end()) {
            int hue = (partId * 137) % 360;
            colorMap[partId] = QColor::fromHsv(hue, 200, 200);
        }

        QBrush partBrush(colorMap[partId], Qt::FDiagPattern);
        scene->addPath(finalPath, QPen(Qt::NoPen), partBrush);

        drawPart(scene, originalPart, partPen, finalTransform);
    }
}
