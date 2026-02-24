#include "geometrypainter.h"
#include "geometryutils.h"
#include <QGraphicsLineItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsPathItem>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QPainterPath>
#include <cmath>
#include <QDebug>
#include <QTransform>

void GeometryPainter::drawPart(QGraphicsScene *scene, const Part& part, const QPen& pen, const QTransform& transform) {
    for (const auto& line : part.lines) {
        QGraphicsLineItem* item = scene->addLine(line.start.x, line.start.y, line.end.x, line.end.y);
        item->setPen(pen);
        item->setTransform(transform);
    }

    for (const auto& arc : part.arcs) {
        QPainterPath path;

        double startAngleDeg = -arc.startAngle * 180.0 / pi;
        double endAngleDeg = -arc.endAngle * 180.0 / pi;

        double spanAngleDeg;
        if (arc.isCounterClockwise) {
            spanAngleDeg = endAngleDeg - startAngleDeg;
        } else {
            spanAngleDeg = startAngleDeg - endAngleDeg;
            spanAngleDeg = -spanAngleDeg;
        }
        QRectF rect(arc.center.x - arc.radius,
                    arc.center.y - arc.radius,
                    arc.radius * 2.0,
                    arc.radius * 2.0);

        path.arcMoveTo(rect, startAngleDeg);
        path.arcTo(rect, startAngleDeg, spanAngleDeg);

        QGraphicsPathItem* item = scene->addPath(path);
        item->setPen(pen);
        item->setTransform(transform);
    }

    for (const auto& circle : part.circles) {
        QGraphicsEllipseItem* item = scene->addEllipse(
            circle.center.x - circle.radius,
            circle.center.y - circle.radius,
            circle.radius * 2.0,
            circle.radius * 2.0
            );
        item->setPen(pen);
        item->setTransform(transform);
    }

    for (const auto& poly : part.lwpolylines) {
        QPainterPath path;
        if (poly.vertices.empty()) continue;

        path.moveTo(poly.vertices[0].x, poly.vertices[0].y);

        bool closed = (poly.flags & 1) != 0;
        size_t numSegments = closed ? poly.vertices.size() : poly.vertices.size() - 1;

        for (size_t i = 0; i < numSegments; ++i) {
            size_t j = (i + 1) % poly.vertices.size();
            const auto& v1 = poly.vertices[i];
            const auto& v2 = poly.vertices[j];
            double dx = v2.x - v1.x;
            double dy = v2.y - v1.y;

            if (std::abs(v1.bulge) < 1e-10) {
                path.lineTo(v2.x, v2.y);
            } else {
                double bulge = v1.bulge;
                bool ccw = bulge > 0;
                double absBulge = std::abs(bulge);

                double theta = 4 * std::atan(absBulge);
                double chord = std::sqrt(dx * dx + dy * dy);

                double r = chord / (2 * std::sin(theta / 2.0));
                double h = r * std::cos(theta / 2.0);

                double mx = (v1.x + v2.x) / 2.0;
                double my = (v1.y + v2.y) / 2.0;

                double ux = -dy / chord;
                double uy = dx / chord;

                if (!ccw) {
                    ux = -ux;
                    uy = -uy;
                }

                double cx = mx + h * ux;
                double cy = my + h * uy;

                double sa = std::atan2(v1.y - cy, v1.x - cx);

                double span = theta * 180.0 / pi;

                if (!ccw) {
                    span = -span;
                }

                path.arcTo(cx - r, cy - r, 2 * r, 2 * r, sa * 180.0 / pi, span);
            }
        }
        QGraphicsPathItem* item = scene->addPath(path);
        item->setPen(pen);
        item->setTransform(transform);
    }
}

// void GeometryPainter::drawGeometry(QGraphicsScene *scene, const Geometry& geom, const QPen& pen) {
//     if (geom.parts.empty()) {
//         qDebug() << "GeometryPainter: No geometry parts to display.";
//         return;
//     }

//     try {
//         const Part& singlePart = geom.getSinglePart();
//         drawPart(scene, singlePart, pen);
//     } catch (const std::runtime_error& e) {
//         qWarning() << "Error in GeometryPainter::drawGeometry:" << e.what();
//     }
// }

