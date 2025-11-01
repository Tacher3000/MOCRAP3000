#include "inputmanager.h"
#include "dxfreader.h"
#include "geometrynormalizer.h"
#include <stdexcept>

Geometry InputManager::loadDxf(const std::string& filePath) {
    DxfReader reader;
    reader.read(filePath);
    Geometry rawGeometry = reader.getGeometry();

    if (rawGeometry.parts.empty()) {
        throw std::runtime_error("DXF file contains no recognizable parts.");
    }

    for (auto& part : rawGeometry.parts) {
        part.polygon = geometry::normalizePart(part);

        // if (part.polygon.contours.empty()) {
        //     throw std::runtime_error("Part geometry could not be normalized to a polygon. Check for unclosed contours or unsupported entities.");
        // }
    }

    return rawGeometry;
}
