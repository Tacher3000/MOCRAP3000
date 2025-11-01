#include "layoutoptimizer.h"
#include <iostream>
#include <algorithm>
#include <cmath>

Polygon LayoutOptimizer::expandPolygon(const Polygon& original, double offset) {
    Polygon expanded;
    // TODO: Реальная реализация Minkowski Sum для расширения полигона
    // Это сложная геометрическая операция. Для простоты здесь просто возвращаем исходный полигон.

    expanded = original;
    return expanded;
}

std::vector<NestingPart> LayoutOptimizer::buildProblem(const Geometry& rawGeometry, const NestingParameters& params) {
    std::vector<NestingPart> partsToNest;

    double offset = (params.partSpacing + params.cutThickness) / 2.0;

    for (const auto& rawPart : rawGeometry.parts) {
        if (rawPart.polygon.contours.empty()) {
            continue;
        }

        Polygon expandedPolygon = expandPolygon(rawPart.polygon, offset);

        int requiredQuantity = 1;
        try {
            size_t pos = rawPart.name.find('_');
            if (pos != std::string::npos && pos + 1 < rawPart.name.size()) {
                requiredQuantity = std::max(1, std::stoi(rawPart.name.substr(pos + 1)));
            } else {
                requiredQuantity = 1;
            }
        } catch (...) {
            requiredQuantity = 1;
        }

        for (int i = 0; i < requiredQuantity; ++i) {
            NestingPart nPart;
            nPart.partId = rawPart.id;
            nPart.geometry = expandedPolygon;
            nPart.quantity = 1;
            partsToNest.push_back(nPart);
        }
    }

    return partsToNest;
}

// Заглушка для NFP и GA
NestingSolution LayoutOptimizer::runPlacementEngine(
    const std::vector<NestingPart>& partsToNest,
    const NestingSheet& sheetTemplate,
    const std::map<int, Polygon>& partGeometryMap
    ) {
    NestingSolution solution;
    solution.partGeometryMap = partGeometryMap; // Сохраняем карту для отрисовки

    // --- Основной цикл Генетического Алгоритма ---
    // 1. Инициализация популяции (случайное размещение + ротация)
    // 2. Цикл:
    //    a. Вычисление приспособленности (Fitness): площадь/количество использованных листов
    //    b. Селекция
    //    c. Кроссовер (обмен генотипами - списками размещения)
    //    d. Мутация (случайное перемещение/ротация)
    // 3. Расчет NFP:
    //    - Для каждой новой позиции детали 'A' относительно 'B' рассчитывается NFP(A, B).
    //    - Проверка на коллизию: если центр 'A' находится внутри NFP(A, B), происходит коллизия.

    // --- Демонстрационное размещение (не алгоритм) ---
    double currentX = 10.0;
    double currentY = 10.0;
    double sheetWidth = sheetTemplate.width;

    for (const auto& part : partsToNest) {
        double partWidth = part.geometry.maxX - part.geometry.minX;
        double partHeight = part.geometry.maxY - part.geometry.minY;

        if (currentX + partWidth > sheetWidth - 10.0) {
            currentX = 10.0;
            currentY += partHeight + 10.0;
        }

        if (currentY + partHeight > sheetTemplate.height - 10.0) {
            break;
        }

        PlacedPart pPart;
        pPart.originalPartId = part.partId;
        pPart.sheetId = sheetTemplate.id;
        pPart.x = currentX - part.geometry.minX;
        pPart.y = currentY - part.geometry.minY;
        pPart.rotation = 0.0;

        solution.placedParts.push_back(pPart);
        currentX += partWidth + 10.0;
    }

    solution.usedSheets.push_back(sheetTemplate);
    solution.utilization = static_cast<double>(solution.placedParts.size()) / partsToNest.size();

    return solution;
}

NestingSolution LayoutOptimizer::optimize(const Geometry& rawGeometry, const NestingParameters& params) {
    if (rawGeometry.parts.empty()) {
        return NestingSolution{};
    }

    std::vector<NestingPart> partsToNest = buildProblem(rawGeometry, params);

    std::map<int, Polygon> originalGeometryMap;
    for (const auto& part : rawGeometry.parts) {
        originalGeometryMap[part.id] = part.polygon;
    }

    NestingSheet sheetTemplate;
    sheetTemplate.id = 1;
    sheetTemplate.width = params.sheetWidth;
    sheetTemplate.height = params.sheetHeight;

    NestingSolution result = runPlacementEngine(partsToNest, sheetTemplate, originalGeometryMap);

    return result;
}
