#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class ParametersWidget;
class ViewerWidget;
class LayoutViewerWidget;
class QSplitter;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void showEvent(QShowEvent *event) override;

private:
    void setupMenu();

    ParametersWidget *parameters;
    ViewerWidget *viewer;
    LayoutViewerWidget *layoutViewer;

    QSplitter *horizontalSplitter;
    QSplitter *verticalSplitter;

    bool firstShow = true;

private slots:
    void loadFile();
    void saveFile();
    void saveAsFile();
    void showHelp();
};

#endif // MAINWINDOW_H
