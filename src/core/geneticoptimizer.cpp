#include "geneticoptimizer.h"
#include "geometryadapter.h"
#include "nfpcalculator.h"
#include <clipper2/clipper.h>

#include <algorithm>
#include <iostream>
#include <numeric>
#include <QDebug>
#include <map>
#include <random>
#include <cmath>
#include <omp.h>
#include <set>

using namespace Clipper2Lib;

/**
 * @brief Вспомогательная функция для перемещения набора полигонов Boost.
 *
 * @details
 * boost::polygon::move работает с ссылками на примитивы.
 * BoostPolygonSet является контейнером (polygon_set_data), поэтому стандартный move
 * для него не перегружен так, как мы ожидаем для "сдвига всех точек".
 * Эта функция извлекает полигоны, сдвигает их и собирает обратно.
 *
 * @param set Набор полигонов для перемещения (in/out).
 * @param dx Сдвиг по X (в единицах Boost coordinates).
 * @param dy Сдвиг по Y (в единицах Boost coordinates).
 */
static void moveBoostSet(BoostPolygonSet& set, int dx, int dy) {
    namespace bp = boost::polygon;
    std::vector<BoostPolygonWithHoles> polys;
    set.get(polys);
    set.clear();

    for(auto& p : polys) {
        bp::move(p, bp::HORIZONTAL, dx);
        bp::move(p, bp::VERTICAL, dy);
        set.insert(p);
    }
}

/**
 * @brief Конструктор оптимизатора.
 * Инициализирует генератор случайных чисел.
 */
GeneticOptimizer::GeneticOptimizer() {
    std::random_device rd;
    m_rng.seed(rd());
}

/**
 * @brief Основной цикл генетического алгоритма.
 *
 * @details
 * Логика выполнения (Call Flow):
 * 1. **Подготовка геометрии:**
 * - Конвертация всех деталей в формат Boost.Polygon.
 * - Пре-калькуляция 4-х вращений (0, 90, 180, 270) для каждой детали, чтобы не делать это в цикле.
 * - Нормализация (сдвиг в 0,0) для упрощения NFP вычислений.
 * 2. **Инициализация популяции:** Создание набора случайных перестановок порядка деталей.
 * 3. **Оценка (Evaluation):** Запуск `decode()` для каждого индивида (параллельно).
 * 4. **Эволюционный цикл:**
 * - Элитизм (сохранение лучшего).
 * - Селекция (Турнирный отбор).
 * - Скрещивание (Order Crossover - сохраняет последовательность генов).
 * - Мутация (Swap или изменение вращения).
 * - Оценка новой популяции.
 * 5. **Завершение:** Возврат лучшего найденного решения при остановке или завершении поколений.
 *
 * @param parts Вектор исходных деталей.
 * @param params Параметры раскроя (размер листа, отступы).
 * @param stopFlag Атомарный флаг для прерывания из GUI потока.
 * @param progressCallback Функция обратного вызова для отрисовки прогресса.
 * @return NestingSolution Лучшее найденное решение.
 */
