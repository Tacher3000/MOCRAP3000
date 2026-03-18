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

    QGroupBox *spacingGroup = new QGroupBox(tr("Отступы (мм)"), this);
    QHBoxLayout *spacingLayout = new QHBoxLayout(spacingGroup);

    spacingLayout->addWidget(new QLabel(tr("Между деталями:")));
    partSpacing = new QLineEdit(savedSpacing, this);
    spacingLayout->addWidget(partSpacing);

    spacingLayout->addWidget(new QLabel(tr("Толщина реза:")));
    cutThickness = new QLineEdit(savedThickness, this);
    spacingLayout->addWidget(cutThickness);

    layout->addWidget(spacingGroup);
    layout->addStretch();

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
    settings.setValue("nesting/partSpacing", partSpacing->text());
    settings.setValue("nesting/cutThickness", cutThickness->text());

    QDialog::accept();
}

NestingParameters ParametersWidget::getNestingParameters() const {
    NestingParameters params;

    params.partSpacing = partSpacing->text().replace(',', '.').toDouble();
    params.cutThickness = cutThickness->text().replace(',', '.').toDouble();

    return params;
}
