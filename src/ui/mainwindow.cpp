#include "mainwindow.h"
#include "parameterswidget.h"
#include "viewerwidget.h"
#include "layoutviewerwidget.h"
#include "partlistwidget.h"
#include "sheetlistwidget.h"

#include <QMenuBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStatusBar>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QTimer>
#include <QScreen>
#include <QGuiApplication>

Q_DECLARE_METATYPE(NestingSolution)
Q_DECLARE_METATYPE(Geometry)
Q_DECLARE_METATYPE(NestingParameters)

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle(tr("MOCRAP3000 - Оптимизация раскроя"));

    qRegisterMetaType<NestingSolution>("NestingSolution");
    qRegisterMetaType<Geometry>("Geometry");
    qRegisterMetaType<NestingParameters>("NestingParameters");

    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->availableGeometry();
    setMinimumSize(screenGeometry.width() * 0.7, screenGeometry.height() * 0.7);
    move(screenGeometry.center() - frameGeometry().center());

    parametersDialog = new ParametersWidget(this);

    setupMenu();
    setupStatusBar();

    QWidget *central = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(5);

    mainSplitter = new QSplitter(Qt::Horizontal, this);
    mainSplitter->setStyleSheet(QStringLiteral("QSplitter::handle { background-color: #999999; width: 1px; }"));

    QWidget *leftPanel = new QWidget(this);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    QLabel* partsLabel = new QLabel(tr("<b>Загруженные детали:</b>"), this);
    leftLayout->addWidget(partsLabel);

    partList = new PartListWidget(this);
    leftLayout->addWidget(partList);

    connect(partList, &PartListWidget::partSelected, this, &MainWindow::onPartSelected);
    connect(partList, &PartListWidget::showAllPartsRequested, this, &MainWindow::onShowAllPartsRequested);

    SheetListWidget* sheetList = new SheetListWidget(this);
    leftLayout->addWidget(sheetList);

    btnStartOptimization = new QPushButton(tr("Запустить оптимизацию"), this);
    btnStartOptimization->setStyleSheet("font-size: 16px; font-weight: bold; padding: 10px; background-color: #4CAF50; color: white; border-radius: 4px;");
    leftLayout->addWidget(btnStartOptimization);

    mainSplitter->addWidget(leftPanel);

    viewersSplitter = new QSplitter(Qt::Vertical, this);
    viewersSplitter->setStyleSheet(QStringLiteral("QSplitter::handle { background-color: #999999; height: 1px; }"));

    viewer = new ViewerWidget(this);
    viewersSplitter->addWidget(viewer);

    layoutViewer = new LayoutViewerWidget(this);
    viewersSplitter->addWidget(layoutViewer);

    mainSplitter->addWidget(viewersSplitter);

    mainSplitter->setStretchFactor(0, 0);
    mainSplitter->setStretchFactor(1, 1);

    mainLayout->addWidget(mainSplitter);
    setCentralWidget(central);

    workerThread = new QThread(this);
    worker = new OptimizationWorker();
    worker->moveToThread(workerThread);

    connect(workerThread, &QThread::finished, worker, &QObject::deleteLater);
    connect(this, &MainWindow::startOptimizationRequested, worker, &OptimizationWorker::process);
    connect(worker, &OptimizationWorker::finished, this, &MainWindow::onOptimizationFinished);
    connect(worker, &OptimizationWorker::errorOccurred, this, &MainWindow::onOptimizationError);
    connect(worker, &OptimizationWorker::progressUpdated, this, &MainWindow::onOptimizationProgress);

    connect(btnStartOptimization, &QPushButton::clicked, this, &MainWindow::startOptimization);

    workerThread->start();
}

MainWindow::~MainWindow() {
    if (workerThread) {
        workerThread->quit();
        workerThread->wait();
    }
}

void MainWindow::setupStatusBar() {
    statusLabel = new QLabel(tr("Готов"), this);
    progressBar = new QProgressBar(this);
    progressBar->setRange(0, 0);
    progressBar->setVisible(false);
    progressBar->setMaximumWidth(200);

    statusBar()->addWidget(statusLabel);
    statusBar()->addPermanentWidget(progressBar);
}

void MainWindow::showEvent(QShowEvent *event) {
    QMainWindow::showEvent(event);
    if (firstShow) {
        firstShow = false;
        QTimer::singleShot(50, this, [this]() {
            int w = this->width();
            mainSplitter->setSizes(QList<int>() << w * 0.25 << w * 0.75);
            viewersSplitter->setSizes(QList<int>() << height() * 0.5 << height() * 0.5);
        });
    }
}