NestingSolution GeneticOptimizer::optimize(const std::vector<Part>& parts,
                                           const NestingParameters& params,
                                           std::atomic<bool>& stopFlag,
                                           std::function<void(const NestingSolution&)> progressCallback)
{

    // qDebug() << "=== Starting Optimization ===";
    // qDebug() << "Parts count:" << parts.size();
    if (!params.sheets.empty()) {
        // qDebug() << "Params (First sheet):" << params.sheets[0].width << "x" << params.sheets[0].height;
    }    if (parts.empty()) return {};

    m_config.populationSize = 20;

    double offsetAmount = (params.partSpacing + params.cutThickness) / 2.0;
    int boostOffset = static_cast<int>(offsetAmount * NFPCalculator::NFP_SCALE);

    qDebug() << "Corrected boostOffset:" << boostOffset;

    // --- 1. ВЫДЕЛЕНИЕ УНИКАЛЬНЫХ ДЕТАЛЕЙ ---
    // Так как при увеличении количества детали просто копируются с тем же ID,
    // мы собираем список только уникальных форм, чтобы не делать лишнюю работу.
    std::vector<Part> uniqueParts;
    std::set<int> processedIds;
    for (const auto& part : parts) {
        if (processedIds.find(part.id) == processedIds.end()) {
            uniqueParts.push_back(part);
            processedIds.insert(part.id);
        }
    }

    // --- 2. КЭШИРОВАНИЕ ГЕОМЕТРИИ ---
    int rotCount = std::max(1, params.allowedRotations);
    double angleStep = 360.0 / rotCount;
    std::map<int, std::vector<BoostPolygonSet>> rotatedPartsCache;

    for(const auto& part : uniqueParts) {
        BoostPolygonSet base = GeometryAdapter::toBoost(part);

        if (boostOffset > 0) {
            Paths64 clipBase = NFPCalculator::toClipper(base);
            clipBase = InflatePaths(clipBase, boostOffset, JoinType::Round, EndType::Polygon);
            base = NFPCalculator::fromClipper(clipBase);
        }

        normalizePolySet(base);

        rotatedPartsCache[part.id].resize(rotCount);
        for(int r = 0; r < rotCount; ++r) {
            if (r == 0) {
                rotatedPartsCache[part.id][r] = base;
            } else {
                rotatedPartsCache[part.id][r] = rotatePolySet(base, r * angleStep);
                normalizePolySet(rotatedPartsCache[part.id][r]);
            }
        }
    }

    m_nfpCache.clear();
    m_innerNfpCache.clear();

    qDebug() << "Инициализация первичной популяции";
    initializePopulation(parts, m_config.populationSize, rotCount);

    // qDebug() << "Оценка первой популяции";
    evaluatePopulation(m_population, parts, params, rotatedPartsCache, stopFlag);

    Individual bestInd = m_population[0];
    if (progressCallback) progressCallback(bestInd.cachedSolution);

    int gen = 0;
    for (; gen < m_config.maxGenerations; ++gen) {
        if (stopFlag) {
            // qDebug() << "Stop requested by user at generation" << gen;
            break;
        }
        // qDebug() << "Processing Generation:" << gen;

        std::vector<Individual> newPop;
        newPop.reserve(m_config.populationSize);
        newPop.push_back(bestInd);

        while (newPop.size() < static_cast<size_t>(m_config.populationSize)) {
            Individual p1 = selection(m_population);
            Individual p2 = selection(m_population);

            std::pair<Individual, Individual> children = crossover(p1, p2);

            mutate(children.first, rotCount);
            mutate(children.second, rotCount);

            newPop.push_back(children.first);
            if(newPop.size() < static_cast<size_t>(m_config.populationSize))
                newPop.push_back(children.second);
        }

        m_population = std::move(newPop);

        // qDebug() << "Evaluating population...";
        evaluatePopulation(m_population, parts, params, rotatedPartsCache, stopFlag);
        if (stopFlag) break;
        // qDebug() << "Evaluation finished.";


        auto it = std::min_element(m_population.begin(), m_population.end(),
                                   [](const Individual& a, const Individual& b) {
                                       return a.fitness < b.fitness;
                                   });

        if (it->fitness < bestInd.fitness) {
            bestInd = *it;
            if (progressCallback) {
                NestingSolution sol = bestInd.cachedSolution;
                sol.generation = gen + 1;
                progressCallback(sol);
            }
        }

        // TODO: Реализовать "Random Restart" (сброс популяции), если локальный минимум держится долго.
    }

    return bestInd.cachedSolution;
}

/**
 * @brief Создает начальную популяцию.
 * @details
 * - Первый индивид ("Адам") создается по эвристике "Сначала крупные". Это дает хороший старт.
 * - Остальные индивиды создаются случайным перемешиванием порядка и вращений.
 */
void GeneticOptimizer::initializePopulation(const std::vector<Part>& parts, int popSize, int allowedRotations) {
    m_population.clear();
    int numParts = static_cast<int>(parts.size());

    struct PartStat { int id; double area; double length; };
    std::vector<PartStat> stats(numParts);

    for (int i = 0; i < numParts; ++i) {
        BoostPolygonSet ps = GeometryAdapter::toBoost(parts[i]);
        namespace bp = boost::polygon;
        bp::rectangle_data<int> r;
        bp::extents(r, ps);
        stats[i] = {
            i,
            static_cast<double>(bp::area(ps)),
            static_cast<double>(std::max(bp::xh(r) - bp::xl(r), bp::yh(r) - bp::yl(r)))
        };
    }

    Individual baseInd;
    baseInd.partIndices.resize(numParts);
    baseInd.rotations.resize(numParts, 0);

    auto areaSorted = stats;
    std::ranges::sort(areaSorted, std::greater<>{}, &PartStat::area);
    for (int i = 0; i < numParts; ++i) baseInd.partIndices[i] = areaSorted[i].id;
    m_population.push_back(baseInd);

    auto lengthSorted = stats;
    std::ranges::sort(lengthSorted, std::greater<>{}, &PartStat::length);
    for (int i = 0; i < numParts; ++i) baseInd.partIndices[i] = lengthSorted[i].id;
    m_population.push_back(baseInd);

    std::uniform_int_distribution<int> rotDist(0, std::max(0, allowedRotations - 1));
    for (int i = 2; i < popSize; ++i) {
        Individual ind = m_population[0];
        std::ranges::shuffle(ind.partIndices, m_rng);
        for (auto& r : ind.rotations) r = rotDist(m_rng);
        m_population.push_back(std::move(ind));
    }
}

