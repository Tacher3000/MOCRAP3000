#include "parameterswidget.h"
#include <QDoubleValidator>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QPushButton>

ParametersWidget::ParametersWidget(QWidget *parent) : QDialog(parent) {
    setWindowTitle(tr("Настройки раскроя"));
    setModal(true);
    setMinimumWidth(350);

    layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);

    QGroupBox *sheetGroup = new QGroupBox(tr("Размер листа (мм)"), this);
    QHBoxLayout *sheetLayout = new QHBoxLayout(sheetGroup);
    sheetWidth = new QLineEdit(tr("1000"), this);
    sheetHeight = new QLineEdit(tr("2000"), this);
    sheetLayout->addWidget(sheetWidth);
    sheetLayout->addWidget(new QLabel("x", this));
    sheetLayout->addWidget(sheetHeight);

    QGroupBox *spacingGroup = new QGroupBox(tr("Отступы (мм)"), this);
    QHBoxLayout *spacingLayout = new QHBoxLayout(spacingGroup);

    spacingLayout->addWidget(new QLabel(tr("Между деталями:")));
    partSpacing = new QLineEdit(tr("5"), this);
    spacingLayout->addWidget(partSpacing);

    spacingLayout->addWidget(new QLabel(tr("Толщина реза:")));
    cutThickness = new QLineEdit(tr("2"), this);
    spacingLayout->addWidget(cutThickness);

    layout->addWidget(sheetGroup);
    layout->addWidget(spacingGroup);
    layout->addStretch();

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Применить"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Закрыть"));
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttonBox);

    QDoubleValidator *validator = new QDoubleValidator(this);
    validator->setBottom(0.0);
    sheetWidth->setValidator(validator);
    sheetHeight->setValidator(validator);
    partSpacing->setValidator(validator);
    cutThickness->setValidator(validator);
}

NestingParameters ParametersWidget::getNestingParameters() const {
    return NestingParameters::fromStrings(
        sheetWidth->text().toStdString(),
        sheetHeight->text().toStdString()
        );
}
