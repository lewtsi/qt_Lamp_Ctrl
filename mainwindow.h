#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtCore>
#include <QMessageBox>
#include <QFile>
#include <QInputDialog>
#include <QFileDialog>
#include <QTextStream>
#include <QtNetwork>
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
    QTimer *timer, *rfTimer;
    QTcpServer *tcpServer;
    QTcpSocket *tcpServerConnection;
    qint32 netRcvCounter;

private slots:
    void on_BtnClearText_clicked();
    void on_btnNetStart_clicked();
    //void tcpServerStart();   //开始监听
    void tcpServerAcceptConnection();  //建立连接
    void tcpServerDisplayError(QAbstractSocket::SocketError socketError);    // 显示错误
    void fx_getLocalIpAddr();
    void on_radioBtn_USB_clicked();
    void on_radioBtn_NET_clicked();
    void on_radioBtn_COM_clicked();
    void on_radio_slider_clicked();
    void on_textBrowserFontSlider_valueChanged(int value);
    void on_timeCounter_valueChanged(int );
    void on_chkBoxRefresh_clicked();
    void on_radio_power_clicked();
    void on_radio_percent_clicked();
    void on_lampSlider_valueChanged(int value);
    void on_lampPowerValue_valueChanged(int );
    void on_PWMout_value_valueChanged(int );
    void on_lampPowerMax_textChanged(QString );
    void on_radioBtnMul_clicked();
    void on_radioBtnSig_clicked();
    int check_uart_is_open();
    int get_LampCtrlStyle(unsigned char opt_type, char **cmd);
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
    void readNetMessage();
    void ReceiveDataHandle(QByteArray rcvBuf, unsigned short len);
    void timerUpdate();
    void rftimerUpdate();
};

#endif // MAINWINDOW_H
