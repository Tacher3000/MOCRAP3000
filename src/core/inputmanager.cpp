#include "inputmanager.h"
#include "dxfreader.h"
#include "geometrynormalizer.h"
#include <stdexcept>
#include <vector>

Geometry InputManager::loadDxf(const std::string& filePath) {
    DxfReader reader;
    reader.read(filePath);
    Geometry rawGeometry = reader.getGeometry();

    if (rawGeometry.parts.empty()) {
        throw std::runtime_error("DXF file contains no recognizable parts.");
    }

    std::vector<Part> validParts;

    for (auto& part : rawGeometry.parts) {
        part.polygon = geometry::normalizePart(part);

        if (!part.polygon.contours.empty()) {
            double width = part.polygon.maxX - part.polygon.minX;
            double height = part.polygon.maxY - part.polygon.minY;

            if (width > 1e-5 || height > 1e-5) {
                part.id = ++m_globalPartId;


                if (part.name.find("Part_") == 0) {
                    part.name = "Part_" + std::to_string(part.id);
                }

                validParts.push_back(part);
            }
        }
    }

    rawGeometry.parts = std::move(validParts);

    if (rawGeometry.parts.empty()) {
        throw std::runtime_error("После фильтрации пустых элементов в DXF файле не осталось валидных деталей.");
    }

    return rawGeometry;
}
