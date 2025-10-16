#include "layoutviewerwidget.h"
#include <QGraphicsScene>

LayoutViewerWidget::LayoutViewerWidget(QWidget *parent) : QWidget(parent) {
    layout = new QVBoxLayout(this);

    view = new QGraphicsView(this);
    view->setScene(new QGraphicsScene(this));
    layout->addWidget(view);

    // Заглушка для отображения раскройки (пустая сцена, TODO: обновлять после оптимизации)

    setLayout(layout);
}
