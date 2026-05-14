#include "outputmanager.h"
#include "../io/dxfexporter.h"

bool OutputManager::exportDxf(const NestingSolution& solution, const std::string& filePath) {
    return DxfExporter::exportSolution(solution, filePath);
}
