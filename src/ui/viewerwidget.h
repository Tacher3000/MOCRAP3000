#ifndef VIEWERWIDGET_H
#define VIEWERWIDGET_H

#include <QWidget>
#include <QGraphicsView>
#include <QVBoxLayout>

class ViewerWidget : public QWidget {
    Q_OBJECT

public:
    ViewerWidget(QWidget *parent = nullptr);

private:
    QGraphicsView *view;

    QVBoxLayout *layout;
};

#endif // VIEWERWIDGET_H
