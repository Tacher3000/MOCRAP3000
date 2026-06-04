#include "geometryutils.h"
#include "nfpcalculator.h"
#include "geometryadapter.h"
#include <cmath>
#include <limits>
#include <numbers>
#include <QTransform>
#include <clipper2/clipper.h>

namespace geometry {

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

std::vector<Part> extractRemnants(const NestingSolution& solution, double cutThickness, double minRemnantArea) {
    using namespace Clipper2Lib;
    std::vector<Part> remnants;
    int remnantIdCounter = 9000;

    for (const auto& sheet : solution.usedSheets) {
        Paths64 sheetPoly;

        if (sheet.isCustomShape && sheet.customShape.has_value()) {
            BoostPolygonSet bSet = GeometryAdapter::toBoost(sheet.customShape.value());

            namespace bp = boost::polygon;
            bp::rectangle_data<int> rect;
            if (bp::extents(rect, bSet)) {
                int minX = bp::xl(rect);
                int minY = bp::yl(rect);
                if (minX != 0 || minY != 0) {
                    std::vector<BoostPolygonWithHoles> polys;
                    bSet.get(polys);
                    bSet.clear();
                    for(auto& p : polys) {
                        bp::move(p, bp::HORIZONTAL, -minX);
                        bp::move(p, bp::VERTICAL, -minY);
                        bSet.insert(p);
                    }
                }
            }
            sheetPoly = NFPCalculator::toClipper(bSet);
        } else {
            Path64 rect;
            rect.push_back(Point64(0LL, 0LL));
            rect.push_back(Point64(static_cast<int64_t>(sheet.width * NFPCalculator::NFP_SCALE), 0LL));
            rect.push_back(Point64(static_cast<int64_t>(sheet.width * NFPCalculator::NFP_SCALE), static_cast<int64_t>(sheet.height * NFPCalculator::NFP_SCALE)));
            rect.push_back(Point64(0LL, static_cast<int64_t>(sheet.height * NFPCalculator::NFP_SCALE)));
            sheetPoly.push_back(rect);
        }

        Paths64 placedHoles;

        for (const auto& pp : solution.placedParts) {
            if (pp.sheetId != sheet.id) continue;

            const Part& originalPart = solution.partsMap.at(pp.originalPartId);
            Path64 partPath;

            double rad = pp.rotation * std::numbers::pi / 180.0;
            double cosA = std::cos(rad);
            double sinA = std::sin(rad);

            for (const auto& line : originalPart.lines) {
                double rx = line.start.x * cosA - line.start.y * sinA;
                double ry = line.start.x * sinA + line.start.y * cosA;

                double finalX = (rx + pp.x) * NFPCalculator::NFP_SCALE;
                double finalY = (ry + pp.y) * NFPCalculator::NFP_SCALE;

                partPath.push_back(Point64(finalX, finalY));
            }

            placedHoles.push_back(partPath);
        }

        if (cutThickness > 0.0) {
            placedHoles = InflatePaths(placedHoles,
                                       (cutThickness / 2.0) * NFPCalculator::NFP_SCALE,
                                       JoinType::Miter,
                                       EndType::Polygon);
        }

        Paths64 remnantPaths;
        Clipper64 clipper;
        clipper.AddSubject(sheetPoly);
        clipper.AddClip(placedHoles);
        clipper.Execute(ClipType::Difference, FillRule::NonZero, remnantPaths);

        double scaledMinArea = minRemnantArea * (NFPCalculator::NFP_SCALE * NFPCalculator::NFP_SCALE);

        for (const auto& path : remnantPaths) {
            if (std::abs(Area(path)) > scaledMinArea) {
                Part remPart;
                remPart.id = remnantIdCounter++;
                remPart.name = "Remnant_S" + std::to_string(sheet.id) + "_" + std::to_string(remPart.id);

                for (size_t i = 0; i < path.size(); ++i) {
                    const auto& pt1 = path[i];
                    const auto& pt2 = path[(i + 1) % path.size()];

                    Line l;
                    l.start.x = static_cast<double>(pt1.x) / NFPCalculator::NFP_SCALE;
                    l.start.y = static_cast<double>(pt1.y) / NFPCalculator::NFP_SCALE;
                    l.end.x = static_cast<double>(pt2.x) / NFPCalculator::NFP_SCALE;
                    l.end.y = static_cast<double>(pt2.y) / NFPCalculator::NFP_SCALE;

                    remPart.lines.push_back(l);
                }

                remnants.push_back(remPart);
            }
        }
    }

    return remnants;
}

}
