#ifndef LAYOUTOPTIMIZER_H
#define LAYOUTOPTIMIZER_H

#include "layoutstructures.h"
#include "geometry.h"
#include <QPainterPath>

class LayoutOptimizer {
public:
    NestingSolution optimize(const Geometry& rawGeometry, const NestingParameters& params);

private:
    struct OptimizablePart {
        int id;
        QPainterPath baseShape;
        QPainterPath expandedShape;
        double area;
        double originalMinX, originalMinY;
    };

    bool placePartOnSheet(QPainterPath& sheetOccupiedRegion,
                          const OptimizablePart& part,
                          double sheetW, double sheetH,
                          NestingSolution& solution,
                          int sheetId);
};

#endif // LAYOUTOPTIMIZER_H