void GeometryPainter::drawGeometry(QGraphicsScene *scene, const Geometry& geom, const QPen& pen) {
    if (geom.parts.empty()) return;
    drawPart(scene, geom.parts[0], pen, QTransform());
}

// void GeometryPainter::drawNestingSolution(QGraphicsScene *scene, const NestingSolution& solution) {
//     if (solution.usedSheets.empty()) return;

//     // --- Настройка стилей ---
//     QPen sheetPen(Qt::black); sheetPen.setWidth(0);
//     QPen partPen(Qt::blue); partPen.setWidth(0);

//     QPen debugPen(Qt::red); debugPen.setWidth(0);
//     debugPen.setStyle(Qt::DashLine);
//     QBrush debugBrush(QColor(255, 0, 0, 50));

//     // Параметры для расчета "красной зоны" (должны совпадать с Optimizer)
//     double spacing = 5.0;
//     double thickness = 2.0;
//     double offset = (spacing + thickness) / 2.0;

//     double currentSheetOffsetX = 0.0;
//     const double margin = 50.0;

//     // 1. Рисуем листы
//     for (const auto& sheet : solution.usedSheets) {
//         scene->addRect(currentSheetOffsetX, 0, sheet.width, sheet.height, sheetPen, Qt::NoBrush);
//         QGraphicsTextItem* t = scene->addText(QString("Лист %1").arg(sheet.id));
//         t->setPos(currentSheetOffsetX, -30);
//         currentSheetOffsetX += sheet.width + margin;
//     }

//     double sheetStepX = 0;
//     if (!solution.usedSheets.empty()) {
//         sheetStepX = solution.usedSheets[0].width + margin;
//     }

//     // 2. Рисуем детали (Синие + Красные призраки)
//     for (const auto& placedPart : solution.placedParts) {
//         auto it = solution.partsMap.find(placedPart.originalPartId);
//         if (it == solution.partsMap.end()) continue;
//         const Part& originalPart = it->second;

//         // --- ЭТАП 1: Подготовка геометрии (Повторяем логику Optimizer) ---

//         // 1. Исходный контур и его нормализация
//         QPainterPath rawPath = geometry::partToPath(originalPart);
//         QRectF rawRect = rawPath.boundingRect();

//         // Матрица нормализации: сдвигает TopLeft исходной детали в (0,0)
//         QTransform normT = QTransform::fromTranslate(-rawRect.left(), -rawRect.top());
//         QPainterPath baseShape = normT.map(rawPath); // Деталь в (0,0)

//         // 2. Создаем "толстую" версию (для расчета коллизий)
//         QPainterPath expandedShape = geometry::expandPath(baseShape, offset);

//         // 3. Поворот
//         QTransform rotT;
//         rotT.rotate(placedPart.rotation);

//         // Поворачиваем "толстую" фигуру, чтобы узнать, как изменился её BoundingBox
//         QPainterPath rotatedExpanded = rotT.map(expandedShape);
//         QRectF rotRect = rotatedExpanded.boundingRect();

//         // 4. ВЫЧИСЛЕНИЕ СДВИГА (ALIGNMENT) - Самое важное!
//         // После поворота TopLeft угол может стать отрицательным (или сместиться).
//         // Нам нужно вернуть его в (0,0), так как Optimizer выдает координаты относительно TopLeft.
//         double alignX = -rotRect.left();
//         double alignY = -rotRect.top();
//         QTransform alignT = QTransform::fromTranslate(alignX, alignY);

//         // 5. Размещение на листе
//         double sheetOffset = (placedPart.sheetId - 1) * sheetStepX;
//         double finalX = sheetOffset + placedPart.x;
//         double finalY = placedPart.y;
//         QTransform placeT = QTransform::fromTranslate(finalX, finalY);

//         // --- ЭТАП 2: Сборка итоговой матрицы ---

//         // Полная матрица трансформации = Norm -> Rot -> Align -> Place
//         // Порядок перемножения в Qt: A * B означает "Сначала примени A, потом B" (если применять к точке)
//         // Но QTransform перемножаются как (T_last * ... * T_first).
//         // Проще всего задать цепочку трансформаций логически:

//         QTransform finalMatrix = normT * rotT * alignT * placeT;

//         // --- ЭТАП 3: Отрисовка ---

