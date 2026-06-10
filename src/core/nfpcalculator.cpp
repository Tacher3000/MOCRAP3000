#include "nfpcalculator.h"
#include <iostream>
#include <boost/polygon/polygon.hpp>
#include <limits>
#include <algorithm>

#include <clipper2/clipper.h>
#include <clipper2/clipper.minkowski.h>

using namespace boost::polygon::operators;
using namespace Clipper2Lib;

// --- Вспомогательные функции свертки ---
// Эти функции реализуют низкоуровневую математику суммы Минковского для сегментов и полигонов.

/**
 * @brief Свертка (сумма Минковского) двух отрезков.
 * Результатом сложения двух отрезков всегда является параллелограмм (или отрезок, если коллинеарны).
 */
void NFPCalculator::convolveTwoSegments(std::vector<BoostPoint>& figure, const BoostEdge& a, const BoostEdge& b) {
    using namespace boost::polygon;
    figure.clear();
    figure.push_back(BoostPoint(a.first));
    figure.push_back(BoostPoint(a.first));
    figure.push_back(BoostPoint(a.second));
    figure.push_back(BoostPoint(a.second));

    figure[0] = convolve(figure[0], b.second);
    figure[1] = convolve(figure[1], b.first);
    figure[2] = convolve(figure[2], b.first);
    figure[3] = convolve(figure[3], b.second);
}

template <typename itrT1, typename itrT2>
void NFPCalculator::convolveTwoPointSequences(BoostPolygonSet& result, itrT1 ab, itrT1 ae, itrT2 bb, itrT2 be) {
    using namespace boost::polygon;
    if(ab == ae || bb == be) return;

    BoostPoint prev_a = *ab;
    std::vector<BoostPoint> vec;
    BoostPolygon poly;
    ++ab;

    for( ; ab != ae; ++ab) {
        BoostPoint prev_b = *bb;
        itrT2 tmpb = bb;
        ++tmpb;
        for( ; tmpb != be; ++tmpb) {
            convolveTwoSegments(vec, std::make_pair(prev_b, *tmpb), std::make_pair(prev_a, *ab));
            set_points(poly, vec.begin(), vec.end());
            result.insert(poly);
            prev_b = *tmpb;
        }
        prev_a = *ab;
    }
}

template <typename itrT>
void NFPCalculator::convolvePointSequenceWithPolygons(BoostPolygonSet& result, itrT b, itrT e, const std::vector<BoostPolygonWithHoles>& polygons) {
    using namespace boost::polygon;
    for(std::size_t i = 0; i < polygons.size(); ++i) {
        convolveTwoPointSequences(result, b, e, begin_points(polygons[i]), end_points(polygons[i]));

        for(auto itrh = begin_holes(polygons[i]); itrh != end_holes(polygons[i]); ++itrh) {
            convolveTwoPointSequences(result, b, e, begin_points(*itrh), end_points(*itrh));
        }
    }
}

/**
 * @brief Полная свертка двух наборов полигонов.
 * Реализует формулу: (Edges(A) * Edges(B)) U (A * Vertices(B)) U (Vertices(A) * B)
 */
void NFPCalculator::convolveTwoPolygonSets(BoostPolygonSet& result, const BoostPolygonSet& a, const BoostPolygonSet& b) {
    using namespace boost::polygon;
    result.clear();
    std::vector<BoostPolygonWithHoles> a_polygons;
    std::vector<BoostPolygonWithHoles> b_polygons;
    a.get(a_polygons);
    b.get(b_polygons);

    for(std::size_t ai = 0; ai < a_polygons.size(); ++ai) {
        convolvePointSequenceWithPolygons(result, begin_points(a_polygons[ai]), end_points(a_polygons[ai]), b_polygons);

        for(auto itrh = begin_holes(a_polygons[ai]); itrh != end_holes(a_polygons[ai]); ++itrh) {
            convolvePointSequenceWithPolygons(result, begin_points(*itrh), end_points(*itrh), b_polygons);
        }

        for(std::size_t bi = 0; bi < b_polygons.size(); ++bi) {
            BoostPolygonWithHoles tmp_poly = a_polygons[ai];
            result.insert(convolve(tmp_poly, *(begin_points(b_polygons[bi]))));

            tmp_poly = b_polygons[bi];
            result.insert(convolve(tmp_poly, *(begin_points(a_polygons[ai]))));
        }
    }
}

// --- Основные методы ---

/**
 * @brief Вычисляет Outer NFP (Зону коллизии) через честную свертку Boost.Polygon.
 *
 * @details
 * Outer NFP для пары полигонов A и B — это множество векторов трансляции,
 * при которых A и B пересекаются. Математически это сумма Минковского A ⊕ (-B).
 */
BoostPolygonSet NFPCalculator::calculateOuterNFP(const BoostPolygonSet& A, const BoostPolygonSet& B) {
    using namespace boost::polygon;

    BoostPolygonSet B_negated;
    std::vector<BoostPolygonWithHoles> b_polys;
    B.get(b_polys);

    for (const auto& poly : b_polys) {
        std::vector<BoostPoint> pts;
        for (auto it = begin_points(poly); it != end_points(poly); ++it) {
            pts.push_back(BoostPoint(-it->x(), -it->y()));
        }
        BoostPolygon negPoly;
        set_points(negPoly, pts.begin(), pts.end());
        B_negated.insert(negPoly);
    }

    BoostPolygonSet result;
    convolveTwoPolygonSets(result, A, B_negated);
    return result;
}


