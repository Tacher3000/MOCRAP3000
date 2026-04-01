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
        path.addEllipse(QPointF(circle.center.x - circle.radius, circle.center.y - circle.radius),
                        circle.radius * 2, circle.radius * 2);
    }

    for (const auto& arc : part.arcs) {
        double mathStart = arc.startAngle;
        double mathEnd = arc.endAngle;
        double mathSpan;

        if (arc.isCounterClockwise) {
            mathSpan = mathEnd - mathStart;
            if (mathSpan <= 0) mathSpan += 2 * M_PI;
        } else {
            mathSpan = mathEnd - mathStart;
            if (mathSpan >= 0) mathSpan -= 2 * M_PI;
        }

        double startAngleQt = -mathStart * 180.0 / M_PI;
        double spanQt = -mathSpan * 180.0 / M_PI;

        QRectF rect(arc.center.x - arc.radius, arc.center.y - arc.radius,
                    arc.radius * 2, arc.radius * 2);

        path.arcMoveTo(rect, startAngleQt);
        path.arcTo(rect, startAngleQt, spanQt);
    }

    for (const auto& poly : part.lwpolylines) {
        if (poly.vertices.empty()) continue;

        QPainterPath polyPath;
        polyPath.moveTo(poly.vertices[0].x, poly.vertices[0].y);

        bool closed = (poly.flags & 1) != 0;
        size_t numSegments = closed ? poly.vertices.size() : poly.vertices.size() - 1;

        for (size_t i = 0; i < numSegments; ++i) {
            size_t j = (i + 1) % poly.vertices.size();
            const auto& v1 = poly.vertices[i];
            const auto& v2 = poly.vertices[j];

            if (std::abs(v1.bulge) < 1e-10) {
                polyPath.lineTo(v2.x, v2.y);
            } else {
                double bulge = v1.bulge;
                double dx = v2.x - v1.x;
                double dy = v2.y - v1.y;
                bool ccw = bulge > 0;
                double absBulge = std::abs(bulge);

                double theta = 4 * std::atan(absBulge);
                double chord = std::sqrt(dx * dx + dy * dy);

                if (chord < 1e-9) {
                    polyPath.lineTo(v2.x, v2.y);
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

                double startAngleQt = -sa_math * 180.0 / M_PI;
                double spanQt = -mathSpan * 180.0 / M_PI;

                polyPath.arcTo(cx - r, cy - r, 2 * r, 2 * r, startAngleQt, spanQt);
            }
        }

        if (closed) {
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
