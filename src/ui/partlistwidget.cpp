#include "partlistwidget.h"
#include <QHBoxLayout>
#include <QGraphicsScene>
#include <QStyle>
#include "geometrypainter.h"

// --- PartItemWidget ---
PartItemWidget::PartItemWidget(const Part& part, QWidget* parent)
    : QWidget(parent), m_part(part)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFixedHeight(95);

    setAttribute(Qt::WA_StyledBackground, true);
    setProperty("selected", false);
    setStyleSheet("PartItemWidget { background-color: transparent; border-bottom: 1px solid #ddd; margin: 2px; }"
                  "PartItemWidget[selected=\"true\"] { background-color: #d0e8ff; border: 1px solid #80bfff; border-radius: 4px; }");

    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);

    m_includeCheck = new QCheckBox(this);
    m_includeCheck->setChecked(true);
    mainLayout->addWidget(m_includeCheck);

    m_previewView = new QGraphicsView(this);
    m_previewView->setFixedSize(80, 80);
    m_previewView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_previewView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_previewView->setInteractive(false);

    m_previewView->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_previewView->setStyleSheet("background: white; border: 1px solid #ccc;");

    QGraphicsScene* scene = new QGraphicsScene(this);
    GeometryPainter::drawPart(scene, m_part, QPen(Qt::black, 0));
    m_previewView->setScene(scene);
    m_previewView->scale(1, -1);
    m_previewView->fitInView(scene->itemsBoundingRect(), Qt::KeepAspectRatio);
    mainLayout->addWidget(m_previewView);

    QVBoxLayout* infoLayout = new QVBoxLayout();
    QString name = QString::fromStdString(m_part.name);

    double width = m_part.polygon.maxX - m_part.polygon.minX;
    double height = m_part.polygon.maxY - m_part.polygon.minY;
    QString dimensions = QString("%1 x %2 мм").arg(width, 0, 'f', 1).arg(height, 0, 'f', 1);

    QLabel* nameLabel = new QLabel(QString("<b>%1</b>").arg(name));
    QLabel* dimLabel = new QLabel(dimensions);
    dimLabel->setStyleSheet("color: #555; font-size: 11px;");

    infoLayout->addWidget(nameLabel);
    infoLayout->addWidget(dimLabel);
    infoLayout->addStretch();
    mainLayout->addLayout(infoLayout, 1);

    m_quantitySpin = new QSpinBox(this);
    m_quantitySpin->setRange(1, 9999);
    m_quantitySpin->setValue(1);
    m_quantitySpin->setFixedWidth(60);

    QVBoxLayout* spinLayout = new QVBoxLayout();
    spinLayout->addWidget(new QLabel(tr("Кол-во:")));
    spinLayout->addWidget(m_quantitySpin);
    spinLayout->addStretch();
    mainLayout->addLayout(spinLayout);
}

void PartItemWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        emit clicked(this);
    }
    QWidget::mousePressEvent(event);
}

void PartItemWidget::setSelected(bool selected) {
    if (property("selected").toBool() != selected) {
        setProperty("selected", selected);
        style()->unpolish(this);
        style()->polish(this);
        update();
    }
}

bool PartItemWidget::isIncluded() const { return m_includeCheck->isChecked(); }
int PartItemWidget::getQuantity() const { return m_quantitySpin->value(); }
Part PartItemWidget::getPart() const { return m_part; }

PartListWidget::PartListWidget(QWidget* parent) : QWidget(parent) {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);

    m_listContainer = new QWidget(scrollArea);
    m_listLayout = new QVBoxLayout(m_listContainer);
    m_listLayout->setAlignment(Qt::AlignTop);

    scrollArea->setWidget(m_listContainer);
    layout->addWidget(scrollArea);
}

void PartListWidget::addGeometry(const Geometry& geometry) {
    for (const auto& part : geometry.parts) {
        PartItemWidget* itemWidget = new PartItemWidget(part, this);
        connect(itemWidget, &PartItemWidget::clicked, this, &PartListWidget::onItemClicked);
        m_listLayout->addWidget(itemWidget);
        m_items.push_back(itemWidget);
    }
}

void PartListWidget::clear() {
    for (auto item : m_items) {
        m_listLayout->removeWidget(item);
        delete item;
    }
    m_items.clear();
}

void PartListWidget::onItemClicked(PartItemWidget* item) {
    if (item->property("selected").toBool()) {
        item->setSelected(false);
        emit showAllPartsRequested();
    } else {
        for (auto* w : m_items) {
            w->setSelected(w == item);
        }
        emit partSelected(item->getPart());
    }
}

Geometry PartListWidget::getGeometryForOptimization() const {
    Geometry resultGeom;
    for (auto item : m_items) {
        if (item->isIncluded()) {
            int qty = item->getQuantity();
            for (int i = 0; i < qty; ++i) {
                resultGeom.parts.push_back(item->getPart());
            }
        }
    }
    return resultGeom;
}
