#include "parameterswidget.h"
#include <QHBoxLayout>
#include <QDoubleValidator>

ParametersWidget::ParametersWidget(QWidget *parent) : QWidget(parent) {
    layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);

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

    optimizeButton = new QPushButton(tr("Запустить оптимизацию"), this);
    optimizeButton->setStyleSheet("font-size: 18px; padding: 10px 20px; margin-top: 20px;");
    layout->addWidget(optimizeButton);

    layout->addStretch();

    QDoubleValidator *validator = new QDoubleValidator(this);
    validator->setBottom(0.0);
    sheetWidth->setValidator(validator);
    sheetHeight->setValidator(validator);
    partSpacing->setValidator(validator);
    cutThickness->setValidator(validator);
    partCount->setValidator(new QIntValidator(1, 99999, this));

    connect(optimizeButton, &QPushButton::clicked, this, &ParametersWidget::optimizeRequested);

    setLayout(layout);
}

NestingParameters ParametersWidget::getNestingParameters() const {
    return NestingParameters::fromStrings(
        sheetWidth->text().toStdString(),
        sheetHeight->text().toStdString(),
        partSpacing->text().toStdString(),
        cutThickness->text().toStdString()
        );
}
