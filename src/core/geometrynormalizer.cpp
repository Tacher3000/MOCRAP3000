#include "geometrynormalizer.h"
#include <cmath>
#include <algorithm>
#include <vector>
#include <list>
#include <limits>

// Гарантируем наличие M_PI
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace geometry {

static constexpr double EPSILON = 0.01;
static constexpr double ANGLE_TOLERANCE = 0.01;

struct Segment {
    Point start;
    Point end;
    std::vector<Point> intermediatePoints;
    bool used = false;
};

double distSq(const Point& a, const Point& b) {
    double dx = a.x - b.x;
    double dy = a.y - b.y;
    return dx*dx + dy*dy;
}

std::vector<Point> generateArcPoints(double cx, double cy, double r, double startAng, double endAng, bool ccw, bool isHole = false) {
    std::vector<Point> points;

    if (ccw) {
        if (endAng < startAng) endAng += 2 * M_PI;
    } else {
        if (startAng < endAng) startAng += 2 * M_PI;
    }

    double sweep = std::abs(endAng - startAng);

    double val = 1.0 - ANGLE_TOLERANCE / r;
    if (val < -1.0) val = -1.0;
    if (val > 1.0) val = 1.0;

    double stepAngle = 2.0 * std::acos(val);
    if (std::isnan(stepAngle) || stepAngle < 0.01) stepAngle = 0.05;

    int steps = std::max(4, static_cast<int>(std::ceil(sweep / stepAngle)));
    const int MAX_ARC_STEPS = 64;
    if (steps > MAX_ARC_STEPS) {
        steps = MAX_ARC_STEPS;
    }

    for (int i = 1; i < steps; ++i) {
        double t = static_cast<double>(i) / steps;
        double angle = startAng + (endAng - startAng) * t;
        points.push_back({cx + r * std::cos(angle), cy + r * std::sin(angle)});
    }
    return points;
}

// Обработка Bulge (для LWPolyline)
std::vector<Point> processBulge(const Point& p1, const Point& p2, double bulge) {
    if (std::abs(bulge) < 1e-6) return {};

    double dx = p2.x - p1.x;
    double dy = p2.y - p1.y;
    double chordLen = std::sqrt(dx*dx + dy*dy);
    if (chordLen < EPSILON) return {};

    double radius = chordLen * (1 + bulge*bulge) / (4 * std::abs(bulge));
    double theta = 4 * std::atan(std::abs(bulge));

    double mx = (p1.x + p2.x) / 2.0;
    double my = (p1.y + p2.y) / 2.0;

    double side = (bulge > 0) ? 1.0 : -1.0;
    if (std::abs(bulge) > 1.0) side = -side;

    double halfChord = chordLen / 2.0;
    double distToCenter = std::sqrt(std::max(0.0, radius*radius - halfChord*halfChord));

    double nx = -dy / chordLen;
    double ny = dx / chordLen;

    double cx = mx - side * distToCenter * nx;
    double cy = my - side * distToCenter * ny;

    double startAng = std::atan2(p1.y - cy, p1.x - cx);
    double endAng = std::atan2(p2.y - cy, p2.x - cx);
    bool ccw = bulge > 0;

    return generateArcPoints(cx, cy, radius, startAng, endAng, ccw);
}

Contour toContour(const LWPolyline& poly) {
    Contour contour;
    if (poly.vertices.empty()) return contour;

    bool closed = (poly.flags & 1);
    size_t count = poly.vertices.size();

    size_t segments = closed ? count : count - 1;

    for (size_t i = 0; i < segments; ++i) {
        const auto& v1 = poly.vertices[i];
        const auto& v2 = poly.vertices[(i + 1) % count];

        Point p1{v1.x, v1.y};
        Point p2{v2.x, v2.y};

        contour.points.push_back(p1);

        if (std::abs(v1.bulge) > 1e-6) {
            auto arcPts = processBulge(p1, p2, v1.bulge);
            contour.points.insert(contour.points.end(), arcPts.begin(), arcPts.end());
        }
    }

    if (!closed) {
        const auto& last = poly.vertices.back();
        contour.points.push_back({last.x, last.y});
    } else {
        if (!contour.points.empty() &&
            (std::abs(contour.points.front().x - contour.points.back().x) > EPSILON ||
             std::abs(contour.points.front().y - contour.points.back().y) > EPSILON))
        {
            contour.points.push_back(contour.points.front());
        }
    }
    return contour;
}

std::vector<Contour> stitchSegments(const std::vector<Segment>& inputSegments) {
    std::vector<Contour> contours;
    std::list<Segment> pool(inputSegments.begin(), inputSegments.end());

    while (!pool.empty()) {
        std::list<Point> currentPoints;
        auto it = pool.begin();
        Segment currentSeg = *it;
        pool.erase(it);

        currentPoints.push_back(currentSeg.start);
        for (const auto& p : currentSeg.intermediatePoints) currentPoints.push_back(p);
        currentPoints.push_back(currentSeg.end);

        bool foundNext = true;
        while (foundNext) {
            foundNext = false;
            for (auto searchIt = pool.begin(); searchIt != pool.end(); ++searchIt) {
                Point currentStart = currentPoints.front();
                Point currentEnd = currentPoints.back();

                if (distSq(currentEnd, searchIt->start) < EPSILON) {
                    for (const auto& p : searchIt->intermediatePoints) currentPoints.push_back(p);
                    currentPoints.push_back(searchIt->end);
                    pool.erase(searchIt);
                    foundNext = true;
                    break;
                }
                else if (distSq(currentEnd, searchIt->end) < EPSILON) {
                    for (auto rit = searchIt->intermediatePoints.rbegin(); rit != searchIt->intermediatePoints.rend(); ++rit) {
                        currentPoints.push_back(*rit);
                    }
                    currentPoints.push_back(searchIt->start);
                    pool.erase(searchIt);
                    foundNext = true;
                    break;
                }
                else if (distSq(currentStart, searchIt->end) < EPSILON) {
                    for (auto rit = searchIt->intermediatePoints.rbegin(); rit != searchIt->intermediatePoints.rend(); ++rit) {
                        currentPoints.push_front(*rit);
                    }
                    currentPoints.push_front(searchIt->start);
                    pool.erase(searchIt);
                    foundNext = true;
                    break;
                }
                else if (distSq(currentStart, searchIt->start) < EPSILON) {
                    for (const auto& p : searchIt->intermediatePoints) currentPoints.push_front(p);
                    currentPoints.push_front(searchIt->end);
                    pool.erase(searchIt);
                    foundNext = true;
                    break;
                }
            }
        }

        Contour currentContour;
        currentContour.points.assign(currentPoints.begin(), currentPoints.end());

        if (!currentContour.points.empty()) {
            if (distSq(currentContour.points.front(), currentContour.points.back()) < EPSILON) {
                currentContour.points.back() = currentContour.points.front();
            } else {
                currentContour.points.push_back(currentContour.points.front());
            }
        }

        if (currentContour.points.size() > 2) {
            contours.push_back(currentContour);
        }
    }

    return contours;
}

/**
 * @brief Вычисляет площадь контура (по формуле Гаусса).
 */
static double getContourArea(const Contour& c) {
    if (c.points.size() < 3) return 0.0;
    double area = 0.0;
    size_t n = c.points.size();
    for (size_t i = 0, j = n - 1; i < n; j = i++) {
        area += (c.points[j].x + c.points[i].x) * (c.points[j].y - c.points[i].y);
    }
    return std::abs(area) / 2.0;
}

/**
 * @brief Проверяет, находится ли точка внутри контура (Ray-Casting алгоритм).
 */
static bool isPointInContour(const Point& pt, const Contour& contour) {
    bool inside = false;
    size_t n = contour.points.size();
    for (size_t i = 0, j = n - 1; i < n; j = i++) {
        const Point& p1 = contour.points[i];
        const Point& p2 = contour.points[j];

        // Проверяем пересечение луча, направленного вправо по оси X
        if (((p1.y > pt.y) != (p2.y > pt.y)) &&
            (pt.x < (p2.x - p1.x) * (pt.y - p1.y) / (p2.y - p1.y) + p1.x)) {
            inside = !inside;
        }
    }
    return inside;
}

// --- Основная функция ---
Polygon normalizePart(const Part& part) {
    Polygon polygon;

    // 1. Обработка кругов
    for (const auto& circ : part.circles) {
        Contour c;
        std::vector<Point> pts = generateArcPoints(circ.center.x, circ.center.y, circ.radius, 0, 2 * M_PI, true);
        c.points = std::move(pts);
        if (!c.points.empty()) c.points.push_back(c.points.front());
        polygon.contours.push_back(c);
    }

    std::vector<Segment> looseSegments;

    // 2. Обработка линий
    for (const auto& line : part.lines) {
        looseSegments.push_back({ {line.start.x, line.start.y}, {line.end.x, line.end.y}, {}, false });
    }

    // 3. Обработка дуг
    for (const auto& arc : part.arcs) {
        double startRad = arc.startAngle;
        double endRad = arc.endAngle;
        bool ccw = (arc.isCounterClockwise != 0);

        std::vector<Point> inter = generateArcPoints(arc.center.x, arc.center.y, arc.radius, startRad, endRad, ccw);
        Point pStart { arc.center.x + arc.radius * std::cos(startRad), arc.center.y + arc.radius * std::sin(startRad) };
        Point pEnd { arc.center.x + arc.radius * std::cos(endRad), arc.center.y + arc.radius * std::sin(endRad) };
        looseSegments.push_back({ pStart, pEnd, inter, false });
    }

    // 4. Обработка полилиний
    for (const auto& lw : part.lwpolylines) {
        if (lw.vertices.size() < 2) continue;
        bool closed = (lw.flags & 1);
        size_t segmentsCount = closed ? lw.vertices.size() : lw.vertices.size() - 1;

        for (size_t i = 0; i < segmentsCount; ++i) {
            const auto& v1 = lw.vertices[i];
            const auto& v2 = lw.vertices[(i + 1) % lw.vertices.size()];
            Point p1{v1.x, v1.y};
            Point p2{v2.x, v2.y};

            if (std::abs(v1.bulge) > 1e-6) {
                auto arcPts = processBulge(p1, p2, v1.bulge);
                looseSegments.push_back({p1, p2, arcPts, false});
            } else {
                looseSegments.push_back({p1, p2, {}, false});
            }
        }
    }

    // 5. Сшивка сегментов
    if (!looseSegments.empty()) {
        std::vector<Contour> stitched = stitchSegments(looseSegments);
        polygon.contours.insert(polygon.contours.end(), stitched.begin(), stitched.end());
    }

    // 6. Анализ контуров: Габариты, сортировка и определение отверстий
    if (!polygon.contours.empty()) {

        // Сортируем контуры по убыванию площади
        std::sort(polygon.contours.begin(), polygon.contours.end(), [](const Contour& a, const Contour& b) {
            return getContourArea(a) > getContourArea(b);
        });

        // Определяем иерархию (кто внутри кого)
        for (size_t i = 0; i < polygon.contours.size(); ++i) {
            int insideCount = 0;
            if (polygon.contours[i].points.empty()) continue;

            // Берем первую точку контура для проверки (этого достаточно)
            Point testPt = polygon.contours[i].points[0];

            // Проверяем только среди контуров, которые больше по площади (они идут раньше в массиве)
            for (size_t j = 0; j < i; ++j) {
                if (isPointInContour(testPt, polygon.contours[j])) {
                    insideCount++;
                }
            }

            // Если контур лежит внутри нечетного количества родительских контуров, значит это отверстие
            polygon.contours[i].isHole = (insideCount % 2 != 0);
        }

        // Вычисляем общий Bounding Box
        double minX = std::numeric_limits<double>::max();
        double maxX = std::numeric_limits<double>::lowest();
        double minY = std::numeric_limits<double>::max();
        double maxY = std::numeric_limits<double>::lowest();

        for (const auto& contour : polygon.contours) {
            for (const auto& p : contour.points) {
                if (p.x < minX) minX = p.x;
                if (p.x > maxX) maxX = p.x;
                if (p.y < minY) minY = p.y;
                if (p.y > maxY) maxY = p.y;
            }
        }
        polygon.minX = minX;
        polygon.maxX = maxX;
        polygon.minY = minY;
        polygon.maxY = maxY;
    }

    return polygon;
}

} // namespace geometry
