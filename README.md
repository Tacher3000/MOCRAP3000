# MOCRAP3000

## Описание
MOCRAP3000 (multicriteria optimization of cutting and rational arrangement of parts for sheet materials) — это система для рациональной компоновки деталей для листовых материалов.

## Требования
- CMake 3.16+
- Qt 6 
- Компилятор с поддержкой C++17
- Git (для зависимостей)

## Установка и сборка

1. Клонируй репозиторий:
   ```bash
   git clone https://github.com/Tacher3000/MOCRAP3000.git
   cd MOCRAP3000
   ```

2. Создай папку сборки и запусти CMake:
   ```bash
   mkdir build
   cd build
   cmake ..
   ```

3. Скомпилируй проект:
   ```bash
   cmake --build .
   ```

## Зависимости
- **libdxfrw**: Автоматически скачивается через FetchContent из GitHub (https://github.com/qgis/libdxfrw.git).

## Структура проекта
- `src/`: Исходный код (UI, ядро, I/O).
- `resources/`: Ресурсы Qt .
- `build/`: Папка сборки (игнорируется в git).

## Контакты
- Автор: Tacher3000
- Email: tacherok3000@gmail.com