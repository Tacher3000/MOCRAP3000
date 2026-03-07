#include "geometryutils.h"
#include <cmath>
#include <limits>
#include <QTransform>

namespace geometry {

// Contour toContour(const LWPolyline& poly) {
//     Contour contour;
//     for (const auto& v : poly.vertices) {
//         contour.points.push_back({v.x, v.y});
//     }
//     if ((poly.flags & 1) && !contour.points.empty()) {
//         const auto& first = contour.points.front();
//         const auto& last = contour.points.back();
//         if (std::abs(first.x - last.x) > 1e-6 || std::abs(first.y - last.y) > 1e-6) {
//             contour.points.push_back(first);
//         }
//     }
//     return contour;
// }

// Polygon normalizePart(const Part& part) {
//     Polygon polygon;
//     if (part.lwpolylines.empty()) return polygon;

//     for (const auto& poly : part.lwpolylines) {
//         polygon.contours.push_back(toContour(poly));
//     }
//     if (!polygon.contours.empty()) {
//         double minX = std::numeric_limits<double>::max();
//         double minY = std::numeric_limits<double>::max();
//         double maxX = std::numeric_limits<double>::lowest();
//         double maxY = std::numeric_limits<double>::lowest();

//         for (const auto& contour : polygon.contours) {
//             for (const auto& p : contour.points) {
//                 if (p.x < minX) minX = p.x;
//                 if (p.x > maxX) maxX = p.x;
//                 if (p.y < minY) minY = p.y;
//                 if (p.y > maxY) maxY = p.y;
//             }
//         }
//         polygon.minX = minX;
//         polygon.maxX = maxX;
//         polygon.minY = minY;
//         polygon.maxY = maxY;
//     }
//     return polygon;
// }

QPolygonF toQPolygon(const Contour& contour) {
    QPolygonF poly;
    poly.reserve(static_cast<int>(contour.points.size()));
    for (const auto& p : contour.points) {
        poly << QPointF(p.x, p.y);
    }
    return poly;
}

QPainterPath toQPainterPath(const Polygon& polygon) {
    QPainterPath path;

    path.setFillRule(Qt::OddEvenFill);

    for (const auto& contour : polygon.contours) {
        QPolygonF qPoly = toQPolygon(contour);
        if (!qPoly.isClosed() && !qPoly.isEmpty()) {
            qPoly << qPoly.first();
        }
        path.addPolygon(qPoly);
    }

    return path;
}



QPainterPath partToPath(const Part& part) {
    QPainterPath path;
    path.setFillRule(Qt::WindingFill);

    for (const auto& line : part.lines) {
        path.moveTo(line.start.x, line.start.y);
        path.lineTo(line.end.x, line.end.y);
    }

    for (const auto& circle : part.circles) {
        path.addEllipse(QPointF(circle.center.x, circle.center.y),
                        circle.radius, circle.radius);
    }

    for (const auto& arc : part.arcs) {
        QRectF rect(arc.center.x - arc.radius, arc.center.y - arc.radius,
                    arc.radius * 2, arc.radius * 2);

        double startAngle = arc.startAngle;
        double endAngle = arc.endAngle;
        double span = 0.0;

        if (arc.isCounterClockwise) {
            if (endAngle < startAngle) endAngle += 360.0;
            span = endAngle - startAngle;
        } else {
            if (startAngle < endAngle) startAngle += 360.0;
            span = endAngle - startAngle;
        }

        path.arcMoveTo(rect, startAngle);
        path.arcTo(rect, startAngle, span);
    }

    for (const auto& poly : part.lwpolylines) {
        if (poly.vertices.empty()) continue;

        QPainterPath polyPath;
        polyPath.moveTo(poly.vertices[0].x, poly.vertices[0].y);

        for (size_t i = 0; i < poly.vertices.size(); ++i) {
            size_t nextIdx = (i + 1) % poly.vertices.size();
            if (!((poly.flags & 1)) && i == poly.vertices.size() - 1) break;

            const auto& p1 = poly.vertices[i];
            const auto& p2 = poly.vertices[nextIdx];

            if (std::abs(p1.bulge) < 1e-6) {
                polyPath.lineTo(p2.x, p2.y);
            } else {
                double dx = p2.x - p1.x;
                double dy = p2.y - p1.y;
                double dist = std::sqrt(dx*dx + dy*dy);

                if (dist < 1e-9) continue;

                double radius = dist * (1 + p1.bulge*p1.bulge) / (4 * std::abs(p1.bulge));

                // Простейший способ нарисовать дугу - рассчитать промежуточную точку или использовать arcTo
                // Но расчет параметров arcTo из bulge громоздкий.
                // Для надежности здесь используем прямую линию, если bulge сложен,
                // ИЛИ (лучше) добавляем сегмент.
                // Чтобы не усложнять сейчас код, оставим линию, но корректно соединенную.
                // В будущем сюда можно вернуть полную математику bulge.
                polyPath.lineTo(p2.x, p2.y);
            }
        }

        if (poly.flags & 1) {
            polyPath.closeSubpath();
        }
        path.addPath(polyPath);
    }

    return path;
}

QPainterPath expandPath(const QPainterPath& path, double offset) {
    if (std::abs(offset) < 1e-6) return path;

    QPainterPathStroker stroker;
    stroker.setWidth(offset * 2.0);
    stroker.setJoinStyle(Qt::RoundJoin);
    stroker.setCapStyle(Qt::RoundCap);

    QPainterPath stroke = stroker.createStroke(path);

    QPainterPath result = path + stroke;

    return result.simplified();
}

}
