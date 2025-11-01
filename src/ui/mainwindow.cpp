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
#include <QScreen>
#include <QGuiApplication>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle(tr("MOCRAP3000 - Оптимизация раскроя"));

    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->availableGeometry();
    setMinimumSize(screenGeometry.width() * 0.7, screenGeometry.height() * 0.7);
    move(screenGeometry.center() - frameGeometry().center());

    setupMenu();

    QWidget *central = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    horizontalSplitter = new QSplitter(Qt::Horizontal, this);
    horizontalSplitter->setStyleSheet("QSplitter::handle { background-color: #999999; width: 1px; }");

    parameters = new ParametersWidget(this);
    parameters->setMinimumWidth(250);
    horizontalSplitter->addWidget(parameters);

    verticalSplitter = new QSplitter(Qt::Vertical, this);
    verticalSplitter->setStyleSheet("QSplitter::handle { background-color: #999999; height: 1px; }");

    viewer = new ViewerWidget(this);
    verticalSplitter->addWidget(viewer);

    layoutViewer = new LayoutViewerWidget(this);
    verticalSplitter->addWidget(layoutViewer);

    horizontalSplitter->addWidget(verticalSplitter);

    QList<int> h_sizes;
    h_sizes << 300 << 1200;
    horizontalSplitter->setSizes(h_sizes);

    QList<int> v_sizes;
    v_sizes << 1 << 1;
    verticalSplitter->setSizes(v_sizes);


    mainLayout->addWidget(horizontalSplitter);
    setCentralWidget(central);

    connect(parameters, &ParametersWidget::optimizeRequested, this, &MainWindow::optimizeLayout);
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
    QAction *loadAct = fileMenu->addAction(tr("Загрузить DXF"));
    connect(loadAct, &QAction::triggered, this, &MainWindow::loadFile);

    QAction *saveAct = fileMenu->addAction(tr("Сохранить раскройку"));
    connect(saveAct, &QAction::triggered, this, &MainWindow::saveFile);

    QAction *saveAsAct = fileMenu->addAction(tr("Сохранить раскройку как"));
    connect(saveAsAct, &QAction::triggered, this, &MainWindow::saveAsFile);

    QMenu *helpMenu = menuBar()->addMenu(tr("Справка"));
    QAction *helpAct = helpMenu->addAction(tr("О программе"));
    connect(helpAct, &QAction::triggered, this, &MainWindow::showHelp);
}

void MainWindow::loadFile() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Открыть DXF файл"), "", tr("DXF Files (*.dxf)"));
    if (!fileName.isEmpty()) {
        try {
            currentGeometry = inputManager.loadDxf(fileName.toStdString());
            viewer->setGeometry(currentGeometry);
        } catch (const std::exception& e) {
            QMessageBox::critical(this, tr("Ошибка загрузки"), tr("Не удалось загрузить DXF файл: ") + QString::fromStdString(e.what()));
        }
    }
}

void MainWindow::saveFile() {
    QMessageBox::information(this, tr("Сохранение"), tr("Функция сохранения раскройки будет реализована позже."));
}

void MainWindow::saveAsFile() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Сохранить раскройку как"), "", tr("Layout Files (*.lay);;All Files (*)"));
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Сохранение"), tr("Функция сохранения раскройки будет реализована позже."));
    }
}

void MainWindow::showHelp() {
    QMessageBox::about(this, tr("О программе MOCRAP3000"),
                       tr("<h2>MOCRAP3000</h2>"
                          "<p>Программа для раскроя листового материала на основе DXF файлов.</p>"
                          "<p>Версия: 0.1</p>"
                          "<p>Разработчик: Tacher3000</p>"));
}

void MainWindow::optimizeLayout() {
    if (currentGeometry.parts.empty()) {
        QMessageBox::warning(this, tr("Ошибка раскроя"), tr("Сначала загрузите DXF файл."));
        return;
    }

    try {
        NestingParameters params = parameters->getNestingParameters();

        NestingSolution solution = optimizer.optimize(currentGeometry, params);

        layoutViewer->setLayoutSolution(solution);

    } catch (const std::exception& e) {
        QMessageBox::critical(this, tr("Ошибка алгоритма"), tr("Произошла ошибка при выполнении раскроя: ") + QString::fromStdString(e.what()));
    }
}
