#ifndef GEOMETRYUTILS_H
#define GEOMETRYUTILS_H

#include "geometry.h"
#include <QPainterPath>
#include <QPolygonF>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace geometry {

// Contour toContour(const LWPolyline& poly);
// Polygon normalizePart(const Part& part);

QPolygonF toQPolygon(const Contour& contour);
QPainterPath toQPainterPath(const Polygon& polygon);

/**
 * @brief Полноценная конвертация Part (Lines, Arcs, Circles, Polys) в QPainterPath.
 * Использует нативные кривые Qt (без аппроксимации в точки).
 */
QPainterPath partToPath(const Part& part);

/**
 * @brief Создает путь с отступом (offset) от исходного.
 * Используется для учета spacing и cutThickness.
 */
QPainterPath expandPath(const QPainterPath& path, double offset);

}

#endif // GEOMETRYUTILS_H
