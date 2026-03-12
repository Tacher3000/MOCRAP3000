#include "layoutviewerwidget.h"
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QPen>
#include <QBrush>
#include <QColor>
#include <QDebug>
#include <QHBoxLayout>
#include "geometrypainter.h"
#include "viewerwidget.h"

LayoutViewerWidget::LayoutViewerWidget(QWidget *parent) : QWidget(parent) {
    layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    view = new CustomGraphicsView(this);
    view->setScene(new QGraphicsScene(this));
    layout->addWidget(view);

    QHBoxLayout *sliderLayout = new QHBoxLayout();
    sliderLayout->setContentsMargins(5, 5, 5, 5);

    sliderLabel = new QLabel(tr("Поколение: 0 / 0 (Утилизация: 0%)"), this);
    sliderLabel->setMinimumWidth(250);

    historySlider = new QSlider(Qt::Horizontal, this);
    historySlider->setEnabled(false);

    sliderLayout->addWidget(sliderLabel);
    sliderLayout->addWidget(historySlider);
    layout->addLayout(sliderLayout);

    connect(historySlider, &QSlider::valueChanged, this, &LayoutViewerWidget::onSliderValueChanged);

    setLayout(layout);
}

void LayoutViewerWidget::clearHistory() {
    solutionHistory.clear();

    historySlider->blockSignals(true);
    historySlider->setRange(0, 0);
    historySlider->setValue(0);
    historySlider->blockSignals(false);

    historySlider->setEnabled(false);
    sliderLabel->setText(tr("Поколение: 0 / 0 (Утилизация: 0%)"));

    QGraphicsScene* scene = view->scene();
    scene->clear();
    view->resetView();
}

void LayoutViewerWidget::setLayoutSolution(const NestingSolution& solution) {
    solutionHistory.push_back(solution);

    int maxIdx = static_cast<int>(solutionHistory.size()) - 1;
    historySlider->setEnabled(true);

    historySlider->blockSignals(true);
    historySlider->setMaximum(maxIdx);
    historySlider->setValue(maxIdx);
    historySlider->blockSignals(false);

    showSolution(maxIdx);
}

void LayoutViewerWidget::onSliderValueChanged(int value) {
    showSolution(value);
}

void LayoutViewerWidget::showSolution(int index) {
    if (index < 0 || index >= static_cast<int>(solutionHistory.size())) return;

    const NestingSolution& sol = solutionHistory[index];

    int currentGen = sol.generation;
    int maxGen = solutionHistory.back().generation;

    sliderLabel->setText(tr("Поколение: %1 / %2 (Утилизация: %3%)")
                             .arg(currentGen)
                             .arg(maxGen)
                             .arg(sol.utilization, 0, 'f', 2));

    QGraphicsScene* scene = view->scene();
    scene->clear();
    view->resetView();

    GeometryPainter::drawNestingSolution(scene, sol);

    view->fitInView(scene->itemsBoundingRect(), Qt::KeepAspectRatio);
}
