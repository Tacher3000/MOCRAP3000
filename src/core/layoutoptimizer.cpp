#include "layoutoptimizer.h"
#include "geometryutils.h"
#include <QTransform>
#include <QDebug>
#include <algorithm>
#include <cmath>

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

    double currentSheetWidth = params.sheets.empty() ? 1000.0 : params.sheets[0].width;
    double currentSheetHeight = params.sheets.empty() ? 1000.0 : params.sheets[0].height;

    std::vector<OptimizablePart> partsToNest;

    for (const auto& rawPart : rawGeometry.parts) {
        solution.partsMap[rawPart.id] = rawPart;

        QPainterPath path = geometry::partToPath(rawPart);
        if (path.isEmpty()) continue;

        QRectF bRect = path.boundingRect();

        QTransform norm;
        norm.translate(-bRect.left(), -bRect.top());
        QPainterPath baseShape = norm.map(path);

        QPainterPath expandedShape = geometry::expandPath(baseShape, offset);
        expandedShape.setFillRule(Qt::WindingFill);

        double area = bRect.width() * bRect.height();

        partsToNest.push_back({
            rawPart.id,
            baseShape,
            expandedShape,
            area,
            bRect.left(),
            bRect.top()
        });
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
        sheets.back().occupied.setFillRule(Qt::WindingFill);
        return sheets.back();
    };

    double coarseGridStep = std::max(1.0, params.partSpacing);

    for (const auto& part : partsToNest) {
        bool placed = false;

        for (auto& sheet : sheets) {
            if (placePartOnSheet(sheet.occupied, part, currentSheetWidth, currentSheetHeight,
                                 solution, sheet.id, coarseGridStep)) {
                placed = true;
                break;
            }
        }

        if (!placed) {
            auto& newSheet = addSheet();
            if (!placePartOnSheet(newSheet.occupied, part, currentSheetWidth, currentSheetHeight,
                                  solution, newSheet.id, coarseGridStep)) {
                qWarning() << "Part ID" << part.id << "does not fit on the sheet!";
            }
        }
    }

    for (const auto& s : sheets) {
        solution.usedSheets.push_back({s.id, currentSheetWidth, currentSheetHeight});
    }

    double totalArea = sheets.size() * currentSheetWidth * currentSheetHeight;
    solution.utilization = (totalArea > 0) ? (static_cast<double>(solution.placedParts.size()) * 100.0 / totalArea) : 0.0;

    return solution;
}


bool LayoutOptimizer::placePartOnSheet(QPainterPath& occupied, const OptimizablePart& part,
                                       double sheetW, double sheetH,
                                       NestingSolution& solution, int sheetId,
                                       double gridStep)
{
    const std::vector<double> rotations = {0.0, 90.0, 180.0, 270.0};

    struct Position {
        double x, y, rot;
        double penalty;
        QRectF rotatedRect;
    };

    Position bestPos = {0, 0, 0, std::numeric_limits<double>::max(), QRectF()};
    bool foundAny = false;

    auto checkCollision = [&](double x, double y, const QPainterPath& shape, const QRectF& rRect) -> bool {
        if (x < 0 || y < 0 || x + rRect.width() > sheetW || y + rRect.height() > sheetH) return true;
        if (occupied.intersects(QRectF(x, y, rRect.width(), rRect.height()))) {
            QPainterPath placed = shape.translated(x, y);
            placed.setFillRule(Qt::WindingFill);
            if (occupied.intersects(placed)) return true;
        }
        return false;
    };

    auto squeezePart = [&](double startX, double startY, const QPainterPath& shape, const QRectF& rRect) -> QPointF {
        double currX = startX;
        double currY = startY;
        double step = gridStep;
        while (step > 0.05) {
            if (!checkCollision(currX - step, currY, shape, rRect)) currX -= step;
            else step /= 2.0;
        }
        step = gridStep;
        while (step > 0.05) {
            if (!checkCollision(currX, currY - step, shape, rRect)) currY -= step;
            else step /= 2.0;
        }
        return QPointF(currX, currY);
    };

    for (double rot : rotations) {
        QTransform t;
        t.rotate(rot);
        QPainterPath rotExpanded = t.map(part.expandedShape);
        rotExpanded.setFillRule(Qt::WindingFill);
        QRectF rRect = rotExpanded.boundingRect();
        QTransform align;
        align.translate(-rRect.left(), -rRect.top());
        QPainterPath checkShape = align.map(rotExpanded);
        double width = rRect.width();
        double height = rRect.height();

        if (width > sheetW || height > sheetH) continue;

        for (double y = 0; y <= sheetH - height; y += gridStep) {
            if (foundAny && (y * sheetW) > bestPos.penalty) break;
            for (double x = 0; x <= sheetW - width; x += gridStep) {
                double coarsePenalty = y * sheetW + x;
                if (foundAny && coarsePenalty >= bestPos.penalty) continue;

                if (!checkCollision(x, y, checkShape, rRect)) {
                    QPointF squeezed = squeezePart(x, y, checkShape, rRect);
                    double finalPenalty = squeezed.y() * sheetW + squeezed.x();
                    if (finalPenalty < bestPos.penalty) {
                        bestPos = {squeezed.x(), squeezed.y(), rot, finalPenalty, rRect};
                        foundAny = true;
                    }
                }
            }
        }
    }

    if (foundAny) {
        PlacedPart pp;
        pp.originalPartId = part.id;
        pp.sheetId = sheetId;
        pp.rotation = bestPos.rot;
        pp.x = bestPos.x - bestPos.rotatedRect.left();
        pp.y = bestPos.y - bestPos.rotatedRect.top();
        solution.placedParts.push_back(pp);

        QTransform t;
        t.rotate(bestPos.rot);
        QPainterPath rotExpanded = t.map(part.expandedShape);
        QTransform finalT;
        finalT.translate(pp.x, pp.y);
        QPainterPath placedShape = finalT.map(rotExpanded);
        placedShape.setFillRule(Qt::WindingFill);
        occupied.addPath(placedShape);
        return true;
    }
    return false;
}
