#include "viewerwidget.h"
#include <QGraphicsScene>

ViewerWidget::ViewerWidget(QWidget *parent) : QWidget(parent) {
    layout = new QVBoxLayout(this);

    view = new QGraphicsView(this);
    view->setScene(new QGraphicsScene(this));
    layout->addWidget(view);

    // TODO: Загрузка и отображение детали (заглушка - пустая сцена)

    setLayout(layout);
}
