#include "mainwindow.h"
#include "parameterswidget.h"
#include "viewerwidget.h"
#include "layoutviewerwidget.h"

#include <QMenuBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QSplitter>
#include <QVBoxLayout>
#include <QStatusBar>
#include <QLabel>
#include <QProgressBar>
#include <QTimer>
#include <QScreen>
#include <QGuiApplication>

// Регистрация мета-типов для сигналов/слотов
Q_DECLARE_METATYPE(NestingSolution)
Q_DECLARE_METATYPE(Geometry)
Q_DECLARE_METATYPE(NestingParameters)

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle(tr("MOCRAP3000 - Оптимизация раскроя"));

    // Регистрация типов должна быть выполнена один раз
    qRegisterMetaType<NestingSolution>("NestingSolution");
    qRegisterMetaType<Geometry>("Geometry");
    qRegisterMetaType<NestingParameters>("NestingParameters");

    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->availableGeometry();
    setMinimumSize(screenGeometry.width() * 0.7, screenGeometry.height() * 0.7);
    move(screenGeometry.center() - frameGeometry().center());

    setupMenu();
    setupStatusBar();

    // --- Настройка UI ---
    QWidget *central = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    horizontalSplitter = new QSplitter(Qt::Horizontal, this);
    horizontalSplitter->setStyleSheet(QStringLiteral("QSplitter::handle { background-color: #999999; width: 1px; }"));

    parameters = new ParametersWidget(this);
    parameters->setMinimumWidth(250);
    horizontalSplitter->addWidget(parameters);

    verticalSplitter = new QSplitter(Qt::Vertical, this);
    verticalSplitter->setStyleSheet(QStringLiteral("QSplitter::handle { background-color: #999999; height: 1px; }"));

    viewer = new ViewerWidget(this);
    verticalSplitter->addWidget(viewer);

    layoutViewer = new LayoutViewerWidget(this);
    verticalSplitter->addWidget(layoutViewer);

    horizontalSplitter->addWidget(verticalSplitter);

    horizontalSplitter->setStretchFactor(0, 0);
    horizontalSplitter->setStretchFactor(1, 1);

    mainLayout->addWidget(horizontalSplitter);
    setCentralWidget(central);

    // --- Настройка Потоков ---
    workerThread = new QThread(this);
    worker = new OptimizationWorker(); // Без родителя, так как переносим в другой поток
    worker->moveToThread(workerThread);

    // Связи сигналов и слотов (QueuedConnection по умолчанию для разных потоков)
    connect(workerThread, &QThread::finished, worker, &QObject::deleteLater);

    // UI -> Worker
    connect(this, &MainWindow::startOptimizationRequested, worker, &OptimizationWorker::process);

    // Worker -> UI
    connect(worker, &OptimizationWorker::finished, this, &MainWindow::onOptimizationFinished);
    connect(worker, &OptimizationWorker::errorOccurred, this, &MainWindow::onOptimizationError);

    // Подключение сигнала прогресса
    connect(worker, &OptimizationWorker::progressUpdated, this, &MainWindow::onOptimizationProgress);

    connect(parameters, &ParametersWidget::optimizeRequested, this, &MainWindow::startOptimization);

    workerThread->start();
}

MainWindow::~MainWindow() {
    // Корректная остановка потока
    if (workerThread) {
        workerThread->quit();
        workerThread->wait();
    }
}

void MainWindow::setupStatusBar() {
    statusLabel = new QLabel(tr("Готов"), this);
    progressBar = new QProgressBar(this);
    progressBar->setRange(0, 0); // Indeterminate mode
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
            horizontalSplitter->setSizes(QList<int>() << w * 0.25 << w * 0.75);
            verticalSplitter->setSizes(QList<int>() << height() * 0.5 << height() * 0.5);
        });
    }
}

void MainWindow::setupMenu() {
    QMenu *fileMenu = menuBar()->addMenu(tr("Файл"));

    // Новый синтаксис коннектов
    connect(fileMenu->addAction(tr("Загрузить DXF")), &QAction::triggered, this, &MainWindow::loadFile);
    connect(fileMenu->addAction(tr("Сохранить раскройку")), &QAction::triggered, this, &MainWindow::saveFile);
    connect(fileMenu->addAction(tr("Сохранить раскройку как")), &QAction::triggered, this, &MainWindow::saveAsFile);

    QMenu *helpMenu = menuBar()->addMenu(tr("Справка"));
    connect(helpMenu->addAction(tr("О программе")), &QAction::triggered, this, &MainWindow::showHelp);
}

void MainWindow::loadFile() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Открыть DXF файл"), QString(), tr("DXF Files (*.dxf)"));
    if (!fileName.isEmpty()) {
        try {
            currentGeometry = inputManager.loadDxf(fileName.toStdString());
            viewer->setGeometry(currentGeometry);
            statusLabel->setText(tr("Файл загружен: %1").arg(fileName));
        } catch (const std::exception& e) {
            QMessageBox::critical(this, tr("Ошибка загрузки"), tr("Не удалось загрузить DXF файл: ") + QString::fromStdString(e.what()));
        }
    }
}

void MainWindow::startOptimization() {
    if (currentGeometry.parts.empty()) {
        QMessageBox::warning(this, tr("Ошибка раскроя"), tr("Сначала загрузите DXF файл."));
        return;
    }

    // Блокируем UI
    parameters->setDisabled(true);
    statusLabel->setText(tr("Выполняется оптимизация..."));
    progressBar->setVisible(true);

    NestingParameters params = parameters->getNestingParameters();

    // Запускаем расчет в рабочем потоке
    emit startOptimizationRequested(currentGeometry, params);
}

void MainWindow::onOptimizationFinished(const NestingSolution &solution) {
    // Разблокируем UI
    parameters->setDisabled(false);
    statusLabel->setText(tr("Оптимизация завершена. Утилизация: %1%").arg(solution.utilization, 0, 'f', 2));
    progressBar->setVisible(false);

    layoutViewer->setLayoutSolution(solution);
}

void MainWindow::onOptimizationError(const QString &message) {
    parameters->setDisabled(false);
    statusLabel->setText(tr("Ошибка оптимизации"));
    progressBar->setVisible(false);

    QMessageBox::critical(this, tr("Ошибка алгоритма"), tr("Произошла ошибка при выполнении раскроя: ") + message);
}

void MainWindow::onOptimizationProgress(const NestingSolution &solution) {
    // Обновляем статус и отображаем текущее решение
    statusLabel->setText(tr("Поколение: %1. Утилизация: %2%")
                             .arg(solution.generation)
                             .arg(solution.utilization, 0, 'f', 2));

    // ВАЖНО: Визуализация промежуточных результатов
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
                          "<p>Версия: 0.2 (Refactored)</p>"));
}
