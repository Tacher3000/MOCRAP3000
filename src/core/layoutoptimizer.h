#ifndef LAYOUTOPTIMIZER_H
#define LAYOUTOPTIMIZER_H

#include "layoutstructures.h"
#include "geometry.h"

class LayoutOptimizer {
public:
    /**
     * @brief Запуск основного алгоритма раскроя.
     * @param rawGeometry Исходные данные геометрии (сырые детали).
     * @param params Параметры раскроя.
     * @return Решение раскроя.
     */
    NestingSolution optimize(const Geometry& rawGeometry, const NestingParameters& params);

private:
    std::vector<NestingPart> buildProblem(const Geometry& rawGeometry, const NestingParameters& params);

    NestingSolution runPlacementEngine(
        const std::vector<NestingPart>& partsToNest,
        const NestingSheet& sheetTemplate,
        const std::map<int, Polygon>& partGeometryMap
        );

    Polygon expandPolygon(const Polygon& original, double offset);
};

#endif // LAYOUTOPTIMIZER_H
