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


        std::vector<Part> allParts = geometry.parts;

        if (allParts.empty()) {
            emit errorOccurred("Нет деталей для раскроя.");
            return;
        }

        NestingSolution solution = m_optimizer.optimize(
            allParts,
            params,
            m_stopRequested,
            [this](const NestingSolution& intermediate) {
                emit progressUpdated(intermediate);
            }
            );

        if (m_stopRequested) return;

        emit finished(solution);

    } catch (const std::exception &e) {
        emit errorOccurred(QString::fromStdString(e.what()));
    } catch (...) {
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
