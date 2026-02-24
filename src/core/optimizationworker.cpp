#include "optimizationworker.h"
#include <QDebug>
#include <vector>

OptimizationWorker::OptimizationWorker(QObject *parent)
    : QObject(parent)
{
}

/**
 * @brief Основной метод обработки, выполняемый в отдельном потоке.
 * * @details
 * Логика выполнения:
 * 1. Сбрасывает флаг остановки.
 * 2. Тиражирует исходные детали (Geometry) согласно указанному количеству (Params).
 * Это необходимо, так как алгоритм раскроя работает с конкретными экземплярами.
 * 3. Запускает блокирующий метод GeneticOptimizer::optimize.
 * 4. Передает callback-лямбду в оптимизатор для обновления прогресса в реальном времени.
 * * @param geometry Исходная геометрия (передается по значению, т.к. Qt копирует данные между потоками).
 * @param params Параметры раскроя (размеры листа, отступы).
 */
void OptimizationWorker::process(const Geometry geometry, const NestingParameters params)
{
    m_stopRequested = false;

    try {
        // 1. Разворачиваем геометрию (Тиражирование)
        std::vector<Part> allParts;
        // Резервируем память, чтобы избежать лишних аллокаций
        allParts.reserve(geometry.parts.size() * params.partCount);

        for (const auto& uniquePart : geometry.parts) {
            for (int i = 0; i < params.partCount; ++i) {
                // Копируем деталь.
                // В будущем здесь можно добавлять уникальный суффикс к ID,
                // если потребуется различать копии (например, Part_1_Copy_1).
                allParts.push_back(uniquePart);
            }
        }

        // 2. Запуск Генетического Алгоритма
        // Этот вызов блокирует поток на длительное время.
        NestingSolution solution = m_optimizer.optimize(
            allParts,
            params,
            m_stopRequested, // Передаем ссылку на атомарный флаг
            [this](const NestingSolution& intermediate) {
                // Лямбда вызывается внутри глубокого цикла оптимизатора.
                // emit thread-safe, он поставит событие в очередь GUI потока.
                emit progressUpdated(intermediate);
            }
            );

        if (m_stopRequested) return;

        emit finished(solution);

    } catch (const std::exception &e) {
        // Ловим стандартные исключения C++ (например, bad_alloc или logic_error из Boost)
        emit errorOccurred(QString::fromStdString(e.what()));
    } catch (...) {
        // Ловим всё остальное (не рекомендуется, но спасает от краша приложения)
        emit errorOccurred(QStringLiteral("Unknown error during optimization"));
    }
}

/**
 * @brief Устанавливает атомарный флаг остановки.
 * Метод thread-safe, может вызываться из GUI потока в любой момент.
 */
void OptimizationWorker::stop()
{
    m_stopRequested = true;
}
