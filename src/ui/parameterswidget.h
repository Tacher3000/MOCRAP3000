#ifndef PARAMETERSWIDGET_H
#define PARAMETERSWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

class ParametersWidget : public QWidget {
    Q_OBJECT

public:
    ParametersWidget(QWidget *parent = nullptr);

private:
    QLabel *sheetSizeLabel;
    QLineEdit *sheetWidth;
    QLineEdit *sheetHeight;

    QLabel *partSpacingLabel;
    QLineEdit *partSpacing;

    QLabel *cutThicknessLabel;
    QLineEdit *cutThickness;

    QLabel *partCountLabel;
    QLineEdit *partCount;

    QPushButton *optimizeButton;

    QVBoxLayout *layout;
};

#endif // PARAMETERSWIDGET_H
