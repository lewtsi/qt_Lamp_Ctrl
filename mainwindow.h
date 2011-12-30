#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtCore>
#include <QMessageBox>
#include <QFile>
#include <QInputDialog>
#include <QFileDialog>
#include <QTextStream>
#include "win_qextserialport.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    Win_QextSerialPort *myCom;

private slots:
    int check_uart_is_open();
    void on_BtnCloseUart_clicked();
    void on_BtnPWMin_clicked();
    void on_BtnNetTop_clicked();
    void on_BtnTest_clicked();
    void on_BtnPWMoutEn_clicked();
    void on_BtnLampOff_clicked();
    void on_BtnLampOn_clicked();
    void on_BtnOpenUart_clicked();
    void on_BtnAddUart_clicked();
    void readMyCom();
};

#endif // MAINWINDOW_H
