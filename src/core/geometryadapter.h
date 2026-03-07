#ifndef GEOMETRYADAPTER_H
#define GEOMETRYADAPTER_H

#include <boost/polygon/polygon.hpp>
#include "geometry.h"
#include <vector>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace bp = boost::polygon;

// Определения типов Boost
typedef bp::point_data<long long> BoostPoint;
typedef bp::polygon_data<long long> BoostPolygon;
typedef bp::polygon_with_holes_data<long long> BoostPolygonWithHoles;
typedef bp::polygon_set_data<long long> BoostPolygonSet;

/**
 * @brief Адаптер для конвертации между пользовательскими типами (double) и типами Boost (long long).
 */
class GeometryAdapter {
public:
    GeometryAdapter();

    // Масштабный коэффициент: double -> long long.
    // Значение 10000.0 (10^4) дает точность 0.0001 мм.
    // Deepnest использует 10^7, здесь для безопасности переполнения можно начать с 10^4 или 10^5.
    static constexpr double SCALE_FACTOR = 10000000.0;

    /**
     * @brief Конвертирует Point (double) в BoostPoint (long long).
     */
    static BoostPoint toBoost(const Point& p) {
        return BoostPoint(static_cast<long long>(std::round(p.x * SCALE_FACTOR)),
                          static_cast<long long>(std::round(p.y * SCALE_FACTOR)));
    }

    /**
     * @brief Конвертирует BoostPoint (long long) обратно в Point (double).
     */
    static Point fromBoost(const BoostPoint& p) {
        return Point{static_cast<double>(p.x()) / SCALE_FACTOR,
                     static_cast<double>(p.y()) / SCALE_FACTOR};
    }

    /**
     * @brief Конвертирует деталь (Part) в набор полигонов Boost.
     * * @details
     * Part может содержать дуги и линии. Boost Polygon работает только с линейными сегментами.
     * Поэтому внутри происходит дискретизация (аппроксимация) кривых.
     * Важно: Используется set.insert() для автоматического объединения (union) перекрывающихся контуров детали.
     */
    static BoostPolygonSet toBoost(const Part& part) {
        BoostPolygonSet set;

        for (const auto& contour : part.polygon.contours) {
            std::vector<BoostPoint> pts;
            pts.reserve(contour.points.size());
            for (const auto& p : contour.points) {
                pts.push_back(toBoost(p));
            }

            // Гарантируем замыкание полигона
            if (!pts.empty() && pts.front() != pts.back()) {
                pts.push_back(pts.front());
            }

            if(pts.size() < 3) continue; // Пропускаем вырожденные полигоны (линии, точки)

            BoostPolygon poly;
            bp::set_points(poly, pts.begin(), pts.end());

            // Вставляем полигон в сет. Boost сам обработает пересечения и вложенность (holes).
            set.insert(poly);
        }

        return set;
    }

    /**
     * @brief Дискретизация дуги в набор точек.
     * Необходима, так как библиотеки полигональной алгебры (Clipper, Boost) не понимают кривые Безье/дуги.
     * * @param tolerance Максимальное отклонение хорды от дуги.
     */
    static std::vector<Point> discretizeArc(const Arc& arc, double tolerance = 0.1) {
        std::vector<Point> points;
        double start = arc.startAngle * M_PI / 180.0;
        double end = arc.endAngle * M_PI / 180.0;

        // Нормализация углов
        if (arc.isCounterClockwise && end < start) end += 2 * M_PI;
        if (!arc.isCounterClockwise && start < end) start += 2 * M_PI;

        double sweep = std::abs(end - start);
        // Вычисляем количество шагов на основе допустимой погрешности (хорда)
        int steps = std::max(4, static_cast<int>(std::ceil(sweep / (2 * std::acos(1 - tolerance / arc.radius)))));

        for (int i = 0; i <= steps; ++i) {
            double t = static_cast<double>(i) / steps;
            double angle = start + (end - start) * t;
            points.push_back({
                arc.center.x + arc.radius * std::cos(angle),
                arc.center.y + arc.radius * std::sin(angle)
            });
        }
        return points;
    }
};

#endif // GEOMETRYADAPTER_H
