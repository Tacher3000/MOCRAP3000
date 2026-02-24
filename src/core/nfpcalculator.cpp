#include "nfpcalculator.h"
#include <iostream>
#include <boost/polygon/polygon.hpp>
#include <limits>
#include <algorithm>

#include <clipper2/clipper.h>
#include <clipper2/clipper.minkowski.h>

using namespace boost::polygon::operators;
using namespace Clipper2Lib;

// --- Вспомогательные функции свертки (Порт minkowski.cc) ---
// Эти функции реализуют низкоуровневую математику суммы Минковского для сегментов и полигонов.

/**
 * @brief Свертка (сумма Минковского) двух отрезков.
 * Результатом сложения двух отрезков всегда является параллелограмм (или отрезок, если коллинеарны).
 */
void NFPCalculator::convolveTwoSegments(std::vector<BoostPoint>& figure, const BoostEdge& a, const BoostEdge& b) {
    using namespace boost::polygon;
    figure.clear();
    // Инициализируем 4 точки параллелограмма
    figure.push_back(BoostPoint(a.first));
    figure.push_back(BoostPoint(a.first));
    figure.push_back(BoostPoint(a.second));
    figure.push_back(BoostPoint(a.second));

    // Сумма векторов (Minkowski sum для точек): p_new = p_a + p_b
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

    // Проходим по всем сегментам полигона A
    for( ; ab != ae; ++ab) {
        BoostPoint prev_b = *bb;
        itrT2 tmpb = bb;
        ++tmpb;
        // Проходим по всем сегментам полигона B
        for( ; tmpb != be; ++tmpb) {
            // Складываем каждый сегмент с каждым
            convolveTwoSegments(vec, std::make_pair(prev_b, *tmpb), std::make_pair(prev_a, *ab));
            set_points(poly, vec.begin(), vec.end());
            result.insert(poly); // Объединяем результат
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
        // Свертка контура A со всеми контурами B
        convolvePointSequenceWithPolygons(result, begin_points(a_polygons[ai]), end_points(a_polygons[ai]), b_polygons);

        // Учет дырок в A
        for(auto itrh = begin_holes(a_polygons[ai]); itrh != end_holes(a_polygons[ai]); ++itrh) {
            convolvePointSequenceWithPolygons(result, begin_points(*itrh), end_points(*itrh), b_polygons);
        }

        // Заполняем "дырки" внутри получившейся фигуры, складывая исходные полигоны с вершинами
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
 * @brief Вычисляет Outer NFP (Зону коллизии).
 * * @details
 * Outer NFP для пары полигонов A и B — это множество векторов трансляции $v$,
 * таких что $A \cap (B + v) \neq \emptyset$.
 * Алгоритм:
 * 1. Инвертировать полигон B (отразить относительно 0,0). $x' = -x, y' = -y$.
 * 2. Вычислить сумму Минковского $A \oplus (-B)$.
 * * @param A Стационарная деталь (препятствие).
 * @param B Деталь, которую мы хотим разместить (orbiting part).
 * @return BoostPolygonSet Полигон, описывающий запрещенные позиции для центра детали B.
 */
// BoostPolygonSet NFPCalculator::calculateOuterNFP(const BoostPolygonSet& A, const BoostPolygonSet& B) {
//     using namespace boost::polygon;

//     // 1. Отражение B (Negate B)
//     BoostPolygonSet B_negated;
//     std::vector<BoostPolygon> b_polys;
//     B.get(b_polys);

//     for (auto& poly : b_polys) {
//         std::vector<BoostPoint> pts;
//         for (auto it = begin_points(poly); it != end_points(poly); ++it) {
//             pts.push_back(BoostPoint(-it->x(), -it->y()));
//         }
//         BoostPolygon negPoly;
//         set_points(negPoly, pts.begin(), pts.end());
//         B_negated.insert(negPoly);
//     }

//     // 2. Свертка (Minkowski Sum)
//     BoostPolygonSet result;
//     convolveTwoPolygonSets(result, A, B_negated);

//     return result;
// }

/**
 * @brief Вычисляет Outer NFP (Зону коллизии).
 * Реализация через Clipper2 MinkowskiSum.
 */
BoostPolygonSet NFPCalculator::calculateOuterNFP(const BoostPolygonSet& A, const BoostPolygonSet& B) {
    using namespace boost::polygon;

    // ЛОГИРОВАНИЕ: Раскомментируйте для отладки
    // qDebug() << "  [NFP] Start Calculate Outer NFP...";
    // QTime timer; timer.start();

    // 1. Отражение B (Negate B) - как и раньше, NFP(A, B) = Minkowski(A, -B)
    BoostPolygonSet B_negated;
    std::vector<BoostPolygon> b_polys;
    B.get(b_polys);

    for (auto& poly : b_polys) {
        std::vector<BoostPoint> pts;
        for (auto it = begin_points(poly); it != end_points(poly); ++it) {
            pts.push_back(BoostPoint(-it->x(), -it->y()));
        }
        BoostPolygon negPoly;
        set_points(negPoly, pts.begin(), pts.end());
        B_negated.insert(negPoly);
    }

    // 2. Конвертация в Clipper2
    Paths64 sub = toClipper(A);         // Препятствие (Subject)
    Paths64 pat = toClipper(B_negated); // Деталь (Pattern)

    if (sub.empty() || pat.empty()) return BoostPolygonSet();

    // 3. Сумма Минковского через Clipper2
    Paths64 solution;

    // Clipper2 MinkowskiSum принимает Pattern и Path.
    // Нам нужно просуммировать каждый контур A с каждым контуром B.
    // Обычно MinkowskiSum работает "Pattern (замкнутый) + Path (открытый или замкнутый)".
    // Здесь мы используем перегрузку: MinkowskiSum(Pattern, Path, isClosed)

    for (const auto& pathA : sub) {
        for (const auto& pathB : pat) {
            // ВАЖНО: MinkowskiSum может быть реализован как шаблон или функция.
            // Если здесь будет ошибка компиляции, значит сигнатура отличается.
            Paths64 tmp = MinkowskiSum(pathB, pathA, true);
            solution.insert(solution.end(), tmp.begin(), tmp.end());
        }
    }

    // 4. Объединение (Union) результата, чтобы убрать самопересечения
    Paths64 unifiedSolution = Union(solution, FillRule::NonZero);

    // 5. Конвертация обратно в Boost
    BoostPolygonSet result = fromClipper(unifiedSolution);

    // if (timer.elapsed() > 100) {
    //    qDebug() << "  [NFP] Slow NFP calc:" << timer.elapsed() << "ms. Result polys:" << unifiedSolution.size();
    // }

    return result;
}

/**
 * @brief Вычисляет Inner NFP (Зону допустимого размещения внутри листа).
 * * @details
 * Использует метод вычитания "Универсума" (Universe Subtraction).
 * Напрямую вычислить Minkowski Difference (A - B) для полигонов сложно.
 * Проще найти зону, где деталь ВЫЛЕЗАЕТ за лист, и вычесть её из всего пространства.
 * * Формула:
 * $InnerNFP = Universe \setminus ( (Universe \setminus Sheet) \oplus (-Part) )$
 * Где:
 * - $Universe$ — огромное ограничивающее пространство.
 * - $Universe \setminus Sheet$ — инверсия листа (дырка в форме листа).
 * - $\oplus (-Part)$ — "раздуваем" границы этой дырки на размер детали.
 * * В GeneticOptimizer для простых прямоугольных листов используется упрощенная логика (просто уменьшение rect),
 * но этот метод нужен для листов сложной формы (с вырезами).
 */
// BoostPolygonSet NFPCalculator::calculateInnerNFP(const BoostPolygonSet& Sheet, const BoostPolygonSet& Part) {
//     using namespace boost::polygon;

//     // 1. Отражаем деталь (так как sliding window требует инверсии для свертки)
//     BoostPolygonSet Part_negated;
//     std::vector<BoostPolygon> b_polys;
//     Part.get(b_polys);
//     for (auto& poly : b_polys) {
//         std::vector<BoostPoint> pts;
//         for (auto it = begin_points(poly); it != end_points(poly); ++it) {
//             pts.push_back(BoostPoint(-it->x(), -it->y()));
//         }
//         BoostPolygon negPoly;
//         set_points(negPoly, pts.begin(), pts.end());
//         Part_negated.insert(negPoly);
//     }

//     // 2. Определяем Универсум
//     rectangle_data<long long> boundsA, boundsB;
//     extents(boundsA, Sheet);
//     extents(boundsB, Part);

//     // Огромный отступ, гарантирующий перекрытие
//     long long widthA = xh(boundsA) - xl(boundsA);
//     long long heightA = yh(boundsA) - yl(boundsA);
//     long long margin = std::max({widthA, heightA}) * 10;

//     rectangle_data<long long> universeRect = boundsA;
//     bloat(universeRect, margin); // Расширяем границы

//     BoostPolygonSet universe;
//     universe.insert(universeRect);

//     // 3. Инверсия листа (Inverse Sheet)
//     BoostPolygonSet Sheet_Inverse = universe - Sheet;

//     // 4. "Раздуваем" инверсию (зону "не-листа") на размер детали
//     // NoFitZone — это зона, где центр детали находится "слишком близко" к границе листа или снаружи.
//     BoostPolygonSet NoFitZone;
//     convolveTwoPolygonSets(NoFitZone, Sheet_Inverse, Part_negated);

//     // 5. Допустимая зона = Универсум - Запретная зона
//     BoostPolygonSet InnerFit = universe - NoFitZone;

//     return InnerFit;
// }

BoostPolygonSet NFPCalculator::calculateInnerNFP(const BoostPolygonSet& Sheet, const BoostPolygonSet& Part) {
    using namespace boost::polygon;

    // qDebug() << "  [NFP] Calculate Inner NFP...";

    // 1. Отражаем деталь
    BoostPolygonSet Part_negated;
    std::vector<BoostPolygon> b_polys;
    Part.get(b_polys);
    for (auto& poly : b_polys) {
        std::vector<BoostPoint> pts;
        for (auto it = begin_points(poly); it != end_points(poly); ++it) {
            pts.push_back(BoostPoint(-it->x(), -it->y()));
        }
        BoostPolygon negPoly;
        set_points(negPoly, pts.begin(), pts.end());
        Part_negated.insert(negPoly);
    }

    // 2. Определяем Универсум (Bounding Box)
    rectangle_data<long long> boundsA;
    extents(boundsA, Sheet);

    // Увеличиваем универсум
    long long width = xh(boundsA) - xl(boundsA);
    long long height = yh(boundsA) - yl(boundsA);
    long long margin = std::max(width, height) * 10;
    if (margin == 0) margin = 1000000;

    rectangle_data<long long> universeRect = boundsA;
    bloat(universeRect, margin);

    BoostPolygonSet universe;
    universe.insert(universeRect);

    // 3. Инверсия листа
    BoostPolygonSet Sheet_Inverse = universe - Sheet;

    // 4. Раздуваем инверсию листа на размер детали (Minkowski Sum)
    // NoFitZone = Minkowski(Sheet_Inverse, Part)
    // (см. пояснение в предыдущем ответе: calculateOuterNFP делает Negate внутри,
    // а нам нужно A + (-B), где B = Part_negated.
    // Чтобы получить A + Part_negated, мы передаем в функцию A и Part_original.
    // Функция сделает A + (-Part_original) = A + Part_negated. Все верно.)

    BoostPolygonSet NoFitZone = calculateOuterNFP(Sheet_Inverse, Part);

    // 5. Допустимая зона
    BoostPolygonSet InnerFit = universe - NoFitZone;

    return InnerFit;
}

// --- Clipper2 Conversion Helpers ---

/**
 * @brief Конвертация из BoostPolygonSet в Clipper2 Paths64.
 * * @details
 * Boost Polygon использует замкнутые контуры (первая точка == последняя).
 * Clipper2 предпочитает незамкнутые массивы точек (замыкание подразумевается).
 * Метод удаляет последнюю точку, если она дублирует первую.
 */
// Clipper2Lib::Paths64 NFPCalculator::toClipper(const BoostPolygonSet& set) {
//     namespace bp = boost::polygon;
//     Clipper2Lib::Paths64 paths;
//     std::vector<BoostPolygonWithHoles> polys;
//     set.get(polys);

//     for (const auto& poly : polys) {
//         // Внешний контур
//         Clipper2Lib::Path64 path;
//         for (auto it = bp::begin_points(poly); it != bp::end_points(poly); ++it) {
//             path.push_back(Clipper2Lib::Point64(it->x(), it->y()));
//         }
//         // Удаляем дубликат замыкания
//         if (path.size() > 0 && path.front() == path.back()) {
//             path.pop_back();
//         }
//         paths.push_back(path);

//         // Внутренние отверстия
//         for (auto itHole = bp::begin_holes(poly); itHole != bp::end_holes(poly); ++itHole) {
//             Clipper2Lib::Path64 holePath;
//             for (auto it = bp::begin_points(*itHole); it != bp::end_points(*itHole); ++it) {
//                 holePath.push_back(Clipper2Lib::Point64(it->x(), it->y()));
//             }
//             if (holePath.size() > 0 && holePath.front() == holePath.back()) {
//                 holePath.pop_back();
//             }
//             paths.push_back(holePath);
//         }
//     }
//     return paths;
// }

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

/**
 * @brief Конвертация из Clipper2 Paths64 в BoostPolygonSet.
 * * @details
 * Восстанавливает замыкание полигонов (добавляет точку в конец),
 * так как Boost Polygon требует явного замыкания.
 */
// BoostPolygonSet NFPCalculator::fromClipper(const Clipper2Lib::Paths64& paths) {
//     BoostPolygonSet set;
//     for (const auto& path : paths) {
//         std::vector<BoostPoint> pts;
//         for (const auto& p : path) {
//             pts.push_back(BoostPoint(p.x, p.y));
//         }
//         // Boost ожидает замкнутые полигоны
//         if (!pts.empty() && pts.front() != pts.back()) {
//             pts.push_back(pts.front());
//         }

//         BoostPolygon poly;
//         boost::polygon::set_points(poly, pts.begin(), pts.end());
//         set.insert(poly);
//     }
//     return set;
// }

BoostPolygonSet NFPCalculator::fromClipper(const Clipper2Lib::Paths64& paths) {
    BoostPolygonSet set;
    for (const auto& path : paths) {
        std::vector<BoostPoint> pts;
        for (const auto& p : path) {
            pts.push_back(BoostPoint(p.x, p.y));
        }
        if (!pts.empty() && pts.front() != pts.back()) pts.push_back(pts.front());

        BoostPolygon poly;
        boost::polygon::set_points(poly, pts.begin(), pts.end());
        set.insert(poly);
    }
    return set;
}
