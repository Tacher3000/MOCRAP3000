#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

#include "geometry.h"
#include <string>

class InputManager {
public:
    Geometry loadDxf(const std::string& filePath);
};

#endif // INPUTMANAGER_H
