#include "parameterswidget.h"

ParametersWidget::ParametersWidget(QWidget *parent) : QWidget(parent) {
    layout = new QVBoxLayout(this);

    sheetSizeLabel = new QLabel(tr("Размер листа (ширина x высота):"), this);
    sheetSizeLabel->setStyleSheet("font-size: 16px; padding: 5px;");
    layout->addWidget(sheetSizeLabel);
    QHBoxLayout *sheetLayout = new QHBoxLayout();
    sheetWidth = new QLineEdit(tr("1000"), this);
    sheetWidth->setStyleSheet("font-size: 16px; padding: 5px; min-height: 30px;");
    sheetHeight = new QLineEdit(tr("2000"), this);
    sheetHeight->setStyleSheet("font-size: 16px; padding: 5px; min-height: 30px;");
    sheetLayout->addWidget(sheetWidth);
    sheetLayout->addWidget(new QLabel("x", this));
    sheetLayout->addWidget(sheetHeight);
    layout->addLayout(sheetLayout);

    partSpacingLabel = new QLabel(tr("Расстояние между деталями:"), this);
    partSpacingLabel->setStyleSheet("font-size: 16px; padding: 5px;");
    layout->addWidget(partSpacingLabel);
    partSpacing = new QLineEdit(tr("5"), this);
    partSpacing->setStyleSheet("font-size: 16px; padding: 5px; min-height: 30px;");
    layout->addWidget(partSpacing);

    cutThicknessLabel = new QLabel(tr("Толщина реза:"), this);
    cutThicknessLabel->setStyleSheet("font-size: 16px; padding: 5px;");
    layout->addWidget(cutThicknessLabel);
    cutThickness = new QLineEdit(tr("2"), this);
    cutThickness->setStyleSheet("font-size: 16px; padding: 5px; min-height: 30px;");
    layout->addWidget(cutThickness);

    partCountLabel = new QLabel(tr("Количество деталей:"), this);
    partCountLabel->setStyleSheet("font-size: 16px; padding: 5px;");
    layout->addWidget(partCountLabel);
    partCount = new QLineEdit(tr("10"), this);
    partCount->setStyleSheet("font-size: 16px; padding: 5px; min-height: 30px;");
    layout->addWidget(partCount);

    optimizeButton = new QPushButton(tr("Оптимизировать"), this);
    optimizeButton->setStyleSheet("font-size: 16px; padding: 5px; min-height: 30px;");
    layout->addWidget(optimizeButton);

    layout->addStretch();

    // TODO: Соединить optimizeButton с CoreLib для оптимизации и обновления layoutViewer

    setLayout(layout);
}
