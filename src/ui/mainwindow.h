#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include "../core/inputmanager.h"
#include "../core/layoutoptimizer.h"
#include "../core/optimizationworker.h"

class ParametersWidget;
class ViewerWidget;
class LayoutViewerWidget;
class PartListWidget;
class QSplitter;
class QLabel;
class QProgressBar;
class QPushButton;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void loadFile();
    void clearAll();
    void saveFile();
    void saveAsFile();
    void showHelp();
    void showSettings();

    void onPartSelected(const Part& part);
    void onShowAllPartsRequested();

    void startOptimization();
    void onOptimizationFinished(const NestingSolution &solution);
    void onOptimizationError(const QString &message);
    void onOptimizationProgress(const NestingSolution &solution);

signals:
    void startOptimizationRequested(const Geometry geometry, const NestingParameters params);

private:
    void setupMenu();
    void setupStatusBar();

    ParametersWidget *parametersDialog;
    PartListWidget *partList;
    QPushButton *btnStartOptimization;

    ViewerWidget *viewer;
    LayoutViewerWidget *layoutViewer;

    QSplitter *mainSplitter;
    QSplitter *viewersSplitter;

    QLabel *statusLabel;
    QProgressBar *progressBar;

    InputManager inputManager;
    Geometry currentGeometry;

    QThread* workerThread = nullptr;
    OptimizationWorker* worker = nullptr;

    bool firstShow = true;
};

#endif // MAINWINDOW_H
