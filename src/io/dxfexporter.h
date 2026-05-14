#ifndef DXFEXPORTER_H
#define DXFEXPORTER_H

#include <string>
#include "../core/layoutstructures.h"

class DxfExporter {
public:
    /**
     * @brief Сохраняет результат раскроя (NestingSolution) в формат DXF.
     * Листы сохраняются на слое "SHEETS", а детали на слое "PARTS".
     */
    static bool exportSolution(const NestingSolution& solution, const std::string& filename);
};

#endif // DXFEXPORTER_H
