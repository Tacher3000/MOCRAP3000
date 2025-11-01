#ifndef PARAMETERSWIDGET_H
#define PARAMETERSWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include "../core/layoutstructures.h"

class ParametersWidget : public QWidget {
    Q_OBJECT

public:
    ParametersWidget(QWidget *parent = nullptr);
    NestingParameters getNestingParameters() const;

signals:
    void optimizeRequested();

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