/**
 * @brief Поворот набора полигонов на фиксированный угол (90, 180, 270).
 * @details
 * Использует целочисленную матрицу поворота, чтобы избежать ошибок округления double.
 * - 90 deg:  (x, y) -> (-y, x)
 * - 180 deg: (x, y) -> (-x, -y)
 * - 270 deg: (x, y) -> (y, -x)
 */
BoostPolygonSet GeneticOptimizer::rotatePolySet(const BoostPolygonSet& set, double angleDeg) {
    namespace bp = boost::polygon;
    std::vector<BoostPolygonWithHoles> polys;
    set.get(polys);
    BoostPolygonSet result;

    double rad = angleDeg * M_PI / 180.0;
    double cosA = std::cos(rad);
    double sinA = std::sin(rad);

    for(const auto& p : polys) {
        std::vector<BoostPoint> pts;
        for(auto it = bp::begin_points(p); it != bp::end_points(p); ++it) {
            double x = it->x();
            double y = it->y();
            pts.push_back(BoostPoint(
                static_cast<int>(std::round(x * cosA - y * sinA)),
                static_cast<int>(std::round(x * sinA + y * cosA))
                ));
        }
        BoostPolygon newOuter;
        bp::set_points(newOuter, pts.begin(), pts.end());
        result.insert(newOuter);

        BoostPolygonSet holesSet;
        for (auto itHole = bp::begin_holes(p); itHole != bp::end_holes(p); ++itHole) {
            std::vector<BoostPoint> hPts;
            for (auto it = bp::begin_points(*itHole); it != bp::end_points(*itHole); ++it) {
                double x = it->x();
                double y = it->y();
                hPts.push_back(BoostPoint(
                    static_cast<int>(std::round(x * cosA - y * sinA)),
                    static_cast<int>(std::round(x * sinA + y * cosA))
                    ));
            }
            BoostPolygon newHole;
            bp::set_points(newHole, hPts.begin(), hPts.end());
            holesSet.insert(newHole);
        }
        result -= holesSet;
    }
    return result;
}

/**
 * @brief Нормализация геометрии: сдвиг Top-Left (Min X, Min Y) угла BoundingBox в (0,0).
 * Важно для алгоритма размещения, который работает в локальных координатах.
 */
void GeneticOptimizer::normalizePolySet(BoostPolygonSet& set) {
    namespace bp = boost::polygon;
    bp::rectangle_data<int> rect;

    if (!bp::extents(rect, set)) return;

    int minX = bp::xl(rect);
    int minY = bp::yl(rect);

    if (minX == 0 && minY == 0) return;

    moveBoostSet(set, -minX, -minY);
}

/**
 * @brief Оценка приспособленности всей популяции.
 * @details
 * Использует OpenMP (`#pragma omp parallel for`) для распараллеливания,
 * так как `decode` (размещение) - самая тяжелая операция.
 *
 * Fitness функция: инвертированная утилизация.
 * Чем выше утилизация (площадь деталей / площадь листов), тем ниже (лучше) fitness.
 */
void GeneticOptimizer::evaluatePopulation(std::vector<Individual>& population,
                                          const std::vector<Part>& parts,
                                          const NestingParameters& params,
                                          const std::map<int, std::vector<BoostPolygonSet>>& rotatedPartsCache,
                                          const std::atomic<bool>& stopFlag)
{
    double angleStep = 360.0 / std::max(1, params.allowedRotations);
#pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < static_cast<int>(population.size()); ++i) {
        if (stopFlag) continue;

        auto& ind = population[i];
        ind.cachedSolution = decode(ind, parts, params, rotatedPartsCache);

        double fitness = ind.cachedSolution.usedSheets.size() * 1e6;
        double centerOfMassPenalty = 0.0;

        std::map<int, double> sheetMaxX;
        std::map<int, double> sheetMaxY;

        for (const auto& pp : ind.cachedSolution.placedParts) {
            int rIdx = static_cast<int>(std::round(pp.rotation / angleStep));
            const auto& pShape = rotatedPartsCache.at(pp.originalPartId)[rIdx];
            namespace bp = boost::polygon;
            bp::rectangle_data<int> r;
            bp::extents(r, pShape);

            double w = static_cast<double>(bp::xh(r) - bp::xl(r)) / NFPCalculator::NFP_SCALE;
            double h = static_cast<double>(bp::yh(r) - bp::yl(r)) / NFPCalculator::NFP_SCALE;

            double partRightX = pp.x + w;
            double partTopY = pp.y + h;

            double centerX = pp.x + (w / 2.0);
            double centerY = pp.y + (h / 2.0);
            centerOfMassPenalty += (std::pow(centerX, 2) + std::pow(centerY, 2));

            sheetMaxX[pp.sheetId] = std::max(sheetMaxX[pp.sheetId], partRightX);
            sheetMaxY[pp.sheetId] = std::max(sheetMaxY[pp.sheetId], partTopY);
        }

        double boundingBoxPenalty = 0.0;
        for (const auto& [sId, maxX] : sheetMaxX) {
            boundingBoxPenalty += (maxX * sheetMaxY[sId]);
        }

        fitness += boundingBoxPenalty + (centerOfMassPenalty * 1e-4);

        size_t placedCount = ind.cachedSolution.placedParts.size();
        size_t expectedCount = ind.partIndices.size();

        if (placedCount < expectedCount) {
            fitness += static_cast<double>(expectedCount - placedCount) * 1e9;
        }

        if (ind.cachedSolution.utilization < 1e-5) fitness += 1e9;

        ind.fitness = fitness;
    }
}

