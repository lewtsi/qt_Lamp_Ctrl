#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_BtnAddUart_clicked()
{
    bool ok = false;
    QString portname;

    portname = QInputDialog::getText(this, tr("添加串口"), tr("设备文件名"), QLineEdit::Normal, 0, &ok);
    if(ok && !portname.isEmpty()){
        ui->portNameComboBox->addItem(portname);
        ui->statusBar->showMessage(tr("添加串口成功"));
        //ui->portNameComboBox->currentText(portname);
    }
}

void MainWindow::on_BtnOpenUart_clicked()
{
    QString portName = ui->portNameComboBox->currentText();   //获取串口名
    myCom = new Win_QextSerialPort(portName, QextSerialBase::EventDriven);
    //这里QextSerialBase::QueryMode应该使用QextSerialBase::Polling

    if(myCom->open(QIODevice::ReadWrite)){
        QMessageBox::information(this, tr("打开成功"), tr("已成功打开串口 ") + portName, QMessageBox::Ok);
    }else{
        QMessageBox::critical(this, tr("打开失败"), tr("未能打开串口 ") + portName + tr("\n该串口设备不存在或已被占用"), QMessageBox::Ok);
        return;
    }

    myCom->setBaudRate(BAUD115200);
    myCom->setDataBits(DATA_8);
    myCom->setParity(PAR_NONE);
    myCom->setStopBits(STOP_1);
    myCom->setFlowControl(FLOW_OFF);
    myCom->setTimeout(500);
    connect(myCom, SIGNAL(readyRead()), this, SLOT(readMyCom()));

    //ui->actionOpen->setEnabled(false);
   // ui->actionAdd->setEnabled(false);

  //  ui->sendMsgLineEdit->setFocus();
    ui->BtnLampOn->setEnabled(true);
    ui->BtnLampOff->setEnabled(true);
    ui->BtnPWMoutEn->setEnabled(true);
    ui->BtnPWMin->setEnabled(true);
    ui->BtnNetTop->setEnabled(true);
    ui->BtnCloseUart->setEnabled(true);
    ui->BtnOpenUart->setEnabled(false);
    ui->BtnTest->setEnabled(true);
    ui->BtnOpenUart->setText(portName + tr("已打开"));

    QPalette pal = ui->BtnOpenUart->palette();
    pal.setColor(QPalette::ButtonText,QColor(255,0,0));
    ui->BtnOpenUart->setPalette(pal);
}

void MainWindow::on_BtnCloseUart_clicked()
{
    myCom->close();
    ui->BtnOpenUart->setText(tr("打开串口"));
    ui->BtnOpenUart->setEnabled(true);
    ui->BtnCloseUart->setEnabled(false);
    ui->BtnTest->setEnabled(false);

    ui->BtnLampOn->setEnabled(false);
    ui->BtnLampOff->setEnabled(false);
    ui->BtnPWMoutEn->setEnabled(false);
    ui->BtnPWMin->setEnabled(false);
    ui->BtnNetTop->setEnabled(false);

    QPalette pal = ui->BtnOpenUart->palette();
    pal.setColor(QPalette::ButtonText,QColor(0,0,0));
    ui->BtnOpenUart->setPalette(pal);
}

static const char fx_msg_lamp_on[] = {0xAA, 0x80, 0xFF, 0x11, 0x01, 0xA0, 0x01, 0xA0, 0x35,
                                          0x00, 0xBB, 0x00, 0xAC, 0xDF, 0x01, 0xEE, 0xDD, 0x0b};
static const char fx_msg_lamp_off[] = {0xAA, 0x80, 0xFF, 0x11, 0x01, 0xA0, 0x01, 0xA0, 0x35,
                                          0x00, 0xBB, 0x00, 0xAC, 0xDF, 0x00, 0xEE, 0xDD, 0x0a};
static char fx_msg_pwm_out_ctrl[] = {0xAA, 0x80, 0xFF, 0x11, 0x01, 0xA0, 0x01, 0xA0, 0x15,
                                           0x00, 0xBB, 0x00, 0xAC, 0xAC, 0x32, 0xEE, 0xDD, 0x6B};
static const char fx_msg_read_nettop[] = {0xAA, 0x80, 0xA0, 0xEE, 0xDD, 0xB9};
static const char fx_msg_read_pwmin[] = {0xAA, 0x80, 0xA1, 0xEE, 0xDD, 0xB8};

int MainWindow::check_uart_is_open()
{
    if (!myCom->isOpen()){
        //QMessageBox::information(this, tr("串口尚未成功打开"), QMessageBox::Ok);
        QMessageBox::critical(this, "错误",
                QString("无法打开串口：%1\n指定的串口不存在或者被占用。").arg(myCom->portName()));
        ui->statusBar->showMessage(tr("串口尚未成功打开"));
        return -1;
    }
    return 0;
}