void MainWindow::setupMenu() {
    QMenu *fileMenu = menuBar()->addMenu(tr("Файл"));
    connect(fileMenu->addAction(tr("Загрузить DXF")), &QAction::triggered, this, &MainWindow::loadFile);
    connect(fileMenu->addAction(tr("Очистить список деталей")), &QAction::triggered, this, &MainWindow::clearAll);

    fileMenu->addSeparator();
    connect(fileMenu->addAction(tr("Настройки раскроя...")), &QAction::triggered, this, &MainWindow::showSettings);

    fileMenu->addSeparator();
    connect(fileMenu->addAction(tr("Сохранить раскройку")), &QAction::triggered, this, &MainWindow::saveFile);
    connect(fileMenu->addAction(tr("Сохранить раскройку как...")), &QAction::triggered, this, &MainWindow::saveAsFile);

    QMenu *helpMenu = menuBar()->addMenu(tr("Справка"));
    connect(helpMenu->addAction(tr("О программе")), &QAction::triggered, this, &MainWindow::showHelp);
}

void MainWindow::loadFile() {
    QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Открыть DXF файлы"), QString(), tr("DXF Files (*.dxf)"));
    if (fileNames.isEmpty()) return;

    int successCount = 0;

    for (const QString& fileName : fileNames) {
        try {
            Geometry loadedGeometry = inputManager.loadDxf(fileName.toStdString());
            currentGeometry.parts.insert(currentGeometry.parts.end(), loadedGeometry.parts.begin(), loadedGeometry.parts.end());

            partList->addGeometry(loadedGeometry);
            successCount++;
        } catch (const std::exception& e) {
            QMessageBox::critical(this, tr("Ошибка загрузки"), tr("Не удалось загрузить DXF файл: %1\n%2").arg(fileName).arg(QString::fromStdString(e.what())));
        }
    }

    if (successCount > 0) {
        viewer->setGeometry(currentGeometry);
        statusLabel->setText(tr("Добавлены новые детали. Загружено файлов: %1").arg(successCount));
    }
}

void MainWindow::clearAll() {
    currentGeometry.parts.clear();
    partList->clear();
    viewer->setGeometry(currentGeometry);
    statusLabel->setText(tr("Список деталей полностью очищен."));
}

void MainWindow::showSettings() {
    parametersDialog->exec();
}

void MainWindow::onPartSelected(const Part& part) {
    Geometry tempGeom;
    tempGeom.parts.push_back(part);
    viewer->setGeometry(tempGeom);
    statusLabel->setText(tr("Просмотр детали: %1").arg(QString::fromStdString(part.name)));
}

void MainWindow::onShowAllPartsRequested() {
    viewer->setGeometry(currentGeometry);
    statusLabel->setText(tr("Отображаются все загруженные детали"));
}

void MainWindow::startOptimization() {
    Geometry activeGeometry = partList->getGeometryForOptimization();

    if (activeGeometry.parts.empty()) {
        QMessageBox::warning(this, tr("Ошибка раскроя"), tr("Нет деталей для раскроя. Убедитесь, что загружен файл и включена хотя бы одна деталь."));
        return;
    }

    partList->setDisabled(true);
    btnStartOptimization->setDisabled(true);

    statusLabel->setText(tr("Выполняется оптимизация..."));
    progressBar->setVisible(true);

    layoutViewer->clearHistory();

    NestingParameters params = parametersDialog->getNestingParameters();

    SheetListWidget* sheetList = this->findChild<SheetListWidget*>();
    if (sheetList) {
        params.sheets = sheetList->getSheets();
    }

    if (params.sheets.empty()) {
        QMessageBox::warning(this, tr("Ошибка"), tr("Добавьте хотя бы один лист для раскроя!"));
        return;
    }

    emit startOptimizationRequested(activeGeometry, params);
}

void MainWindow::onOptimizationFinished(const NestingSolution &solution) {
    partList->setDisabled(false);
    btnStartOptimization->setDisabled(false);

    statusLabel->setText(tr("Оптимизация завершена. Утилизация: %1%").arg(solution.utilization, 0, 'f', 2));
    progressBar->setVisible(false);

    layoutViewer->setLayoutSolution(solution);
}

void MainWindow::onOptimizationError(const QString &message) {
    partList->setDisabled(false);
    btnStartOptimization->setDisabled(false);

    statusLabel->setText(tr("Ошибка оптимизации"));
    progressBar->setVisible(false);

    QMessageBox::critical(this, tr("Ошибка алгоритма"), tr("Произошла ошибка при выполнении раскроя: ") + message);
}

void MainWindow::onOptimizationProgress(const NestingSolution &solution) {
    statusLabel->setText(tr("Поколение: %1. Утилизация: %2%")
                             .arg(solution.generation)
                             .arg(solution.utilization, 0, 'f', 2));
    layoutViewer->setLayoutSolution(solution);
}

void MainWindow::saveFile() {
    QMessageBox::information(this, tr("Сохранение"), tr("Функция сохранения раскройки будет реализована позже."));
}

void MainWindow::saveAsFile() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Сохранить раскройку как"), QString(), tr("Layout Files (*.lay);;All Files (*)"));
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Сохранение"), tr("Функция сохранения раскройки будет реализована позже."));
    }
}

void MainWindow::showHelp() {
    QMessageBox::about(this, tr("О программе MOCRAP3000"),
                       tr("<h2>MOCRAP3000</h2>"
                          "<p>Программа для раскроя листового материала на основе DXF файлов.</p>"
                          "<p>Версия: 0.5 (Selectable Parts)</p>"));
}
