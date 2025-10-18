#include "inputmanager.h"
#include "dxfreader.h"

Geometry InputManager::loadDxf(const std::string& filePath) {
    DxfReader reader;
    reader.read(filePath);
    return reader.getGeometry();
}
