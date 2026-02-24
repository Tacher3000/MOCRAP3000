#include "geometrynormalizer.h"
#include <cmath>
#include <algorithm>
#include <vector>
#include <list>
#include <limits>

// Гарантируем наличие M_PI (стандарт C++ не гарантирует его без _USE_MATH_DEFINES в MSVC, но в MinGW обычно есть)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace geometry {

// Константы для точности
static constexpr double EPSILON = 1e-5;
static constexpr double ANGLE_TOLERANCE = 0.1; // Максимальное отклонение хорды (для дискретизации)

// --- Вспомогательные структуры и функции ---

struct Segment {
    Point start;
    Point end;
    std::vector<Point> intermediatePoints; // Точки между start и end (для дуг)
    bool used = false;
};

// Функция расчета расстояния (квадрат)
double distSq(const Point& a, const Point& b) {
    double dx = a.x - b.x;
    double dy = a.y - b.y;
    return dx*dx + dy*dy;
}

// Генерация точек дуги по центру, радиусу и углам
std::vector<Point> generateArcPoints(double cx, double cy, double r, double startAng, double endAng, bool ccw) {
    std::vector<Point> points;

    // Нормализация углов
    if (ccw) {
        if (endAng < startAng) endAng += 2 * M_PI;
    } else {
        if (startAng < endAng) startAng += 2 * M_PI;
    }

    double sweep = std::abs(endAng - startAng);
    // Адаптивное количество шагов на основе радиуса и точности
    // s = 2 * r * sin(step / 2) <= tolerance => step approx 2 * acos(1 - tol/r)
    // Защита от acos(>1)
    double val = 1.0 - ANGLE_TOLERANCE / r;
    if (val < -1.0) val = -1.0;
    if (val > 1.0) val = 1.0;

    double stepAngle = 2.0 * std::acos(val);
    if (std::isnan(stepAngle) || stepAngle < 0.01) stepAngle = 0.05; // Защита от слишком малого шага

    int steps = std::max(4, static_cast<int>(std::ceil(sweep / stepAngle)));

    for (int i = 1; i < steps; ++i) { // start и end добавляются снаружи
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

    // Находим середину хорды
    double mx = (p1.x + p2.x) / 2.0;
    double my = (p1.y + p2.y) / 2.0;

    // Высота сегмента и направление
    double side = (bulge > 0) ? 1.0 : -1.0;
    // Если дуга > 180 (abs(bulge) > 1), центр лежит по другую сторону хорды
    if (std::abs(bulge) > 1.0) side = -side;

    // Расстояние от хорды до центра: h = sqrt(r^2 - (d/2)^2)
    double halfChord = chordLen / 2.0;
    double distToCenter = std::sqrt(std::max(0.0, radius*radius - halfChord*halfChord));

    // Нормаль к хорде (-dy, dx) / len
    double nx = -dy / chordLen;
    double ny = dx / chordLen;

    double cx = mx - side * distToCenter * nx;
    double cy = my - side * distToCenter * ny;

    double startAng = std::atan2(p1.y - cy, p1.x - cx);
    double endAng = std::atan2(p2.y - cy, p2.x - cx);
    bool ccw = bulge > 0;

    return generateArcPoints(cx, cy, radius, startAng, endAng, ccw);
}

// Преобразование одной полилинии в контур
Contour toContour(const LWPolyline& poly) {
    Contour contour;
    if (poly.vertices.empty()) return contour;

    bool closed = (poly.flags & 1);
    size_t count = poly.vertices.size();

    // Если замкнута, обрабатываем последнее ребро (last -> first)
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

    // Если не замкнута, добавляем последнюю точку
    if (!closed) {
        const auto& last = poly.vertices.back();
        contour.points.push_back({last.x, last.y});
    } else {
        // Гарантируем физическое замыкание (первая точка == последняя)
        if (!contour.points.empty() &&
            (std::abs(contour.points.front().x - contour.points.back().x) > EPSILON ||
             std::abs(contour.points.front().y - contour.points.back().y) > EPSILON))
        {
            contour.points.push_back(contour.points.front());
        }
    }
    return contour;
}

// Сшивание разрозненных сегментов (Lines и Arcs)
std::vector<Contour> stitchSegments(const std::vector<Segment>& inputSegments) {
    std::vector<Contour> contours;
    std::list<Segment> pool(inputSegments.begin(), inputSegments.end());

    while (!pool.empty()) {
        Contour currentContour;
        auto it = pool.begin();
        Segment currentSeg = *it;
        pool.erase(it);

        // Добавляем точки первого сегмента
        currentContour.points.push_back(currentSeg.start);
        currentContour.points.insert(currentContour.points.end(), currentSeg.intermediatePoints.begin(), currentSeg.intermediatePoints.end());
        currentContour.points.push_back(currentSeg.end);

        Point currentEnd = currentSeg.end;
        bool foundNext = true;

        while (foundNext) {
            foundNext = false;
            for (auto searchIt = pool.begin(); searchIt != pool.end(); ++searchIt) {
                // Проверяем начало следующего с концом текущего
                if (distSq(currentEnd, searchIt->start) < EPSILON) {
                    currentContour.points.insert(currentContour.points.end(), searchIt->intermediatePoints.begin(), searchIt->intermediatePoints.end());
                    currentContour.points.push_back(searchIt->end);
                    currentEnd = searchIt->end;
                    pool.erase(searchIt);
                    foundNext = true;
                    break;
                }
                // Проверяем конец следующего с концом текущего (обратный порядок линии)
                else if (distSq(currentEnd, searchIt->end) < EPSILON) {
                    // Реверс сегмента
                    std::vector<Point> revInter = searchIt->intermediatePoints;
                    std::reverse(revInter.begin(), revInter.end());

                    currentContour.points.insert(currentContour.points.end(), revInter.begin(), revInter.end());
                    currentContour.points.push_back(searchIt->start);
                    currentEnd = searchIt->start;
                    pool.erase(searchIt);
                    foundNext = true;
                    break;
                }
            }
        }

        // Замыкание контура
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

// --- Основная функция ---

Polygon normalizePart(const Part& part) {
    Polygon polygon;

    // 1. Обработка LWPolylines
    for (const auto& lw : part.lwpolylines) {
        Contour c = toContour(lw);
        if (c.points.size() > 2) {
            polygon.contours.push_back(c);
        }
    }

    // 2. Обработка Circles
    for (const auto& circ : part.circles) {
        Contour c;
        std::vector<Point> pts = generateArcPoints(circ.center.x, circ.center.y, circ.radius, 0, 2 * M_PI, true);
        c.points = std::move(pts);
        if (!c.points.empty()) c.points.push_back(c.points.front());
        polygon.contours.push_back(c);
    }

    // 3. Сбор из линий и дуг для сшивания
    std::vector<Segment> looseSegments;
    looseSegments.reserve(part.lines.size() + part.arcs.size());

    for (const auto& line : part.lines) {
        looseSegments.push_back({ {line.start.x, line.start.y}, {line.end.x, line.end.y}, {}, false });
    }

    for (const auto& arc : part.arcs) {
        double startRad = arc.startAngle * M_PI / 180.0;
        double endRad = arc.endAngle * M_PI / 180.0;
        bool ccw = (arc.isCounterClockwise != 0);

        std::vector<Point> inter = generateArcPoints(arc.center.x, arc.center.y, arc.radius, startRad, endRad, ccw);

        Point pStart { arc.center.x + arc.radius * std::cos(startRad), arc.center.y + arc.radius * std::sin(startRad) };
        Point pEnd { arc.center.x + arc.radius * std::cos(endRad), arc.center.y + arc.radius * std::sin(endRad) };

        looseSegments.push_back({ pStart, pEnd, inter, false });
    }

    if (!looseSegments.empty()) {
        std::vector<Contour> stitched = stitchSegments(looseSegments);
        polygon.contours.insert(polygon.contours.end(), stitched.begin(), stitched.end());
    }

    // 4. Расчет Bounding Box
    if (!polygon.contours.empty()) {
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
