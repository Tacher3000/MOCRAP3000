#include "sheetlistwidget.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QGraphicsScene>
#include "geometrypainter.h"

SheetItemWidget::SheetItemWidget(const SheetRequest& req, QWidget* parent)
    : QWidget(parent), m_request(req)
{
    setFixedHeight(95);
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("SheetItemWidget { background-color: transparent; border-bottom: 1px solid #ddd; margin: 2px; }");

    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);

    QGraphicsView* previewView = new QGraphicsView(this);
    previewView->setFixedSize(80, 80);
    previewView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    previewView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    previewView->setInteractive(false);
    previewView->setAttribute(Qt::WA_TransparentForMouseEvents);
    previewView->setStyleSheet("background: white; border: 1px solid #ccc;");

    QGraphicsScene* scene = new QGraphicsScene(this);

    Part renderPart;
    if (req.isCustomShape && req.customShape.has_value()) {
        renderPart = req.customShape.value();
    } else {
        Line top{ {0, req.height}, {req.width, req.height} };
        Line right{ {req.width, req.height}, {req.width, 0} };
        Line bottom{ {req.width, 0}, {0, 0} };
        Line left{ {0, 0}, {0, req.height} };
        renderPart.lines = {top, right, bottom, left};
    }

    GeometryPainter::drawPart(scene, renderPart, QPen(Qt::black, 0));
    previewView->setScene(scene);
    previewView->scale(1, -1);
    previewView->fitInView(scene->itemsBoundingRect(), Qt::KeepAspectRatio);

    mainLayout->addWidget(previewView);

    QVBoxLayout* infoLayout = new QVBoxLayout();
    QString titleText;
    QString dimText;

    if (req.isCustomShape && req.customShape.has_value()) {
        titleText = QString::fromStdString(req.customShape.value().name);
        double width = req.customShape.value().polygon.maxX - req.customShape.value().polygon.minX;
        double height = req.customShape.value().polygon.maxY - req.customShape.value().polygon.minY;
        dimText = QString("%1 x %2 мм").arg(width, 0, 'f', 1).arg(height, 0, 'f', 1);
    } else {
        titleText = QString("Лист %1").arg(req.id);
        dimText = QString("%1 x %2 мм").arg(req.width).arg(req.height);
    }

    QString qtyText = req.isInfinite ? tr("Беск.") : QString("%1 шт.").arg(req.quantity);

    infoLayout->addWidget(new QLabel("<b>" + titleText + "</b>"));
    infoLayout->addWidget(new QLabel(dimText));
    infoLayout->addWidget(new QLabel(qtyText));
    infoLayout->addStretch();

    mainLayout->addLayout(infoLayout, 1);

    QPushButton* btnRemove = new QPushButton("X");
    btnRemove->setFixedWidth(30);
    connect(btnRemove, &QPushButton::clicked, this, [this]() { emit removeRequested(this); });
    mainLayout->addWidget(btnRemove);
}

SheetRequest SheetItemWidget::getRequest() const {
    return m_request;
}

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

    QPushButton *btnAdd = new QPushButton(tr("+ Прямоугольный"));
    connect(btnAdd, &QPushButton::clicked, this, &SheetListWidget::addSheet);

    inputLayout->addWidget(spinWidth);
    inputLayout->addWidget(new QLabel("x"));
    inputLayout->addWidget(spinHeight);
    inputLayout->addWidget(spinQty);
    inputLayout->addWidget(checkInfinite);
    inputLayout->addWidget(btnAdd);

    mainLayout->addLayout(inputLayout);

    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    QWidget *scrollContent = new QWidget(scrollArea);
    listLayout = new QVBoxLayout(scrollContent);
    listLayout->setAlignment(Qt::AlignTop); // Прижимаем элементы к верху
    listLayout->setContentsMargins(0, 0, 0, 0);

    scrollArea->setWidget(scrollContent);
    mainLayout->addWidget(scrollArea, 1); // stretch = 1 заставит ScrollArea занять доступное место

    addSheet();
}

void SheetListWidget::addSheet() {
    SheetRequest req;
    req.id = m_sheetIdCounter++;
    req.width = spinWidth->value();
    req.height = spinHeight->value();
    req.quantity = spinQty->value();
    req.isInfinite = checkInfinite->isChecked();

    SheetItemWidget* item = new SheetItemWidget(req, this);
    connect(item, &SheetItemWidget::removeRequested, this, [this](SheetItemWidget* w) {
        std::erase(m_sheetWidgets, w);
        w->deleteLater();
    });

    m_sheetWidgets.push_back(item);
    listLayout->addWidget(item);
}

void SheetListWidget::addCustomSheet(const Part& part) {
    SheetRequest req;
    req.id = m_sheetIdCounter++;
    req.quantity = spinQty->value();
    req.isInfinite = checkInfinite->isChecked();
    req.isCustomShape = true;
    req.customShape = part;

    SheetItemWidget* item = new SheetItemWidget(req, this);
    connect(item, &SheetItemWidget::removeRequested, this, [this](SheetItemWidget* w) {
        std::erase(m_sheetWidgets, w);
        w->deleteLater();
    });

    m_sheetWidgets.push_back(item);
    listLayout->addWidget(item);
}

std::vector<SheetRequest> SheetListWidget::getSheets() const {
    std::vector<SheetRequest> res;
    res.reserve(m_sheetWidgets.size());
    for (const auto& w : m_sheetWidgets) {
        res.push_back(w->getRequest());
    }
    return res;
}