/**
 * @brief Турнирная селекция.
 * Выбирает двух случайных индивидов и возвращает лучшего из них.
 */
Individual GeneticOptimizer::selection(const std::vector<Individual>& pop) {
    std::uniform_int_distribution<size_t> dist(0, pop.size() - 1);
    const auto& p1 = pop[dist(m_rng)];
    const auto& p2 = pop[dist(m_rng)];
    return (p1.fitness < p2.fitness) ? p1 : p2;
}

/**
 * @brief Оператор кроссовера (Order Crossover - OX1).
 * @details
 * Поскольку геном - это перестановка (порядок деталей), нельзя использовать простой одноточечный кроссовер
 * (это приведет к дубликатам или пропуску деталей).
 * Используется OX1:
 * 1. Выбирается подстрока генов от родителя 1.
 * 2. Оставшиеся позиции заполняются генами от родителя 2 в том порядке, в котором они встречаются у него.
 */
std::pair<Individual, Individual> GeneticOptimizer::crossover(const Individual& p1, const Individual& p2) {
    size_t size = p1.partIndices.size();
    Individual c1, c2;
    c1.partIndices.resize(size, -1); c1.rotations.resize(size, 0);
    c2.partIndices.resize(size, -1); c2.rotations.resize(size, 0);

    std::uniform_int_distribution<size_t> dist(0, size - 1);
    size_t start = dist(m_rng);
    size_t end = dist(m_rng);
    if(start > end) std::swap(start, end);

    for(size_t i=start; i<=end; ++i) {
        c1.partIndices[i] = p1.partIndices[i];
        c1.rotations[i] = p1.rotations[i];

        c2.partIndices[i] = p2.partIndices[i];
        c2.rotations[i] = p2.rotations[i];
    }

    auto fillChild = [&](Individual& child, const Individual& donor) {
        size_t current = (end + 1) % size;
        for (size_t i = 0; i < size; ++i) {
            size_t donorIdx = (end + 1 + i) % size;
            int candidate = donor.partIndices[donorIdx];

            bool found = false;
            for(size_t k=start; k<=end; ++k) {
                if(child.partIndices[k] == candidate) { found = true; break; }
            }

            if(!found) {
                child.partIndices[current] = candidate;
                child.rotations[current] = donor.rotations[donorIdx];
                current = (current + 1) % size;
            }
        }
    };

    fillChild(c1, p2);
    fillChild(c2, p1);

    return {c1, c2};
}

/**
 * @brief Оператор мутации.
 * Два типа мутаций с вероятностью 5%:
 * 1. Swap: Менят местами две соседние детали в очереди.
 * 2. Rotate: Меняет вращение детали на случайное.
 */
void GeneticOptimizer::mutate(Individual& ind, int allowedRotations) {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    double rate = 0.05;

    size_t len = ind.partIndices.size();
    for(size_t i=0; i<len; ++i) {
        if(dist(m_rng) < rate) {
            size_t j = (i + 1) % len;
            std::swap(ind.partIndices[i], ind.partIndices[j]);
            std::swap(ind.rotations[i], ind.rotations[j]);
        }
        if(dist(m_rng) < rate) {
            ind.rotations[i] = std::uniform_int_distribution<int>(0, std::max(0, allowedRotations - 1))(m_rng);
        }
    }
}

// ---------------------------------------------------------
// PLACEMENT WORKER LOGIC
// ---------------------------------------------------------