BoostPolygonSet NFPCalculator::calculateInnerNFP(const BoostPolygonSet& Sheet, const BoostPolygonSet& Part) {
    using namespace Clipper2Lib;

    BoostPolygonSet Part_negated;
    std::vector<BoostPolygonWithHoles> p_polys;
    Part.get(p_polys);
    for (const auto& poly : p_polys) {
        std::vector<BoostPoint> pts;
        for (auto it = boost::polygon::begin_points(poly); it != boost::polygon::end_points(poly); ++it) {
            pts.push_back(BoostPoint(-it->x(), -it->y()));
        }

        std::ranges::reverse(pts);

        BoostPolygon negPoly;
        boost::polygon::set_points(negPoly, pts.begin(), pts.end());
        Part_negated.insert(negPoly);
    }

    Paths64 sheetPaths = toClipper(Sheet);
    Paths64 partNegPaths = toClipper(Part_negated);

    if (sheetPaths.empty() || partNegPaths.empty()) {
        return BoostPolygonSet();
    }

    Paths64 collisionZone;
    for (const auto& sPath : sheetPaths) {
        for (const auto& pPath : partNegPaths) {
            Paths64 swept = MinkowskiSum(pPath, sPath, true);
            collisionZone.insert(collisionZone.end(), swept.begin(), swept.end());
        }
    }

    collisionZone = Union(collisionZone, FillRule::NonZero);

    Rect64 sheetBounds = GetBounds(sheetPaths);
    Rect64 partBounds = GetBounds(partNegPaths);
    int64_t margin = std::max({sheetBounds.Width(), sheetBounds.Height(), partBounds.Width(), partBounds.Height()}) * 2;
    if (margin < 1000) margin = 10000;

    Path64 universePath = {
        Point64(sheetBounds.left - margin, sheetBounds.top - margin),
        Point64(sheetBounds.right + margin, sheetBounds.top - margin),
        Point64(sheetBounds.right + margin, sheetBounds.bottom + margin),
        Point64(sheetBounds.left - margin, sheetBounds.bottom + margin)
    };

    Paths64 candidates = Difference({universePath}, collisionZone, FillRule::NonZero);
    Paths64 validInnerNfp;

    for (const auto& cand : candidates) {
        if (cand.empty()) continue;

        bool touchesUniverse = false;
        for (const auto& pt : cand) {
            if (pt.x <= sheetBounds.left - margin + 100 || pt.x >= sheetBounds.right + margin - 100 ||
                pt.y <= sheetBounds.top - margin + 100 || pt.y >= sheetBounds.bottom + margin - 100) {
                touchesUniverse = true;
                break;
            }
        }
        if (touchesUniverse) continue;

        Point64 testPt = cand[0];
        Paths64 shiftedPart;
        for (const auto& pPath : partNegPaths) {
            Path64 sp;
            for (const auto& pt : pPath) {
                sp.push_back(Point64(-pt.x + testPt.x, -pt.y + testPt.y));
            }

            std::ranges::reverse(sp);

            shiftedPart.push_back(sp);
        }

        Paths64 out = Difference(shiftedPart, sheetPaths, FillRule::NonZero);

        double errorArea = 0.0;
        for (const auto& oPath : out) {
            errorArea += std::abs(Area(oPath));
        }

        if (errorArea < 1000.0) {
            validInnerNfp.push_back(cand);
        }
    }

    return fromClipper(validInnerNfp);
}

Clipper2Lib::Paths64 NFPCalculator::toClipper(const BoostPolygonSet& set) {
    namespace bp = boost::polygon;
    Clipper2Lib::Paths64 paths;
    std::vector<BoostPolygonWithHoles> polys;
    set.get(polys);

    for (const auto& poly : polys) {
        Clipper2Lib::Path64 path;
        for (auto it = bp::begin_points(poly); it != bp::end_points(poly); ++it) {
            path.push_back(Clipper2Lib::Point64(it->x(), it->y()));
        }
        if (path.size() > 0 && path.front() == path.back()) path.pop_back();
        paths.push_back(path);

        for (auto itHole = bp::begin_holes(poly); itHole != bp::end_holes(poly); ++itHole) {
            Clipper2Lib::Path64 holePath;
            for (auto it = bp::begin_points(*itHole); it != bp::end_points(*itHole); ++it) {
                holePath.push_back(Clipper2Lib::Point64(it->x(), it->y()));
            }
            if (holePath.size() > 0 && holePath.front() == holePath.back()) holePath.pop_back();
            paths.push_back(holePath);
        }
    }
    return paths;
}

BoostPolygonSet NFPCalculator::fromClipper(const Clipper2Lib::Paths64& paths) {
    BoostPolygonSet posSet;
    BoostPolygonSet negSet;

    Clipper2Lib::Paths64 unifiedPaths = Clipper2Lib::Union(paths, Clipper2Lib::FillRule::NonZero);

    for (const auto& path : unifiedPaths) {
        std::vector<BoostPoint> pts;
        pts.reserve(path.size() + 1);
        for (const auto& p : path) {
            pts.emplace_back(p.x, p.y);
        }
        if (!pts.empty() && pts.front() != pts.back()) {
            pts.push_back(pts.front());
        }

        BoostPolygon poly;
        boost::polygon::set_points(poly, pts.begin(), pts.end());

        if (Clipper2Lib::IsPositive(path)) {
            posSet.insert(poly);
        } else {
            negSet.insert(poly);
        }
    }

    posSet -= negSet;
    return posSet;
}
