#ifndef LAYOUTVIEWERWIDGET_H
#define LAYOUTVIEWERWIDGET_H

#include <QWidget>
#include <QGraphicsView>
#include <QVBoxLayout>
#include <QGraphicsScene>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QSlider>
#include <QLabel>
#include <vector>
#include "../core/layoutstructures.h"
#include "viewerwidget.h"

class LayoutViewerWidget : public QWidget {
    Q_OBJECT

public:
    LayoutViewerWidget(QWidget *parent = nullptr);
    void setLayoutSolution(const NestingSolution& solution);
    void clearHistory();

private slots:
    void onSliderValueChanged(int value);

private:
    void showSolution(int index);

    CustomGraphicsView *view;
    QVBoxLayout *layout;

    QSlider *historySlider;
    QLabel *sliderLabel;

    std::vector<NestingSolution> solutionHistory;
};

#endif // LAYOUTVIEWERWIDGET_H
