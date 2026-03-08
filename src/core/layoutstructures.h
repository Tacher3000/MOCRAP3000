#ifndef LAYOUTSTRUCTURES_H
#define LAYOUTSTRUCTURES_H

#include "geometry.h"
#include <vector>
#include <string>
#include <map>
#include <stdexcept>

struct SheetRequest {
    double width = 0.0;
    double height = 0.0;
    int quantity = 1;
    bool isInfinite = false;
};

struct NestingParameters {
    std::vector<SheetRequest> sheets; // Теперь здесь список листов!
    double partSpacing = 0.0;
    double cutThickness = 0.0;

    static NestingParameters fromStrings(
        const std::string& spacingStr,
        const std::string& thicknessStr)
    {
        NestingParameters p;
        try {
            p.partSpacing = std::stod(spacingStr);
            p.cutThickness = std::stod(thicknessStr);
        } catch (const std::exception& e) {
            throw std::runtime_error("Invalid parameter format: " + std::string(e.what()));
        }
        return p;
    }
};

struct NestingPart {
    int partId = 0;
    Polygon geometry;
    int quantity = 1;
};

struct NestingSheet {
    int id = 0;
    double width = 0.0;
    double height = 0.0;
};

struct PlacedPart {
    int originalPartId = 0;
    int sheetId = 0;
    double x = 0.0;
    double y = 0.0;
    double rotation = 0.0;
};

struct NestingSolution {
    std::vector<PlacedPart> placedParts;
    std::vector<NestingSheet> usedSheets;
    double utilization = 0.0;
    int generation = 0;

    std::map<int, Part> partsMap;
};

#endif // LAYOUTSTRUCTURES_H
