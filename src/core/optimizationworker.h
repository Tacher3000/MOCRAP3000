#ifndef OPTIMIZATIONWORKER_H
#define OPTIMIZATIONWORKER_H

#include <QObject>
#include <QMutex>
#include <atomic>
#include "geneticoptimizer.h"
#include "layoutstructures.h"

/**
 * @brief Worker для выполнения тяжелых расчетов в отдельном потоке.
 * Следует паттерну "Worker Object" в Qt.
 */
class OptimizationWorker : public QObject {
    Q_OBJECT

public:
    explicit OptimizationWorker(QObject *parent = nullptr);

public slots:
    /**
     * @brief Запускает процесс оптимизации.
     * Использует передачу по значению для Geometry (Qt скопирует при QueuedConnection),
     * или по const&, если прямой вызов.
     */
    void process(const Geometry geometry, const NestingParameters params);

    /**
     * @brief Запрос на остановку вычислений.
     */
    void stop();

signals:
    void finished(const NestingSolution &solution);
    void errorOccurred(const QString &message);
    void progressUpdated(const NestingSolution &currentBest);

private:
    GeneticOptimizer m_optimizer;
    std::atomic<bool> m_stopRequested{false};
};

#endif // OPTIMIZATIONWORKER_H