//         // А) Рисуем Синюю (чистовую) деталь
//         // Применяем полную матрицу к исходной детали
//         // (drawPart внутри делает scene->addPath(path * transform))
//         drawPart(scene, originalPart, partPen, finalMatrix);

//         // Б) Рисуем Красного призрака (Отладка)
//         // Чтобы нарисовать его точно там же, берем baseShape (который уже нормализован),
//         // расширяем его и прогоняем через оставшуюся часть матрицы (без normT, т.к. уже нормализован)

//         QTransform debugMatrix = rotT * alignT * placeT;

//         // Берем нормализованную форму, расширяем её (получаем expandedShape в 0,0)
//         // И применяем к ней поворот, выравнивание и размещение.
//         scene->addPath(debugMatrix.map(expandedShape), debugPen, debugBrush);
//     }
// }



void GeometryPainter::drawNestingSolution(QGraphicsScene *scene, const NestingSolution& solution) {
    if (solution.usedSheets.empty()) return;

    QPen sheetPen(Qt::black, 0);
    QPen partPen(Qt::blue, 0);

    QPen safeZonePen(Qt::red, 0, Qt::DashLine);

    // Отступ для визуализации красной зоны (должен совпадать с логикой Optimizer)
    // TODO: Передавать это значение внутри NestingSolution
    double offset = 3.5;

    double currentSheetOffsetX = 0.0;
    const double margin = 50.0;

    // Шаг между листами для визуализации
    double sheetVizStep = 0;
    if (!solution.usedSheets.empty()) {
        sheetVizStep = solution.usedSheets[0].width + margin;
    }

    // 1. Рисуем рамки листов
    for (const auto& sheet : solution.usedSheets) {
        scene->addRect(currentSheetOffsetX, 0, sheet.width, sheet.height, sheetPen, Qt::NoBrush);

        QGraphicsTextItem* t = scene->addText(QStringLiteral("Лист %1").arg(sheet.id));
        t->setPos(currentSheetOffsetX, -25);
        // Переворачиваем текст, т.к. view scale(1, -1)
        t->setTransform(QTransform::fromScale(1, -1), true);

        currentSheetOffsetX += sheet.width + margin;
    }

    // 2. Рисуем детали
    for (const auto& placedPart : solution.placedParts) {
        auto it = solution.partsMap.find(placedPart.originalPartId);
        if (it == solution.partsMap.end()) continue;

        const Part& originalPart = it->second;
        QPainterPath path = geometry::partToPath(originalPart);
        QRectF bRect = path.boundingRect();

        // --- УНИФИЦИРОВАННАЯ МАТРИЦА ТРАНСФОРМАЦИИ ---
        // Логика должна совпадать с GeneticOptimizer::decode 1 в 1

        QTransform transform;

        // ШАГ 1: Нормализация исходной детали (Top-Left -> 0,0)
        transform.translate(-bRect.left(), -bRect.top());

        // ШАГ 2: Вращение (вокруг 0,0)
        transform.rotate(placedPart.rotation);

        // ШАГ 3: Ре-нормализация (Новый Top-Left -> 0,0)
        // Нам нужно узнать bounding rect ПОСЛЕ вращения шага 2
        // Применяем текущую матрицу к пути, чтобы узнать новый rect
        QRectF rotatedRect = transform.map(path).boundingRect();
        // Сдвигаем обратно в ноль
        transform.translate(-rotatedRect.left(), -rotatedRect.top());

        // ШАГ 4: Размещение на листе
        double sheetOffset = (placedPart.sheetId - 1) * sheetVizStep;
        transform.translate(sheetOffset + placedPart.x, placedPart.y);

        // --- ОТРИСОВКА ---

        // Рисуем деталь (синяя)
        drawPart(scene, originalPart, partPen, transform);

        // Рисуем красную зону (красный пунктир)
        // Применяем ту же трансформацию к исходному пути, затем расширяем
        QPainterPath finalPath = transform.map(path);
        QPainterPath expandedPath = geometry::expandPath(finalPath, offset);

        // expandPath может создать сложную геометрию, упрощаем для скорости отрисовки
        // expandedPath = expandedPath.simplified();
        scene->addPath(expandedPath, safeZonePen, Qt::NoBrush);
    }
}
