#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include "../core/inputmanager.h"
#include "../core/layoutoptimizer.h" // Для Geometry type
#include "../core/optimizationworker.h"

class ParametersWidget;
class ViewerWidget;
class LayoutViewerWidget;
class QSplitter;
class QLabel;
class QProgressBar;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void loadFile();
    void saveFile();
    void saveAsFile();
    void showHelp();

    // Слот запуска (UI -> Worker)
    void startOptimization();

    // Слоты приема результатов (Worker -> UI)
    void onOptimizationFinished(const NestingSolution &solution);
    void onOptimizationError(const QString &message);

    // Слот отображения промежуточного результата
    void onOptimizationProgress(const NestingSolution &solution);

signals:
    // Сигнал для передачи данных в Worker (cross-thread)
    void startOptimizationRequested(const Geometry geometry, const NestingParameters params);

private:
    void setupMenu();
    void setupStatusBar();

    ParametersWidget *parameters;
    ViewerWidget *viewer;
    LayoutViewerWidget *layoutViewer;

    QSplitter *horizontalSplitter;
    QSplitter *verticalSplitter;

    // Элементы UI статуса
    QLabel *statusLabel;
    QProgressBar *progressBar;

    InputManager inputManager;
    Geometry currentGeometry;

    // Поточность
    QThread* workerThread = nullptr;
    OptimizationWorker* worker = nullptr;

    bool firstShow = true;
};

#endif // MAINWINDOW_H
