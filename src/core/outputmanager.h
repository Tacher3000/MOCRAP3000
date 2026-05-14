#ifndef OUTPUTMANAGER_H
#define OUTPUTMANAGER_H

#include "layoutstructures.h"
#include <string>

/**
 * @brief Фасад для экспорта данных.
 * Изолирует UI от прямой работы с модулем IO.
 */
class OutputManager {
public:
    /**
     * @brief Экспортирует результат раскроя в файл.
     * @param solution Готовая раскройка.
     * @param filePath Путь к файлу для сохранения (DXF).
     * @return true в случае успеха, иначе false.
     */
    bool exportDxf(const NestingSolution& solution, const std::string& filePath);
};

#endif // OUTPUTMANAGER_H
