#ifndef GEOMETRYPAINTER_H
#define GEOMETRYPAINTER_H

#include <QGraphicsScene>
#include <QPen>
#include <QBrush>
#include "../core/geometry.h"
#include "../core/layoutstructures.h"

class GeometryPainter {
public:
    static void drawGeometry(QGraphicsScene *scene, const Geometry& geom, const QPen& pen);
    static void drawPart(QGraphicsScene *scene, const Part& part, const QPen& pen);
    static void drawNestingSolution(QGraphicsScene *scene, const NestingSolution& solution);

private:
    static constexpr double pi = 3.14159265358979323846;
};

#endif // GEOMETRYPAINTER_H