/**
 * @brief Декодирует хромосому (Individual) в физическое решение (NestingSolution).
 *
 * @details
 * Это самая сложная часть алгоритма. Использует стратегию "First Fit Decreasing" с NFP.
 *
 * Логика выполнения:
 * 1. **Подготовка:** Масштабирование размеров листа в Boost координаты (long long).
 * 2. **Итерация по деталям:** Берет детали в порядке, определенном в `ind.partIndices`.
 * 3. **Попытка размещения:** Пробует разместить деталь на существующих листах.
 * - Вычисляет **Inner NFP** (где деталь влезает в лист геометрически).
 * - Если на листе уже есть детали, вычисляет **Outer NFP** для каждой из них относительно новой детали.
 * - **Outer NFP** - это сумма Минковского (A, -B), представляющая зону коллизии.
 * - **Valid Region** (допустимая зона) = Inner NFP (Лист) MINUS Union(All Outer NFPs).
 * - Для этой булевой операции разности используется библиотека Clipper2 (она быстрее и стабильнее Boost в сложных случаях).
 * 4. **Выбор позиции:**
 * - Если Valid Region не пуст, выбирается точка с минимальным X (затем Y). Это стратегия "Left-Bottom".
 * 5. **Создание нового листа:** Если деталь не влезла никуда, создается новый лист.
 *
 * @param ind Индивид для оценки.
 * @param parts Все доступные детали.
 * @param params Параметры (размеры листа).
 * @param rotatedPartsCache Кэш геометрических примитивов.
 * @return NestingSolution Готовое решение.
 */
