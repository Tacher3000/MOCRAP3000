#include "layoutoptimizer.h"
#include "geometryutils.h"
#include <QTransform>
#include <QDebug>
#include <algorithm>
#include <cmath>

NestingSolution LayoutOptimizer::optimize(const Geometry& rawGeometry, const NestingParameters& params) {
    if (rawGeometry.parts.empty()) return {};

    NestingSolution solution;
    double offset = (params.partSpacing + params.cutThickness) / 2.0;

    std::vector<OptimizablePart> partsToNest;
    int partsCountPerUniqueGeometry = params.partCount;

    for (const auto& rawPart : rawGeometry.parts) {
        solution.partsMap[rawPart.id] = rawPart;

        QPainterPath path = geometry::partToPath(rawPart);

        if (path.isEmpty()) continue;

        QRectF bRect = path.boundingRect();

        QTransform norm;
        norm.translate(-bRect.left(), -bRect.top());
        QPainterPath baseShape = norm.map(path);

        QPainterPath expandedShape = geometry::expandPath(baseShape, offset);
        double area = bRect.width() * bRect.height();

        for(int i = 0; i < partsCountPerUniqueGeometry; ++i) {
            partsToNest.push_back({
                rawPart.id,
                baseShape,
                expandedShape,
                area,
                bRect.left(),
                bRect.top()
            });
        }
    }

    std::sort(partsToNest.begin(), partsToNest.end(), [](const auto& a, const auto& b) {
        return a.area > b.area;
    });

    struct SheetData {
        int id;
        QPainterPath occupied;
    };
    std::vector<SheetData> sheets;

    auto addSheet = [&]() -> SheetData& {
        sheets.push_back({ (int)sheets.size() + 1, QPainterPath() });
        return sheets.back();
    };

    for (const auto& part : partsToNest) {
        bool placed = false;

        for (auto& sheet : sheets) {
            if (placePartOnSheet(sheet.occupied, part, params.sheetWidth, params.sheetHeight, solution, sheet.id)) {
                placed = true;
                break;
            }
        }

        if (!placed) {
            auto& newSheet = addSheet();
            if (!placePartOnSheet(newSheet.occupied, part, params.sheetWidth, params.sheetHeight, solution, newSheet.id)) {
                qWarning() << "Part ID" << part.id << "does not fit on the sheet!";
            }
        }
    }

    for (const auto& s : sheets) {
        solution.usedSheets.push_back({s.id, params.sheetWidth, params.sheetHeight});
    }

    double totalArea = sheets.size() * params.sheetWidth * params.sheetHeight;
    double usedArea = 0;

    solution.utilization = (totalArea > 0) ? (static_cast<double>(solution.placedParts.size()) * 100.0 / totalArea) : 0.0;

    return solution;
}

bool LayoutOptimizer::placePartOnSheet(QPainterPath& occupied, const OptimizablePart& part,
                                       double sheetW, double sheetH,
                                       NestingSolution& solution, int sheetId)
{
    const double gridStep = 5.0;
    const std::vector<double> rotations = {0.0, 90.0, 180.0, 270.0};

    struct Position {
        double x, y, rot;
        double penalty;
    };

    Position bestPos = {0, 0, 0, std::numeric_limits<double>::max()};
    bool found = false;

    for (double rot : rotations) {
        QTransform t;
        t.rotate(rot);


        QPainterPath rotExpanded = t.map(part.expandedShape);
        QRectF rRect = rotExpanded.boundingRect();

        QTransform align;
        align.translate(-rRect.left(), -rRect.top());
        QPainterPath checkShape = align.map(rotExpanded);

        double width = rRect.width();
        double height = rRect.height();

        if (width > sheetW || height > sheetH) continue;

        for (double y = 0; y <= sheetH - height; y += gridStep) {
            if ((y * sheetW) > bestPos.penalty) break;

            for (double x = 0; x <= sheetW - width; x += gridStep) {
                double penalty = y * sheetW + x;

                if (penalty >= bestPos.penalty) continue;

                if (occupied.intersects(QRectF(x, y, width, height))) {
                    QPainterPath placed = checkShape.translated(x, y);
                    if (occupied.intersects(placed)) {
                        continue; // Занято
                    }
                }

                bestPos = {x, y, rot, penalty};
                found = true;

                goto next_rotation;
            }
        }
    next_rotation:;
    }

    if (found) {
        PlacedPart pp;
        pp.originalPartId = part.id;
        pp.sheetId = sheetId;
        pp.x = bestPos.x;
        pp.y = bestPos.y;
        pp.rotation = bestPos.rot;
        solution.placedParts.push_back(pp);

        QTransform t;
        t.rotate(bestPos.rot);
        QPainterPath rotExpanded = t.map(part.expandedShape);
        QRectF rRect = rotExpanded.boundingRect();

        QTransform finalT;
        finalT.translate(bestPos.x - rRect.left(), bestPos.y - rRect.top());
        QPainterPath placedShape = finalT.map(rotExpanded);

        occupied = occupied.united(placedShape);
        return true;
    }

    return false;
}
