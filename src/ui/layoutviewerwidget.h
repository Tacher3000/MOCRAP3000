#ifndef LAYOUTVIEWERWIDGET_H
#define LAYOUTVIEWERWIDGET_H

#include <QWidget>
#include <QGraphicsView>
#include <QVBoxLayout>

class LayoutViewerWidget : public QWidget {
    Q_OBJECT

public:
    LayoutViewerWidget(QWidget *parent = nullptr);

private:
    QGraphicsView *view;

    QVBoxLayout *layout;
};

#endif // LAYOUTVIEWERWIDGET_H
