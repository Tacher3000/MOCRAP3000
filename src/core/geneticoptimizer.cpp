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
#include <omp.h> // Если доступен OpenMP для параллельных вычислений

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
static void moveBoostSet(BoostPolygonSet& set, long long dx, long long dy) {
    namespace bp = boost::polygon;
    std::vector<BoostPolygon> polys;
    set.get(polys);
    set.clear(); // Очищаем контейнер перед перезаписью

    for(auto& p : polys) {
        bp::move(p, bp::HORIZONTAL, dx);
        bp::move(p, bp::VERTICAL, dy);
        set.insert(p); // Вставляем обратно перемещенный полигон
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

    qDebug() << "=== Starting Optimization ===";
    qDebug() << "Parts count:" << parts.size();
    qDebug() << "Params:" << params.sheetWidth << "x" << params.sheetHeight;
    if (parts.empty()) return {};

    // 1. Конфигурация
    m_config.populationSize = 20; // Небольшой размер популяции типичен для нестинга (тяжелая fitness-функция)

    // --- КЭШИРОВАНИЕ ГЕОМЕТРИИ ---
    // Мы заранее вычисляем все возможные варианты вращения деталей.
    // Это критически важно для производительности decode().
    // Ключ map: ID детали. Значение: Вектор из 4-х BoostPolygonSet (0, 90, 180, 270 градусов).
    std::map<int, std::vector<BoostPolygonSet>> rotatedPartsCache;

    for(const auto& part : parts) {
        BoostPolygonSet base = GeometryAdapter::toBoost(part);
        // Нормализуем базовую деталь в 0,0 (Top-Left bounding box в 0,0)
        normalizePolySet(base);

        rotatedPartsCache[part.id].resize(4);

        for(int r = 0; r < 4; ++r) {
            if (r == 0) {
                rotatedPartsCache[part.id][r] = base;
            } else {
                rotatedPartsCache[part.id][r] = rotatePolySet(base, r * 90);
                normalizePolySet(rotatedPartsCache[part.id][r]);
            }
        }
    }

    qDebug() << "Инициализация первичной популяции";
    // 2. Инициализация первичной популяции
    initializePopulation(parts, m_config.populationSize);

    qDebug() << "Оценка первой популяции";
    // 3. Оценка первой популяции (расчет укладки)
    evaluatePopulation(m_population, parts, params, rotatedPartsCache);

    // Поиск текущего лидера
    Individual bestInd = m_population[0];
    // Обновляем UI первым результатом
    if (progressCallback) progressCallback(bestInd.cachedSolution);

    // 4. Основной цикл поколений
    int gen = 0;
    for (; gen < m_config.maxGenerations; ++gen) {
        if (stopFlag) {
            qDebug() << "Stop requested by user at generation" << gen;
            break;
        }
        qDebug() << "Processing Generation:" << gen; // <--- ЛОГ ПОКОЛЕНИЯ

        // Элитизм: гарантированно сохраняем лучшего индивида
        std::vector<Individual> newPop;
        newPop.reserve(m_config.populationSize);
        newPop.push_back(bestInd);

        // Генерация потомков
        while (newPop.size() < static_cast<size_t>(m_config.populationSize)) {
            // Селекция
            Individual p1 = selection(m_population);
            Individual p2 = selection(m_population);

            // Скрещивание (Crossover)
            std::pair<Individual, Individual> children = crossover(p1, p2);

            // Мутация
            mutate(children.first);
            mutate(children.second);

            newPop.push_back(children.first);
            if(newPop.size() < static_cast<size_t>(m_config.populationSize))
                newPop.push_back(children.second);
        }

        m_population = std::move(newPop);

        // Оценка нового поколения
        qDebug() << "Evaluating population...";
        evaluatePopulation(m_population, parts, params, rotatedPartsCache);
        qDebug() << "Evaluation finished.";

        // Поиск нового лучшего
        // Примечание: Мы ищем MIN fitness (где fitness = 1/utilization или waste),
        // но в данной реализации fitness = 1.0/utilization, значит чем меньше, тем лучше.
        auto it = std::max_element(m_population.begin(), m_population.end(),
                                   [](const Individual& a, const Individual& b) {
                                       return a.fitness < b.fitness; // Трюк: max_element + < ищет "максимальный" по значению
                                   });

        // Проверка на улучшение
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
void GeneticOptimizer::initializePopulation(const std::vector<Part>& parts, int popSize) {
    m_population.clear();
    int numParts = static_cast<int>(parts.size());

    // Сортировка по площади (Adam - первый индивид)
    std::vector<std::pair<int, double>> sortedParts(numParts);
    for(int i=0; i<numParts; ++i) {
        BoostPolygonSet ps = GeometryAdapter::toBoost(parts[i]);
        sortedParts[i] = {i, boost::polygon::area(ps)};
    }
    // Сортировка по убыванию площади
    std::sort(sortedParts.begin(), sortedParts.end(), [](auto& a, auto& b){
        return a.second > b.second;
    });

    Individual adam;
    adam.partIndices.resize(numParts);
    adam.rotations.resize(numParts, 0); // По умолчанию без вращения

    for(int i=0; i<numParts; ++i) adam.partIndices[i] = sortedParts[i].first;

    m_population.push_back(adam);

    // Остальные - случайные вариации
    for(int i=1; i<popSize; ++i) {
        Individual ind = adam;
        // Shuffle - случайная перестановка порядка укладки
        std::shuffle(ind.partIndices.begin(), ind.partIndices.end(), m_rng);
        // Случайные вращения (0..3)
        std::uniform_int_distribution<int> rotDist(0, 3);
        for(auto& r : ind.rotations) r = rotDist(m_rng);
        m_population.push_back(ind);
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
BoostPolygonSet GeneticOptimizer::rotatePolySet(const BoostPolygonSet& set, int angle) {
    namespace bp = boost::polygon;
    std::vector<BoostPolygon> polys;
    set.get(polys);
    BoostPolygonSet result;

    for(const auto& p : polys) {
        std::vector<BoostPoint> pts;
        for(auto it = bp::begin_points(p); it != bp::end_points(p); ++it) {
            long long x = it->x();
            long long y = it->y();
            long long nx = x, ny = y;

            if (angle == 90) { nx = -y; ny = x; }
            else if (angle == 180) { nx = -x; ny = -y; }
            else if (angle == 270) { nx = y; ny = -x; }

            pts.push_back(BoostPoint(nx, ny));
        }
        BoostPolygon newP;
        bp::set_points(newP, pts.begin(), pts.end());
        result.insert(newP);
    }
    return result;
}

/**
 * @brief Нормализация геометрии: сдвиг Top-Left угла BoundingBox в (0,0).
 * Важно для алгоритма размещения, который работает в локальных координатах.
 */
void GeneticOptimizer::normalizePolySet(BoostPolygonSet& set) {
    namespace bp = boost::polygon;
    bp::rectangle_data<long long> rect;
    if (!bp::extents(rect, set)) return; // Пустой сет

    long long dx = -bp::xl(rect); // xl - min X
    long long dy = -bp::yl(rect); // yl - min Y

    moveBoostSet(set, dx, dy);
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
                                          const std::map<int, std::vector<BoostPolygonSet>>& rotatedPartsCache)
{
    // Распараллеливаем цикл по индивидам
#pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < static_cast<int>(population.size()); ++i) {
        // Декодируем генотип в фенотип (раскладку)
        population[i].cachedSolution = decode(population[i], parts, params, rotatedPartsCache);

        // Расчет Fitness.
        // utilization возвращается в процентах (0..100).
        // Цель алгоритма: минимизировать fitness.
        // Если утилизация 0 (ошибка), даем огромный штраф.
        double util = population[i].cachedSolution.utilization;
        if (util < 1e-5) population[i].fitness = 1e9; // Штраф
        else population[i].fitness = 1.0 / util; // Чем больше util, тем меньше fitness
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
    // Меньше fitness - лучше
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

    // 1. Копируем сегмент от первого родителя
    for(size_t i=start; i<=end; ++i) {
        c1.partIndices[i] = p1.partIndices[i];
        c1.rotations[i] = p1.rotations[i]; // Вращение привязано к детали

        c2.partIndices[i] = p2.partIndices[i];
        c2.rotations[i] = p2.rotations[i];
    }

    // 2. Заполняем пробелы генами второго родителя
    auto fillChild = [&](Individual& child, const Individual& donor) {
        size_t current = (end + 1) % size; // Начинаем сразу после сегмента
        for (size_t i = 0; i < size; ++i) {
            size_t donorIdx = (end + 1 + i) % size; // Проходим по донору циклично
            int candidate = donor.partIndices[donorIdx];

            // Проверяем, есть ли уже этот ген в сегменте
            bool found = false;
            for(size_t k=start; k<=end; ++k) {
                if(child.partIndices[k] == candidate) { found = true; break; }
            }

            // Если гена нет, вставляем
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
void GeneticOptimizer::mutate(Individual& ind) {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    double rate = 0.05; // 5% вероятность мутации гена

    size_t len = ind.partIndices.size();
    for(size_t i=0; i<len; ++i) {
        if(dist(m_rng) < rate) {
            // Swap with next
            size_t j = (i + 1) % len;
            std::swap(ind.partIndices[i], ind.partIndices[j]);
            std::swap(ind.rotations[i], ind.rotations[j]);
        }
        if(dist(m_rng) < rate) {
            // New random rotation
            ind.rotations[i] = std::uniform_int_distribution<int>(0, 3)(m_rng);
        }
    }
}

// ---------------------------------------------------------
// PLACEMENT WORKER LOGIC (СЕРДЦЕ АЛГОРИТМА)
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
    qDebug() << "  [Decode] Start decoding individual with" << ind.partIndices.size() << "parts.";

    NestingSolution solution;

    // Переводим размеры листа в систему координат Boost (умножаем на 10^7)
    long long sheetWidthLocal = static_cast<long long>(params.sheetWidth * NFPCalculator::NFP_SCALE);
    long long sheetHeightLocal = static_cast<long long>(params.sheetHeight * NFPCalculator::NFP_SCALE);

    if (sheetWidthLocal <= 0 || sheetHeightLocal <= 0) {
        qWarning() << "Invalid sheet dimensions:" << params.sheetWidth << "x" << params.sheetHeight;
        return solution;
    }

    // Локальная структура для хранения состояния размещения во время расчета
    struct PlacedItem {
        int id; // Индекс в массиве parts (не путать с Part ID)
        int rotation; // Индекс вращения (0..3)
        BoostPolygonSet poly; // Полигон в абсолютных координатах листа
        long long x, y; // Координаты размещения (Top-Left bounding box)
    };

    // Контекст листа
    struct SheetCtx {
        int id;
        std::vector<PlacedItem> placedItems;
        // Paths64 combinedOuterNFP; // (Опционально) можно кэшировать объединение препятствий
    };

    std::vector<SheetCtx> sheets;
    sheets.push_back({1, {}}); // Создаем первый пустой лист

    // --- ГЛАВНЫЙ ЦИКЛ РАЗМЕЩЕНИЯ ---
    for (size_t i = 0; i < ind.partIndices.size(); ++i) {
        int partIdx = ind.partIndices[i];
        int rotIdx = ind.rotations[i];
        const Part& originalPart = parts[partIdx];

        // Получаем геометрию из кэша (уже повернута и нормализована в 0,0)
        const BoostPolygonSet& partShape = rotatedPartsCache.at(originalPart.id)[rotIdx];

        qDebug() << "    [Decode] Placing part" << i << "(ID:" << originalPart.id << ")";

        // Получаем габариты детали (BBox) для быстрой проверки
        namespace bp = boost::polygon;
        bp::rectangle_data<long long> partRect;
        bp::extents(partRect, partShape);
        long long pW = bp::xh(partRect) - bp::xl(partRect);
        long long pH = bp::yh(partRect) - bp::yl(partRect);

        bool placed = false;

        // Итерация по существующим листам
        for (auto& sheet : sheets) {
            // ОПТИМИЗАЦИЯ 1: Лист пустой?
            if (sheet.placedItems.empty()) {
                if (pW <= sheetWidthLocal && pH <= sheetHeightLocal) {
                    // Просто кладем в 0,0
                    PlacedItem newItem;
                    newItem.id = partIdx;
                    newItem.rotation = rotIdx;
                    newItem.x = 0;
                    newItem.y = 0;
                    newItem.poly = partShape; // Копируем форму
                    // (Она уже в 0,0, сдвигать не надо)

                    sheet.placedItems.push_back(newItem);

                    // Запись в результат
                    PlacedPart pp;
                    pp.originalPartId = originalPart.id;
                    pp.sheetId = sheet.id;
                    pp.rotation = rotIdx * 90.0;
                    pp.x = 0.0;
                    pp.y = 0.0;
                    solution.placedParts.push_back(pp);
                    solution.partsMap[originalPart.id] = originalPart;

                    placed = true;
                    qDebug() << "      -> Placed on EMPTY sheet" << sheet.id;
                    break;
                }
            }

            // ШАГ 1: Вычисляем Inner NFP (Допустимая зона внутри листа)
            // Для прямоугольного листа и нормализованной детали это просто прямоугольник.
            // BBox листа минус BBox детали.
            long long maxX = sheetWidthLocal - pW;
            long long maxY = sheetHeightLocal - pH;

            if (maxX < 0 || maxY < 0) continue; // Деталь физически больше листа

            // Формируем путь Clipper для Inner NFP
            Path64 innerNFPPath;
            innerNFPPath.push_back(Point64(0, 0));
            innerNFPPath.push_back(Point64(maxX, 0LL));
            innerNFPPath.push_back(Point64(maxX, maxY));
            innerNFPPath.push_back(Point64(0LL, maxY));

            Paths64 innerNFPClipper;
            innerNFPClipper.push_back(innerNFPPath);

            // ШАГ 2: Вычитаем препятствия (Outer NFP размещенных деталей)
            if (!sheet.placedItems.empty()) {
                Paths64 obstacles;
                qDebug() << "      -> Calculating NFP with" << sheet.placedItems.size() << "obstacles...";

                for (const auto& placedItem : sheet.placedItems) {
                    // Логика Outer NFP:
                    // A = уже размещенная деталь (placedItem)
                    // B = текущая деталь (partShape)
                    // OuterNFP(A, B) = MinkowskiSum(A, -B).
                    // Это множество точек, куда если поставить origin B, то A и B пересекутся.

                    // Берем кэшированную геометрию A (в 0,0) и B (в 0,0)
                    const BoostPolygonSet& polyA = rotatedPartsCache.at(parts[placedItem.id].id)[placedItem.rotation];

                    // Вычисляем NFP в относительных координатах (как будто обе в 0,0)
                    BoostPolygonSet outerNFP = NFPCalculator::calculateOuterNFP(polyA, partShape);
                    Paths64 outerPath = NFPCalculator::toClipper(outerNFP);

                    // Сдвигаем полученный NFP в реальную позицию детали A на листе
                    for (auto& path : outerPath) {
                        for (auto& pt : path) {
                            pt.x += placedItem.x;
                            pt.y += placedItem.y;
                        }
                    }
                    // Добавляем в список препятствий
                    obstacles.insert(obstacles.end(), outerPath.begin(), outerPath.end());
                }

                // ВЫЧИТАНИЕ: FinalNFP = InnerNFP Difference Union(Obstacles)
                // Clipper автоматически делает Union для Subject/Clip перед разностью.
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

            double bestScore = std::numeric_limits<double>::max();
            Point64 bestPos;
            bool foundPos = false;

            for (const auto& path : innerNFPClipper) {
                for (const auto& pt : path) {
                    // score = x + y * k. k должно быть маленьким, чтобы Y влиял меньше X.
                    double score = pt.x + pt.y * 0.0001;

                    if (score < bestScore) {
                        bestScore = score;
                        bestPos = pt;
                        foundPos = true;
                    }
                }
            }

            if (foundPos) {
                // Размещаем деталь
                PlacedItem newItem;
                newItem.id = partIdx;
                newItem.rotation = rotIdx;
                newItem.x = bestPos.x;
                newItem.y = bestPos.y;

                // Для корректной работы calculateOuterNFP на следующем шаге,
                // нам нужно, чтобы 'poly' в placedItems оставался в 0,0 (как шаблон),
                // или мы должны хранить координаты отдельно.
                // В данной реализации мы храним polyShape (шаблон) и координаты (x,y) отдельно.
                // Но для визуализации/экспорта в placedItems можно было бы хранить абсолютные координаты.
                // Здесь `newItem.poly` не используется для OuterNFP (берется из кэша),
                // но может пригодиться для дебага.
                newItem.poly = partShape;
                moveBoostSet(newItem.poly, newItem.x, newItem.y);

                sheet.placedItems.push_back(newItem);

                // Запись в итоговое решение (конвертация обратно в double)
                PlacedPart pp;
                pp.originalPartId = originalPart.id;
                pp.sheetId = sheet.id;
                pp.rotation = rotIdx * 90.0;
                pp.x = static_cast<double>(newItem.x) / NFPCalculator::NFP_SCALE;
                pp.y = static_cast<double>(newItem.y) / NFPCalculator::NFP_SCALE;
                solution.placedParts.push_back(pp);
                solution.partsMap[originalPart.id] = originalPart;

                placed = true;
                qDebug() << "      -> Placed on sheet" << sheet.id << "at" << pp.x << pp.y;
                break; // Переходим к следующей детали
            }
        } // конец цикла по листам

        // Если деталь не поместилась ни на один лист, создаем новый
        if (!placed) {
            if (pW <= sheetWidthLocal && pH <= sheetHeightLocal) {
                SheetCtx newSheet;
                newSheet.id = static_cast<int>(sheets.size()) + 1;

                // Размещаем в 0,0
                PlacedItem newItem;
                newItem.id = partIdx;
                newItem.rotation = rotIdx;
                newItem.x = 0;
                newItem.y = 0;
                newItem.poly = partShape; // 0,0

                newSheet.placedItems.push_back(newItem);
                sheets.push_back(newSheet);

                PlacedPart pp;
                pp.originalPartId = originalPart.id;
                pp.sheetId = newSheet.id;
                pp.rotation = rotIdx * 90.0;
                pp.x = 0.0;
                pp.y = 0.0;
                solution.placedParts.push_back(pp);
                solution.partsMap[originalPart.id] = originalPart;
            } else {
                // Критическая ошибка: деталь больше размера листа
                qWarning() << "Part " << originalPart.id << " does not fit on empty sheet!";
            }
        }
    } // конец цикла по деталям

    // 4. Сбор статистики (Утилизация)
    double totalArea = 0;
    double partsArea = 0;

    for (const auto& sheet : sheets) {
        if (sheet.placedItems.empty()) continue;

        solution.usedSheets.push_back({sheet.id, params.sheetWidth, params.sheetHeight});
        totalArea += params.sheetWidth * params.sheetHeight;

        for (const auto& item : sheet.placedItems) {
            // Площадь берем из кэша (Boost возвращает double/long long Area)
            double area = boost::polygon::area(rotatedPartsCache.at(parts[item.id].id)[item.rotation]);
            // Нормализуем площадь обратно к единицам пользователя (делим на scale^2)
            partsArea += area / (NFPCalculator::NFP_SCALE * NFPCalculator::NFP_SCALE);
        }
    }

    solution.utilization = (totalArea > 0) ? (partsArea / totalArea) * 100.0 : 0.0;

    qDebug() << "  [Decode] Finished. Utilization:" << solution.utilization;

    return solution;
}