void MainWindow::on_BtnLampOn_clicked()
{
    if(check_uart_is_open()) return;

    // myCom->write(ui->lineEdit->text().toAscii() + "\n");
    myCom->write(fx_msg_lamp_on, 18);
    ui->statusBar->showMessage(tr("发送开灯信号成功"));
    qDebug() << "write: "<<myCom->bytesToWrite()<<"bytes";
}

void MainWindow::on_BtnLampOff_clicked()
{
    if(check_uart_is_open()) return;

    myCom->write(fx_msg_lamp_off, 18);
    ui->statusBar->showMessage(tr("发送关灯信号成功"));
    qDebug() << "write: "<<myCom->bytesToWrite()<<"bytes";
}

void MainWindow::on_BtnPWMoutEn_clicked()
{
    if(check_uart_is_open()) return;

    //发送数据
    fx_msg_pwm_out_ctrl[14] = (char) (ui->PWMout_value->value());
    fx_msg_pwm_out_ctrl[17] = 0x59 ^ fx_msg_pwm_out_ctrl[14];
    myCom->write(fx_msg_pwm_out_ctrl, 18);
    ui->statusBar->showMessage(tr("发送调光信号成功"));
    qDebug() << "write: "<<myCom->bytesToWrite()<<"bytes";
}


static char tmp_buf[20];
static const char hex2ascii[] = "0123456789ABCDEF";
void MainWindow::readMyCom()
{
    unsigned short addr, pwm, freq, cnt;
    double lltmp;
	
    qDebug() << "read:" << myCom->bytesAvailable() << "bytes";
    QByteArray temp = myCom->readAll();
    ui->textBrowser->append(tr("接收 < ")+temp.toHex() + tr(" >"));
    if((temp[0] == 0xAA) && (temp[1] == 0x80)){
        if(temp[2] == 0xA1){	// PWM INPUT
            ui->statusBar->showMessage(tr("读取PWM信息成功"));
            ui->textBrowser->clear();
            ui->textBrowser->append(tr("接收 < ")+temp.toHex() + tr(" >"));
            cnt = 3;
            while(cnt < 1000){
                if(temp[cnt] == 0xEE) break;

                addr = ((unsigned short)temp[cnt] << 8) + temp[cnt + 1];
                pwm = (unsigned short)temp[cnt + 2];
                freq = temp[cnt + 4];
                freq *= 256;
                freq += (unsigned char)temp[cnt + 3];
                //ui->textBrowser->append(addr);
                lltmp = addr;
                ui->NodeAddr->setNum(lltmp);
                lltmp = pwm;
                ui->PWM_dutycycle->setNum(lltmp);
                if(freq > 10000)    // 65535 - 10k - 1.28Mhz
                    freq = 0;
                lltmp = freq;
                ui->PWM_freq->setNum(lltmp);

                tmp_buf[0] = hex2ascii[(unsigned char)(temp[cnt] >> 4)&0x0F];
                tmp_buf[1] = hex2ascii[(unsigned char)temp[cnt]&0x0F];
                tmp_buf[2] = hex2ascii[(unsigned char)(temp[cnt + 1] >> 4)&0x0F];
                tmp_buf[3] = hex2ascii[(unsigned char)temp[cnt + 1]&0x0F];
                tmp_buf[4] = 0;
                ui->textBrowser->append(tr("--------------------------------------------"
                                           "--------------------------------------------"));
                ui->textBrowser->append(tr("节点地址：") + tmp_buf +
                                        tr("  输入电压占空比:") + ui->PWM_dutycycle->text() + tr("%") +
                                        tr("  功率输出频率值:") + ui->PWM_freq->text() + tr("Hz"));
                //ui->textBrowser->insertPlainText(rdtmp);
                cnt += 7;
            }
            if(cnt == 3) ui->textBrowser->append(tr("*** 没有PWM信息数据 ***"));
            else ui->textBrowser->append(tr("****************************"
                                            "*********************************************"));
        }
	}
}


static char PWM_IN[] = {0xAA, 0x80, 0xA1,
                        0xAE, 0x03, 0x32, 0xe8, 0x03, 0x00, 0x7C,
                        0xAE, 0x02, 0x0A, 0x84, 0x01, 0x00, 0x7C, 0xEE, 0xDD, 0xEF};
void MainWindow::on_BtnTest_clicked()
{
    if(check_uart_is_open()) return;
    myCom->write(PWM_IN, 20);
}

void MainWindow::on_BtnNetTop_clicked()
{
    if(check_uart_is_open()) return;
    myCom->write(fx_msg_read_nettop, 6);
    ui->statusBar->showMessage(tr("发送读取网络拓扑结构指令"));
    qDebug() << "write: "<<myCom->bytesToWrite()<<"bytes";
}

void MainWindow::on_BtnPWMin_clicked()
{
    if(check_uart_is_open()) return;
    myCom->write(fx_msg_read_pwmin, 6);
    ui->statusBar->showMessage(tr("发送读取PWM信息指令"));
    qDebug() << "write: "<<myCom->bytesToWrite()<<"bytes";
}

