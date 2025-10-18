#ifndef VIEWERWIDGET_H
#define VIEWERWIDGET_H

#include <QWidget>
#include <QGraphicsView>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QMouseEvent>
#include "../core/geometry.h"

class CustomGraphicsView : public QGraphicsView {
    Q_OBJECT

public:
    CustomGraphicsView(QWidget *parent = nullptr);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    bool rotating = false;
    QPoint lastPos;
};

class ViewerWidget : public QWidget {
    Q_OBJECT

public:
    ViewerWidget(QWidget *parent = nullptr);
    void setGeometry(const Geometry& geom);

private:
    CustomGraphicsView *view;
    QVBoxLayout *layout;
};

#endif
