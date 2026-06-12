#include "parameterswidget.h"
#include <QDoubleValidator>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QSettings>

ParametersWidget::ParametersWidget(QWidget *parent) : QDialog(parent) {
    setWindowTitle(tr("Настройки раскроя"));
    setModal(true);
    setMinimumWidth(300);

    layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);

    QSettings settings("MOCRAP_Inc", "MOCRAP3000");
    QString savedSpacing = settings.value("nesting/partSpacing", "5.0").toString();
    QString savedThickness = settings.value("nesting/cutThickness", "2.0").toString();
    bool savedShowRemnants = settings.value("nesting/showRemnants", true).toBool();
    int savedRotations = settings.value("nesting/allowedRotations", 4).toInt();
    QString savedMargin = settings.value("nesting/sheetMargin", "5.0").toString();

    QGroupBox *spacingGroup = new QGroupBox(tr("Отступы (мм)"), this);
    QGridLayout *spacingLayout = new QGridLayout(spacingGroup);

    spacingLayout->addWidget(new QLabel(tr("От края листа:")), 0, 0);
    sheetMargin = new QLineEdit(savedMargin, this);
    spacingLayout->addWidget(sheetMargin, 0, 1);

    spacingLayout->addWidget(new QLabel(tr("Между деталями:")), 1, 0);
    partSpacing = new QLineEdit(savedSpacing, this);
    spacingLayout->addWidget(partSpacing, 1, 1);

    spacingLayout->addWidget(new QLabel(tr("Толщина реза:")), 2, 0);
    cutThickness = new QLineEdit(savedThickness, this);
    spacingLayout->addWidget(cutThickness, 2, 1);

    layout->addWidget(spacingGroup);
    layout->addStretch();

    QHBoxLayout *rotationsSpinLayout = new QHBoxLayout(this);
    rotationsSpinLayout->addWidget(new QLabel(tr("Кол-во поворотов:")));
    rotationsSpin = new QSpinBox(this);
    rotationsSpin->setRange(1, 360);
    rotationsSpin->setValue(savedRotations);
    rotationsSpinLayout->addWidget(rotationsSpin);
    layout->addLayout(rotationsSpinLayout);

    showRemnantsCheck = new QCheckBox(tr("Визуализировать полезные остатки"), this);
    showRemnantsCheck->setChecked(savedShowRemnants);
    layout->addWidget(showRemnantsCheck);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Применить"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Закрыть"));

    connect(buttonBox, &QDialogButtonBox::accepted, this, &ParametersWidget::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttonBox);

    QDoubleValidator *validator = new QDoubleValidator(this);
    validator->setBottom(0.0);
    partSpacing->setValidator(validator);
    cutThickness->setValidator(validator);
}

void ParametersWidget::accept() {
    QSettings settings("MOCRAP_Inc", "MOCRAP3000");
    settings.setValue("nesting/sheetMargin", sheetMargin->text());
    settings.setValue("nesting/partSpacing", partSpacing->text());
    settings.setValue("nesting/cutThickness", cutThickness->text());
    settings.setValue("nesting/showRemnants", showRemnantsCheck->isChecked());
    settings.setValue("nesting/allowedRotations", rotationsSpin->value());

    QDialog::accept();
}

NestingParameters ParametersWidget::getNestingParameters() const {
    NestingParameters params;

    params.sheetMargin = sheetMargin->text().replace(',', '.').toDouble();
    params.partSpacing = partSpacing->text().replace(',', '.').toDouble();
    params.cutThickness = cutThickness->text().replace(',', '.').toDouble();
    params.showRemnants = showRemnantsCheck->isChecked();
    params.allowedRotations = rotationsSpin->value();

    return params;
}
