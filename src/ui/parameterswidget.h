#ifndef PARAMETERSWIDGET_H
#define PARAMETERSWIDGET_H

#include <QDialog>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include "../core/layoutstructures.h"

class ParametersWidget : public QDialog {
    Q_OBJECT

public:
    ParametersWidget(QWidget *parent = nullptr);
    NestingParameters getNestingParameters() const;

protected:
    void accept() override;
private:
    QLineEdit *sheetWidth;
    QLineEdit *sheetHeight;
    QLineEdit *partSpacing;
    QLineEdit *cutThickness;

    QVBoxLayout *layout;
};

#endif // PARAMETERSWIDGET_H