NestingSolution GeneticOptimizer::decode(const Individual& ind,
                                         const std::vector<Part>& parts,
                                         const NestingParameters& params,
                                         const std::map<int, std::vector<BoostPolygonSet>>& rotatedPartsCache)
{
    // qDebug() << "  [Decode] Start decoding individual with" << ind.partIndices.size() << "parts.";

    constexpr double epsilon = 1e-5;
    constexpr double contactDilation = 50.0 * NFPCalculator::NFP_SCALE;
    NestingSolution solution;

    // --- 1. СКЛАД ЛИСТОВ ---
    struct AvailableSheet {
        int reqId;
        double width, height;
        int remaining;
        bool isInfinite;
        bool isCustomShape;
        std::optional<Part> customShape;
    };
    std::vector<AvailableSheet> availSheets;
    for(const auto& sr : params.sheets) {
        availSheets.push_back({sr.id, sr.width, sr.height, sr.quantity, sr.isInfinite, sr.isCustomShape, sr.customShape});
    }

    auto getNextSheet = [&]() -> AvailableSheet* {
        for (auto& s : availSheets) {
            if (s.isInfinite || s.remaining > 0) return &s;
        }
        return nullptr;
    };

    struct PlacedItem {
        int id; int rotation; BoostPolygonSet poly; int x, y;
    };

    struct SheetCtx {
        int id;
        int reqId;
        std::vector<PlacedItem> placedItems;
        double width, height;
        bool isCustomShape;
        std::optional<Part> customShape;
    };

    std::vector<SheetCtx> sheets;
    AvailableSheet* firstS = getNextSheet();
    if (firstS) {
        if (!firstS->isInfinite) firstS->remaining--;
        sheets.push_back({1, firstS->reqId, {}, firstS->width, firstS->height, firstS->isCustomShape, firstS->customShape});
    } else {
        return solution;
    }

    double offsetAmount = (params.partSpacing + params.cutThickness) / 2.0;
    double angleStep = 360.0 / std::max(1, params.allowedRotations);

    for (size_t i = 0; i < ind.partIndices.size(); ++i) {
        int partIdx = ind.partIndices[i];
        int rotIdx = ind.rotations[i];
        const Part& originalPart = parts[partIdx];

        const BoostPolygonSet& partShape = rotatedPartsCache.at(originalPart.id)[rotIdx];

        // qDebug() << "    [Decode] Placing part" << i << "(ID:" << originalPart.id << ")";

        namespace bp = boost::polygon;
        bp::rectangle_data<int> partRect;
        bp::extents(partRect, partShape);
        int pW = bp::xh(partRect) - bp::xl(partRect);
        int pH = bp::yh(partRect) - bp::yl(partRect);

        bool placed = false;

        for (auto& sheet : sheets) {
            int sheetWidthLocal = static_cast<int>(sheet.width * NFPCalculator::NFP_SCALE);
            int sheetHeightLocal = static_cast<int>(sheet.height * NFPCalculator::NFP_SCALE);

            if (sheet.placedItems.empty() && !sheet.isCustomShape) {
                if (pW <= sheetWidthLocal && pH <= sheetHeightLocal) {
                    PlacedItem newItem{partIdx, rotIdx, partShape, 0, 0};
                    sheet.placedItems.push_back(newItem);

                    PlacedPart pp{originalPart.id, sheet.id, offsetAmount, offsetAmount, rotIdx * angleStep};
                    solution.placedParts.push_back(pp);
                    solution.partsMap[originalPart.id] = originalPart;

                    placed = true;
                    break;
                }
            }

            int maxX = sheetWidthLocal - pW;
            int maxY = sheetHeightLocal - pH;

            Paths64 innerNFPClipper;
            if (sheet.isCustomShape) {
                auto key = std::make_tuple(sheet.reqId, originalPart.id, rotIdx);

                {
                    std::shared_lock lock(m_innerNfpMutex);
                    auto it = m_innerNfpCache.find(key);
                    if (it != m_innerNfpCache.end()) {
                        innerNFPClipper = it->second;
                    }
                }

                if (innerNFPClipper.empty()) {
                    BoostPolygonSet sheetPoly = GeometryAdapter::toBoost(sheet.customShape.value());
                    normalizePolySet(sheetPoly);
                    const auto& pShape = rotatedPartsCache.at(originalPart.id)[rotIdx];
                    BoostPolygonSet innerFit = NFPCalculator::calculateInnerNFP(sheetPoly, pShape);
                    innerNFPClipper = NFPCalculator::toClipper(innerFit);

                    std::unique_lock lock(m_innerNfpMutex);
                    m_innerNfpCache[key] = innerNFPClipper;
                }
            } else {
                if (maxX >= 0 && maxY >= 0) {
                    Path64 innerNFPPath = { Point64(0, 0), Point64(maxX, 0), Point64(maxX, maxY), Point64(0, maxY) };
                    innerNFPClipper.push_back(innerNFPPath);
                }
            }

            if (innerNFPClipper.empty()) continue;

            if (!sheet.placedItems.empty()) {
                Paths64 obstacles;
                obstacles.reserve(sheet.placedItems.size() * 2);

                for (const auto& placedItem : sheet.placedItems) {
                    int idA = parts[placedItem.id].id;
                    int rotA = placedItem.rotation;
                    int idB = originalPart.id;
                    int rotB = rotIdx;

                    auto key = std::make_tuple(idA, rotA, idB, rotB);
                    Paths64 outerPath;

                    {
                        std::shared_lock lock(m_nfpMutex);
                        auto it = m_nfpCache.find(key);
                        if (it != m_nfpCache.end()) {
                            outerPath = it->second;
                        }
                    }

                    if (outerPath.empty()) {
                        std::unique_lock lock(m_nfpMutex);
                        auto it = m_nfpCache.find(key);
                        if (it != m_nfpCache.end()) {
                            outerPath = it->second;
                        } else {
                            const auto& polyA = rotatedPartsCache.at(idA)[rotA];
                            const auto& polyB = rotatedPartsCache.at(idB)[rotB];
                            BoostPolygonSet nfpBoost = NFPCalculator::calculateOuterNFP(polyA, polyB);
                            outerPath = NFPCalculator::toClipper(nfpBoost);
                            m_nfpCache[key] = outerPath;
                        }
                    }

                    for (auto& path : outerPath) {
                        for (auto& pt : path) {
                            pt.x += placedItem.x;
                            pt.y += placedItem.y;
                        }
                    }
                    obstacles.insert(obstacles.end(), outerPath.begin(), outerPath.end());
                }

                Paths64 finalNFP;
                Clipper64 clipper;
                clipper.AddSubject(innerNFPClipper);
                clipper.AddClip(obstacles);
                clipper.Execute(ClipType::Difference, FillRule::NonZero, finalNFP);
                innerNFPClipper = finalNFP;
            }

            // ШАГ 3: Поиск лучшей позиции в FinalNFP
            // FinalNFP - это набор полигонов (зон), куда можно поставить деталь.
            // Нам нужно выбрать ОДНУ точку.
            // Эвристика: Left-Bottom (минимальный X, при равенстве - минимальный Y).

            double bestScoreX = std::numeric_limits<double>::max();
            double bestScoreY = std::numeric_limits<double>::max();
            double bestContactScore = -1.0;
            Point64 bestPos;
            bool foundPos = false;

            struct PlacedBBox {
                double minX, minY, maxX, maxY;
            };

            std::vector<PlacedBBox> placedBoxes;
            placedBoxes.reserve(sheet.placedItems.size());

            for (const auto& item : sheet.placedItems) {
                namespace bp = boost::polygon;
                bp::rectangle_data<int> r;
                if (bp::extents(r, item.poly)) {
                    placedBoxes.emplace_back(
                        static_cast<double>(bp::xl(r)), static_cast<double>(bp::yl(r)),
                        static_cast<double>(bp::xh(r)), static_cast<double>(bp::yh(r))
                        );
                }
            }

            for (const auto& path : innerNFPClipper) {
                for (const auto& pt : path) {
                    if (!sheet.isCustomShape) {
                        if (pt.x < 0 || pt.y < 0 || pt.x > maxX || pt.y > maxY) {
                            continue;
                        }
                    }

                    double currentX = static_cast<double>(pt.x);
                    double currentY = static_cast<double>(pt.y);

                    bool isBetterBL = currentX < bestScoreX - epsilon ||
                                      (std::abs(currentX - bestScoreX) <= epsilon && currentY < bestScoreY - epsilon);

                    bool isEqualBL = std::abs(currentX - bestScoreX) <= epsilon &&
                                     std::abs(currentY - bestScoreY) <= epsilon;

                    if (isBetterBL || isEqualBL) {
                        double contactScore = 0.0;
                        double partMinX = currentX;
                        double partMinY = currentY;
                        double partMaxX = currentX + static_cast<double>(pW);
                        double partMaxY = currentY + static_cast<double>(pH);

                        for (const auto& box : placedBoxes) {
                            double interLeft = std::max(partMinX, box.minX - contactDilation);
                            double interTop = std::max(partMinY, box.minY - contactDilation);
                            double interRight = std::min(partMaxX, box.maxX + contactDilation);
                            double interBottom = std::min(partMaxY, box.maxY + contactDilation);

                            if (interLeft < interRight && interTop < interBottom) {
                                contactScore += (interRight - interLeft) * (interBottom - interTop);
                            }
                        }

                        if (isBetterBL || contactScore > bestContactScore) {
                            bestScoreX = currentX;
                            bestScoreY = currentY;
                            bestContactScore = contactScore;
                            bestPos = pt;
                            foundPos = true;
                        }
                    }
                }
            }

            double sheetOffsetX = 0.0;
            double sheetOffsetY = 0.0;
            if (sheet.isCustomShape && sheet.customShape.has_value()) {
                BoostPolygonSet bpSet = GeometryAdapter::toBoost(sheet.customShape.value());
                namespace bp = boost::polygon;
                bp::rectangle_data<int> rect;
                if (bp::extents(rect, bpSet)) {
                    sheetOffsetX = static_cast<double>(bp::xl(rect)) / NFPCalculator::NFP_SCALE;
                    sheetOffsetY = static_cast<double>(bp::yl(rect)) / NFPCalculator::NFP_SCALE;
                }
            }

            if (foundPos) {
                PlacedItem newItem{partIdx, rotIdx, partShape, static_cast<int>(bestPos.x), static_cast<int>(bestPos.y)};
                moveBoostSet(newItem.poly, newItem.x, newItem.y);
                sheet.placedItems.push_back(newItem);

                PlacedPart pp;
                pp.originalPartId = originalPart.id;
                pp.sheetId = sheet.id;
                pp.rotation = rotIdx * angleStep;
                pp.x = (static_cast<double>(newItem.x) / NFPCalculator::NFP_SCALE) + offsetAmount + sheetOffsetX;
                pp.y = (static_cast<double>(newItem.y) / NFPCalculator::NFP_SCALE) + offsetAmount + sheetOffsetY;
                solution.placedParts.push_back(pp);
                solution.partsMap[originalPart.id] = originalPart;

                placed = true;
                break;
            }
        }

        if (!placed) {
            AvailableSheet* nextS = getNextSheet();
            bool placedOnNewSheet = false;

            if (nextS) {
                if (!nextS->isInfinite) nextS->remaining--;

                SheetCtx newSheet{
                    static_cast<int>(sheets.size()) + 1,
                    nextS->reqId,
                    {},
                    nextS->width,
                    nextS->height,
                    nextS->isCustomShape,
                    nextS->customShape
                };

                int newSheetWLocal = static_cast<int>(newSheet.width * NFPCalculator::NFP_SCALE);
                int newSheetHLocal = static_cast<int>(newSheet.height * NFPCalculator::NFP_SCALE);

                if (!newSheet.isCustomShape) {
                    if (pW <= newSheetWLocal && pH <= newSheetHLocal) {
                        PlacedItem newItem{partIdx, rotIdx, partShape, 0, 0};
                        newSheet.placedItems.push_back(newItem);
                        sheets.push_back(newSheet);

                        PlacedPart pp{originalPart.id, newSheet.id, offsetAmount, offsetAmount, rotIdx * angleStep};
                        solution.placedParts.push_back(pp);
                        solution.partsMap[originalPart.id] = originalPart;
                        placedOnNewSheet = true;
                    }
                } else {
                    Paths64 newSheetInnerNFP;
                    auto key = std::make_tuple(newSheet.reqId, originalPart.id, rotIdx);

                    {
                        std::shared_lock lock(m_innerNfpMutex);
                        auto it = m_innerNfpCache.find(key);
                        if (it != m_innerNfpCache.end()) {
                            newSheetInnerNFP = it->second;
                        }
                    }

                    if (newSheetInnerNFP.empty()) {
                        std::unique_lock lock(m_innerNfpMutex);
                        auto it = m_innerNfpCache.find(key);
                        if (it != m_innerNfpCache.end()) {
                            newSheetInnerNFP = it->second;
                        } else {
                            BoostPolygonSet sheetPoly = GeometryAdapter::toBoost(newSheet.customShape.value());
                            normalizePolySet(sheetPoly);
                            const auto& pShape = rotatedPartsCache.at(originalPart.id)[rotIdx];
                            BoostPolygonSet innerFit = NFPCalculator::calculateInnerNFP(sheetPoly, pShape);
                            newSheetInnerNFP = NFPCalculator::toClipper(innerFit);
                            m_innerNfpCache[key] = newSheetInnerNFP;
                        }
                    }

                    if (!newSheetInnerNFP.empty()) {
                        double bestX = std::numeric_limits<double>::max();
                        double bestY = std::numeric_limits<double>::max();
                        Point64 initialPos;
                        bool foundInitial = false;

                        for (const auto& path : newSheetInnerNFP) {
                            for (const auto& pt : path) {
                                if (pt.x < bestX || (std::abs(pt.x - bestX) <= epsilon && pt.y < bestY)) {
                                    bestX = static_cast<double>(pt.x);
                                    bestY = static_cast<double>(pt.y);
                                    initialPos = pt;
                                    foundInitial = true;
                                }
                            }
                        }

                        if (foundInitial) {
                            PlacedItem newItem{partIdx, rotIdx, partShape, static_cast<int>(initialPos.x), static_cast<int>(initialPos.y)};
                            moveBoostSet(newItem.poly, newItem.x, newItem.y);
                            newSheet.placedItems.push_back(newItem);
                            sheets.push_back(newSheet);

                            double tOffsetX = 0.0;
                            double tOffsetY = 0.0;
                            if (newSheet.customShape.has_value()) {
                                BoostPolygonSet bpSet = GeometryAdapter::toBoost(newSheet.customShape.value());
                                namespace bp = boost::polygon;
                                bp::rectangle_data<int> rect;
                                if (bp::extents(rect, bpSet)) {
                                    tOffsetX = static_cast<double>(bp::xl(rect)) / NFPCalculator::NFP_SCALE;
                                    tOffsetY = static_cast<double>(bp::yl(rect)) / NFPCalculator::NFP_SCALE;
                                }
                            }

                            PlacedPart pp;
                            pp.originalPartId = originalPart.id;
                            pp.sheetId = newSheet.id;
                            pp.rotation = rotIdx * angleStep;
                            pp.x = (static_cast<double>(newItem.x) / NFPCalculator::NFP_SCALE) + offsetAmount + tOffsetX;
                            pp.y = (static_cast<double>(newItem.y) / NFPCalculator::NFP_SCALE) + offsetAmount + tOffsetY;
                            solution.placedParts.push_back(pp);
                            solution.partsMap[originalPart.id] = originalPart;
                            placedOnNewSheet = true;
                        }
                    }
                }
            }

            if (!placedOnNewSheet) {
                solution.unplacedPartIds.push_back(originalPart.id);
            }
        }
    }

    double totalBoundingBoxArea = 0.0;
    double partsArea = 0.0;

    for (const auto& sheet : sheets) {
        if (sheet.placedItems.empty()) continue;

        int maxBoostX = 0;
        int maxBoostY = 0;
        double sheetPartsArea = 0.0;

        for (const auto& item : sheet.placedItems) {
            namespace bp = boost::polygon;
            bp::rectangle_data<int> r;
            if (bp::extents(r, item.poly)) {
                maxBoostX = std::max(maxBoostX, bp::xh(r));
                maxBoostY = std::max(maxBoostY, bp::yh(r));
            }
            sheetPartsArea += boost::polygon::area(rotatedPartsCache.at(parts[item.id].id)[item.rotation]);
        }

        double realMaxX = (static_cast<double>(maxBoostX) / NFPCalculator::NFP_SCALE) + offsetAmount;
        double realMaxY = (static_cast<double>(maxBoostY) / NFPCalculator::NFP_SCALE) + offsetAmount;

        NestingSheet ns;
        ns.id = sheet.id;
        ns.width = sheet.width;
        ns.height = sheet.height;
        ns.usedWidth = realMaxX;
        ns.usedHeight = realMaxY;
        ns.isCustomShape = sheet.isCustomShape;
        ns.customShape = sheet.customShape;
        solution.usedSheets.push_back(ns);

        totalBoundingBoxArea += (realMaxX * realMaxY);
        partsArea += sheetPartsArea / (NFPCalculator::NFP_SCALE * NFPCalculator::NFP_SCALE);
    }

    solution.utilization = (totalBoundingBoxArea > 0) ? (partsArea / totalBoundingBoxArea) * 100.0 : 0.0;

    // qDebug() << "  [Decode] Finished. Utilization:" << solution.utilization;
    solution.showRemnants = params.showRemnants;
    solution.cutThickness = params.cutThickness;
    return solution;
}
