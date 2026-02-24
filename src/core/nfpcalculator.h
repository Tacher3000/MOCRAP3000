#ifndef NFP_CALCULATOR_H
#define NFP_CALCULATOR_H

#include "geometryadapter.h"
#include <vector>
#include <clipper2/clipper.h>

class NFPCalculator {
public:
    // Используем высокий скейл как в Deepnest (10^7) для точности
    static constexpr double NFP_SCALE = 10000000.0;

    /**
     * @brief Вычисляет Outer NFP (No Fit Polygon).
     * Это зона вокруг детали A, куда нельзя ставить деталь B (чтобы они не пересекались).
     * Математически: Minkowski Sum (A, -B).
     */
    static BoostPolygonSet calculateOuterNFP(const BoostPolygonSet& A, const BoostPolygonSet& B);

    /**
     * @brief Вычисляет Inner NFP (Inner Fit Polygon).
     * Это зона ВНУТРИ детали A (обычно листа), куда можно поставить деталь B.
     * Реализовано через инверсию пространства (Universe Subtraction).
     */
    static BoostPolygonSet calculateInnerNFP(const BoostPolygonSet& Sheet, const BoostPolygonSet& Part);

    /**
     * @brief Утилита для конвертации Boost Polygon -> Clipper2 Path
     */
    static Clipper2Lib::Paths64 toClipper(const BoostPolygonSet& set);

    /**
     * @brief Утилита для конвертации Clipper2 Path -> Boost Polygon
     */
    static BoostPolygonSet fromClipper(const Clipper2Lib::Paths64& paths);

private:
    typedef std::pair<BoostPoint, BoostPoint> BoostEdge;

    // Реализация свертки (Convolution) сегментов и полигонов (порт из minkowski.cc)
    static void convolveTwoSegments(std::vector<BoostPoint>& figure, const BoostEdge& a, const BoostEdge& b);

    template <typename itrT1, typename itrT2>
    static void convolveTwoPointSequences(BoostPolygonSet& result, itrT1 ab, itrT1 ae, itrT2 bb, itrT2 be);

    template <typename itrT>
    static void convolvePointSequenceWithPolygons(BoostPolygonSet& result, itrT b, itrT e, const std::vector<BoostPolygonWithHoles>& polygons);

    static void convolveTwoPolygonSets(BoostPolygonSet& result, const BoostPolygonSet& a, const BoostPolygonSet& b);
};

#endif // NFP_CALCULATOR_H
