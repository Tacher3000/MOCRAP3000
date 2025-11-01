#ifndef LAYOUTSTRUCTURES_H
#define LAYOUTSTRUCTURES_H

#include "geometry.h"
#include <vector>
#include <string>
#include <map>

struct NestingParameters {
    double sheetWidth = 0.0;
    double sheetHeight = 0.0;
    double partSpacing = 0.0;
    double cutThickness = 0.0;
    // int maxQuantity = 0;

    static NestingParameters fromStrings(
        const std::string& widthStr,
        const std::string& heightStr,
        const std::string& spacingStr,
        const std::string& thicknessStr)
    {
        NestingParameters p;
        try {
            p.sheetWidth = std::stod(widthStr);
            p.sheetHeight = std::stod(heightStr);
            p.partSpacing = std::stod(spacingStr);
            p.cutThickness = std::stod(thicknessStr);
        } catch (const std::exception& e) {
            throw std::runtime_error("Invalid parameter format: " + std::string(e.what()));
        }

        if (p.sheetWidth <= 0 || p.sheetHeight <= 0 || p.partSpacing < 0 || p.cutThickness < 0) {
            throw std::runtime_error("Nesting parameters must be non-negative.");
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
    Polygon usableArea;
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

    std::map<int, Polygon> partGeometryMap;
};

#endif // LAYOUTSTRUCTURES_H
