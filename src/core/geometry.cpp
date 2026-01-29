#include "geometry.h"
#include <cmath>

// namespace geometry {
// Contour toContour(const LWPolyline& poly) {
//     Contour contour;
//     for (const auto& v : poly.vertices) {
//         if (std::abs(v.bulge) < 1e-6) {
//             contour.points.push_back({v.x, v.y});
//         } else {
//         }
//     }
//     if ((poly.flags & 1) != 0 && !contour.points.empty()) {
//         contour.points.push_back(contour.points.front());
//     }
//     return contour;
// }

// Polygon normalizePart(const Part& part) {
//     Polygon polygon;
//     if (part.lwpolylines.empty()) {
//         return polygon;
//     }

//     // В упрощенном случае берем первую LWPolyline как внешний контур
//     const LWPolyline& mainPoly = part.lwpolylines[0];
//     polygon.contours.push_back(toContour(mainPoly));

//     // TODO: Обработка отверстий (остальные LWPolyline, круги и т.д.)
//     // TODO: Триангуляция и объединение линий/дуг/кругов в контуры
//     // TODO: Вычисление bounding box

//     if (!polygon.contours.empty() && !polygon.contours[0].points.empty()) {
//         double minX = polygon.contours[0].points[0].x;
//         double maxX = minX;
//         double minY = polygon.contours[0].points[0].y;
//         double maxY = minY;
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
// }
