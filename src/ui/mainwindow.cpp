#include "mainwindow.h"
#include "parameterswidget.h"
#include "viewerwidget.h"
#include "layoutviewerwidget.h"

#include <QMenuBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWidget>
#include <QShowEvent>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setupMenu();

    QWidget *central = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    horizontalSplitter = new QSplitter(Qt::Horizontal, this);
    horizontalSplitter->setStyleSheet("QSplitter::handle { background-color: #999999; width: 1px; }");

    parameters = new ParametersWidget(this);
    horizontalSplitter->addWidget(parameters);

    verticalSplitter = new QSplitter(Qt::Vertical, this);
    verticalSplitter->setStyleSheet("QSplitter::handle { background-color: #999999; height: 1px; }");

    viewer = new ViewerWidget(this);
    verticalSplitter->addWidget(viewer);

    layoutViewer = new LayoutViewerWidget(this);
    verticalSplitter->addWidget(layoutViewer);

    horizontalSplitter->addWidget(verticalSplitter);

    horizontalSplitter->setSizes(QList<int>() << 300 << 900);
    verticalSplitter->setSizes(QList<int>() << 400 << 400);

    mainLayout->addWidget(horizontalSplitter);
    central->setLayout(mainLayout);
    setCentralWidget(central);
}

MainWindow::~MainWindow() {}

void MainWindow::showEvent(QShowEvent *event) {
    QMainWindow::showEvent(event);

    if (firstShow) {
        firstShow = false;

        QTimer::singleShot(50, this, [this]() {
            int w = this->width();
            horizontalSplitter->setSizes(QList<int>() << w * 0.25 << w * 0.75);
            verticalSplitter->setSizes(QList<int>() << height() * 0.5 << height() * 0.5);
        });
    }
}

void MainWindow::setupMenu() {
    QMenu *fileMenu = menuBar()->addMenu(tr("Файл"));
    QAction *loadAct = fileMenu->addAction(tr("Загрузить"));
    connect(loadAct, &QAction::triggered, this, &MainWindow::loadFile);

    QAction *saveAct = fileMenu->addAction(tr("Сохранить"));
    connect(saveAct, &QAction::triggered, this, &MainWindow::saveFile);

    QAction *saveAsAct = fileMenu->addAction(tr("Сохранить как"));
    connect(saveAsAct, &QAction::triggered, this, &MainWindow::saveAsFile);

    QMenu *helpMenu = menuBar()->addMenu(tr("Справка"));
    QAction *helpAct = helpMenu->addAction(tr("О программе"));
    connect(helpAct, &QAction::triggered, this, &MainWindow::showHelp);
}

void MainWindow::loadFile() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Загрузить DXF"), "", tr("DXF Files (*.dxf)"));
    if (!fileName.isEmpty()) {
        Geometry geom = inputManager.loadDxf(fileName.toStdString());
        viewer->setGeometry(geom);
    }
}

void MainWindow::saveFile() {
    // TODO: Логика сохранения раскройки
}

void MainWindow::saveAsFile() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Сохранить как DXF"), "", tr("DXF Files (*.dxf)"));
    if (!fileName.isEmpty()) {
        // TODO: Использовать IoLib для сохранения раскройки
    }
}

void MainWindow::showHelp() {
    QMessageBox::information(this, tr("Справка"), tr("MOCRAP3000 - система оптимизации раскроя.\nВерсия 1.0"));
}
