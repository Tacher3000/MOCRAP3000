#include "sheetlistwidget.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

SheetListWidget::SheetListWidget(QWidget *parent) : QWidget(parent) {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 5, 0, 5);

    mainLayout->addWidget(new QLabel(tr("<b>Склад листов:</b>"), this));

    QHBoxLayout *inputLayout = new QHBoxLayout();

    spinWidth = new QDoubleSpinBox();
    spinWidth->setRange(1, 10000);
    spinWidth->setValue(2000);
    spinWidth->setToolTip("Ширина");

    spinHeight = new QDoubleSpinBox();
    spinHeight->setRange(1, 10000);
    spinHeight->setValue(1000);
    spinHeight->setToolTip("Высота");

    spinQty = new QSpinBox();
    spinQty->setRange(1, 9999);
    spinQty->setValue(1);
    spinQty->setToolTip("Кол-во");

    checkInfinite = new QCheckBox(tr("Беск."));
    connect(checkInfinite, &QCheckBox::toggled, spinQty, &QSpinBox::setDisabled);

    QPushButton *btnAdd = new QPushButton(tr("+"));
    btnAdd->setFixedWidth(30);
    connect(btnAdd, &QPushButton::clicked, this, &SheetListWidget::addSheet);

    inputLayout->addWidget(spinWidth);
    inputLayout->addWidget(new QLabel("x"));
    inputLayout->addWidget(spinHeight);
    inputLayout->addWidget(spinQty);
    inputLayout->addWidget(checkInfinite);
    inputLayout->addWidget(btnAdd);

    mainLayout->addLayout(inputLayout);

    listLayout = new QVBoxLayout();
    mainLayout->addLayout(listLayout);

    addSheet();
}

void SheetListWidget::addSheet() {
    SheetRequest req;
    req.width = spinWidth->value();
    req.height = spinHeight->value();
    req.quantity = spinQty->value();
    req.isInfinite = checkInfinite->isChecked();

    QWidget *rowWidget = new QWidget(this);
    QHBoxLayout *rowLayout = new QHBoxLayout(rowWidget);
    rowLayout->setContentsMargins(0, 2, 0, 2);

    QString text = QString("%1 x %2 мм, %3")
                       .arg(req.width).arg(req.height)
                       .arg(req.isInfinite ? tr("Беск.") : QString::number(req.quantity) + tr(" шт."));

    rowLayout->addWidget(new QLabel(text));

    QPushButton *btnRemove = new QPushButton("X");
    btnRemove->setFixedWidth(30);
    rowLayout->addWidget(btnRemove);

    connect(btnRemove, &QPushButton::clicked, [this, rowWidget]() {
        for (auto it = m_sheets.begin(); it != m_sheets.end(); ++it) {
            if (it->widget == rowWidget) {
                m_sheets.erase(it);
                break;
            }
        }
        rowWidget->deleteLater();
    });

    m_sheets.push_back({req, rowWidget});
    listLayout->addWidget(rowWidget);
}

std::vector<SheetRequest> SheetListWidget::getSheets() const {
    std::vector<SheetRequest> res;
    for (const auto& s : m_sheets) res.push_back(s.data);
    return res;
}
