#ifndef GENETICOPTIMIZER_H
#define GENETICOPTIMIZER_H

#include "layoutstructures.h"
#include "geometry.h"
#include "geometryadapter.h"
#include <vector>
#include <random>
#include <atomic>
#include <functional>
#include <map>

/**
 * @brief Особь в популяции генетического алгоритма.
 */
struct Individual {
    std::vector<int> partIndices; // Порядок
    std::vector<int> rotations;   // Индекс угла (0..3)

    double fitness = 0.0;
    NestingSolution cachedSolution;
};

class GeneticOptimizer {
public:
    struct Config {
        int populationSize = 20;
        int maxGenerations = 100;
        double mutationRate = 0.1;
    };

    GeneticOptimizer();

    NestingSolution optimize(const std::vector<Part>& parts,
                             const NestingParameters& params,
                             std::atomic<bool>& stopFlag,
                             std::function<void(const NestingSolution&)> progressCallback = nullptr);

private:
    void initializePopulation(const std::vector<Part>& parts, int popSize);

    // Кэш для ускорения: вращенные версии всех деталей (в Boost формате)
    void evaluatePopulation(std::vector<Individual>& population,
                            const std::vector<Part>& parts,
                            const NestingParameters& params,
                            const std::map<int, std::vector<BoostPolygonSet>>& rotatedPartsCache);

    Individual selection(const std::vector<Individual>& population);
    std::pair<Individual, Individual> crossover(const Individual& p1, const Individual& p2);
    void mutate(Individual& ind);

    // Основная функция раскроя (Placement Worker Logic)
    NestingSolution decode(const Individual& ind,
                           const std::vector<Part>& parts,
                           const NestingParameters& params,
                           const std::map<int, std::vector<BoostPolygonSet>>& rotatedPartsCache);

    // Утилиты геометрии
    BoostPolygonSet rotatePolySet(const BoostPolygonSet& set, int angle);
    void normalizePolySet(BoostPolygonSet& set);

    std::vector<Individual> m_population;
    std::mt19937 m_rng;
    Config m_config;
};

#endif // GENETICOPTIMIZER_H
