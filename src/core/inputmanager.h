#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

#include "geometry.h"
#include <string>

/**
 * @brief Класс для управления вводом данных, включая загрузку DXF-файлов.
 */
class InputManager {
public:
    /**
     * @brief Загружает геометрические данные из DXF-файла.
     * @param filePath Путь к файлу DXF.
     * @return Загруженная структура Geometry.
     */
    Geometry loadDxf(const std::string& filePath);
};

#endif // INPUTMANAGER_H
