#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QFont>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(timerUpdate()));
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
    //ui->BtnTest->setEnabled(true);
    ui->BtnOpenUart->setText(portName + tr("已打开"));
    ui->chkBoxRefresh->setEnabled(true);

    QPalette pal = ui->BtnOpenUart->palette();
    pal.setColor(QPalette::ButtonText,QColor(255,0,0));
    ui->BtnOpenUart->setPalette(pal);
    QFont serifFont("宋体", 9, QFont::Bold);
    ui->BtnOpenUart->setFont(serifFont);
}

void MainWindow::on_BtnCloseUart_clicked()
{

    QPalette pal = ui->BtnOpenUart->palette();
    pal.setColor(QPalette::ButtonText,QColor(0,0,0));
    ui->BtnOpenUart->setPalette(pal);

    //timer->stop();
    ui->chkBoxRefresh->setChecked(false);
    ui->chkBoxRefresh->setEnabled(false);
    on_chkBoxRefresh_clicked();

    myCom->close();
    ui->BtnOpenUart->setText(tr("打开串口"));
    ui->BtnOpenUart->setEnabled(true);
    ui->BtnCloseUart->setEnabled(false);
    //ui->BtnTest->setEnabled(false);

    ui->BtnLampOn->setEnabled(false);
    ui->BtnLampOff->setEnabled(false);
    ui->BtnPWMoutEn->setEnabled(false);
    ui->BtnPWMin->setEnabled(false);
    ui->BtnNetTop->setEnabled(false);
    QFont serifFont("微软雅黑", 12, QFont::Bold);
    ui->BtnOpenUart->setFont(serifFont);
}

static char fx_sig_lamp_on[] = {0xAA, 0x80, 0xFC, 0x0E, 0x01,
                                0xAE, 0x04, 0xBB, 0x00, 0xAC, 0xDF, 0x01, 0xEE, 0xDD, 0x89};    // len = 15
static char fx_sig_lamp_off[] = {0xAA, 0x80, 0xFC, 0x0E, 0x01,
                                 0xAE, 0x04, 0xBB, 0x00, 0xAC, 0xDF, 0x00, 0xEE, 0xDD, 0x88};
static char fx_sig_lamp_pwm_out[] = {0xAA, 0x80, 0xFC, 0x0E, 0x01,
                                     0xAE, 0x04, 0xBB, 0x00, 0xAC, 0xAC, 0x32, 0xEE, 0xDD, 0xD3};
static char fx_mul_lamp_on[] = {0xAA, 0x80, 0xFF, 0x11, 0x01, 0xA0, 0x01, 0xA0, 0x35,
                                          0x00, 0xBB, 0x00, 0xAC, 0xDF, 0x01, 0xEE, 0xDD, 0x0b}; // len = 18
static char fx_mul_lamp_off[] = {0xAA, 0x80, 0xFF, 0x11, 0x01, 0xA0, 0x01, 0xA0, 0x35,
                                          0x00, 0xBB, 0x00, 0xAC, 0xDF, 0x00, 0xEE, 0xDD, 0x0a};
static char fx_mul_lamp_pwm_out[] = {0xAA, 0x80, 0xFF, 0x11, 0x01, 0xA0, 0x01, 0xA0, 0x15,
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
        return 0;
    }
    return 1;
}

static short addrOP, addrED;
// opt-type 操作类型，开灯01， 关灯02，输出PWM 03
// char *指向发送指令地址
// int 发送指令长度
int MainWindow::get_LampCtrlStyle(unsigned char opt_type, char **cmd)
{
    //ui->NodeAddr->setText(ui->AddrOP->text());
    bool ok = true, flag = false;
    unsigned char i;
    char *buf;

    if(ui->AddrOP->text().isEmpty()) goto rtn_empty;
    addrOP = ui->AddrOP->text().toUShort(&ok, 16);
    if(ui->radioBtnMul->isChecked()){
        if(ui->AddrED->text().isEmpty()) goto rtn_empty;
        addrED = ui->AddrED->text().toUShort(&ok, 16);
        flag = true;
    }
    ui->NodeAddr->setNum(addrOP);
    if(opt_type == 0x01){
        if(flag) buf = fx_mul_lamp_on;
        else buf = fx_sig_lamp_on;
    }else if(opt_type == 0x02){
        if(flag) buf = fx_mul_lamp_off;
        else buf = fx_sig_lamp_off;
    }else{
        if(flag){
            fx_mul_lamp_pwm_out[14] = (char) (ui->PWMout_value->value());
            buf = fx_mul_lamp_pwm_out;
        }else{
            fx_sig_lamp_pwm_out[11] = (char) (ui->PWMout_value->value());
            buf = fx_sig_lamp_pwm_out;
        }
    }
    *cmd = buf;

    if(flag){
        buf[5] = (unsigned char)(addrOP >> 8);
        buf[6] = (unsigned char)addrOP;
        buf[7] = (unsigned char)(addrED >> 8);
        buf[8] = (unsigned char)addrED;
        if(ui->chkBox_odd->isChecked()){
            if(ui->chkBox_even->isChecked()){
                buf[9] = 0;
            }else{
                buf[9] |= 0x01;
            }
        }else{
            if(ui->chkBox_even->isChecked()){
                buf[9] = 0x02;
            }else{
                ui->chkBox_odd->setChecked(true);
                ui->chkBox_even->setChecked(true);
                buf[9] = 0;
            }
        }
        buf[17] = 0;
        for(i=0; i<17; i++)
            buf[17] ^= buf[i];
        return 18;
    }else{
        buf[5] = (unsigned char)(addrOP >> 8);
        buf[6] = (unsigned char)addrOP;
        buf[14] = 0;
        for(i=0; i<14; i++)
            buf[14] ^= buf[i];
        return 15;
    }

    rtn_empty:
        //QMessageBox::information(this, tr("地址栏填写不完整"), QMessageBox::Ok);
        ui->statusBar->showMessage(tr("地址信息填写不完整"));
        return 0;
}

static char *CtrlCmd;
void MainWindow::on_BtnLampOn_clicked()
{
    int len;

    if(check_uart_is_open() == 0) return;
    len = get_LampCtrlStyle(0x01, &CtrlCmd);

    if(len != 0){
        myCom->write(CtrlCmd, len);
        ui->statusBar->showMessage(tr("发送开灯信号成功"));
        qDebug() << "write: "<<myCom->bytesToWrite()<<"bytes";
    }
}

void MainWindow::on_BtnLampOff_clicked()
{
    int len;
    if(check_uart_is_open() == 0) return;
    len = get_LampCtrlStyle(0x02, &CtrlCmd);

    if(len != 0){
        myCom->write(CtrlCmd, len);
        ui->statusBar->showMessage(tr("发送关灯信号指令"));
        qDebug() << "write: "<<myCom->bytesToWrite()<<"bytes";
    }
}

void MainWindow::on_BtnPWMoutEn_clicked()
{
    int len;
    if(check_uart_is_open() == 0) return;
    len = get_LampCtrlStyle(0x03, &CtrlCmd);

    if(len != 0){
        myCom->write(CtrlCmd, len);
        ui->statusBar->showMessage(tr("发送调光信号指令"));
        qDebug() << "write: "<<myCom->bytesToWrite()<<"bytes";
    }
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
            ui->statusBar->showMessage(tr("发送读取路灯状态信息指令"));
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
            if(cnt == 3) ui->textBrowser->append(tr("*** 没有路灯状态信息数据 ***"));
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
    if(check_uart_is_open() == 0) return;
    myCom->write(PWM_IN, 20);
}

void MainWindow::on_BtnNetTop_clicked()
{
    if(check_uart_is_open() == 0) return;
    myCom->write(fx_msg_read_nettop, 6);
    ui->statusBar->showMessage(tr("发送读取网络拓扑结构指令"));
    qDebug() << "write: "<<myCom->bytesToWrite()<<"bytes";
}

void MainWindow::on_BtnPWMin_clicked()
{
    if(check_uart_is_open() == 0) return;
    myCom->write(fx_msg_read_pwmin, 6);
    ui->statusBar->showMessage(tr("发送读取PWM信息指令"));
    qDebug() << "write: "<<myCom->bytesToWrite()<<"bytes";
}


void MainWindow::on_radioBtnSig_clicked()
{
    ui->radioBtnSig->setChecked(true);
    ui->radioBtnMul->setChecked(false);
    ui->label_from->setEnabled(false);
    ui->label_to->setEnabled(false);
    ui->AddrED->setEnabled(false);
    ui->chkBox_even->setEnabled(false);
    ui->chkBox_odd->setEnabled(false);

}

void MainWindow::on_radioBtnMul_clicked()
{
    ui->radioBtnSig->setChecked(false);
    ui->label_from->setEnabled(true);
    ui->label_to->setEnabled(true);
    ui->AddrED->setEnabled(true);
    //ui->AddrOP->setText("AE01");
    //ui->AddrED->setText("AE15");
    ui->chkBox_even->setEnabled(true);
    ui->chkBox_odd->setEnabled(true);
}

static unsigned short lampPowerMaxValue = 150;
void MainWindow::on_lampPowerMax_textChanged(QString )
{
    bool ok = true;

    lampPowerMaxValue = ui->lampPowerMax->text().toShort(&ok, 10);
    ui->lampPowerValue->setValue(lampPowerMaxValue);
    //ui->lampPowerValue->setMaximum(lampPowerMaxValue);
    //ui->lampPowerValue->setMinimum(lampPowerMaxValue / 2);
    ui->NodeAddr->setNum(lampPowerMaxValue);
}

void MainWindow::on_PWMout_value_valueChanged(int value)
{
    unsigned short v, w;
    if(ui->radio_percent->isChecked() == false) return;
    w = ui->PWMout_value->value();
    v = lampPowerMaxValue * (w + 100) / 200;
    ui->lampPowerValue->setValue(v);
    ui->lampBar->setValue(w/2 + 50);
    ui->lampSlider->setValue(w);
    ui->label_powerPercent->setText(ui->lampBar->text());
}

void MainWindow::on_lampPowerValue_valueChanged(int )
{
    unsigned short v, w;
    if(ui->radio_power->isChecked() == false) return;

    w = ui->lampPowerValue->value();
    //v = lampPowerMaxValue * (w + 100) / 200;
    v = (w * 200) / lampPowerMaxValue - 100;  // (x - m/2) / (m/2)
    ui->PWMout_value->setValue(v);
    ui->lampSlider->setValue(v);
    ui->lampBar->setValue(w * 100 / lampPowerMaxValue);
    ui->label_powerPercent->setText(ui->lampBar->text());
}

void MainWindow::on_lampSlider_sliderMoved(int position)
{
    if(ui->radio_slider->isChecked() == false) return;

    unsigned short v, w;
    w = ui->lampSlider->value();
    v = lampPowerMaxValue * (w + 100) / 200;
    ui->lampPowerValue->setValue(v);
    ui->lampBar->setValue(w/2 + 50);
    ui->PWMout_value->setValue(w);
    ui->label_powerPercent->setText(ui->lampBar->text());
}

void MainWindow::on_radio_percent_clicked()
{
    ui->PWMout_value->setEnabled(true);
    ui->lampPowerValue->setEnabled(false);
}

void MainWindow::on_radio_power_clicked()
{
    ui->lampPowerValue->setEnabled(true);
    ui->PWMout_value->setEnabled(false);
}

static unsigned short fx_timerCounter = 0, fx_timerMax = 120, fx_timerOVcnt = 0;

void MainWindow::timerUpdate()
{
    if(ui->chkBoxRefresh->isChecked()){
        fx_timerCounter ++;
        if(fx_timerCounter >= fx_timerMax){
            fx_timerCounter = 0;
            if(ui->BtnPWMin->isEnabled())
                on_BtnPWMin_clicked();
            ui->statusBar->showMessage(tr("定时读取路灯状态信息"));
            fx_timerOVcnt ++;
            if(fx_timerOVcnt > 998) fx_timerOVcnt = 0;
            ui->label_timerOVcnt->setNum(fx_timerOVcnt);
        }
        ui->lcdTimer->display(fx_timerMax - fx_timerCounter);
        ui->timerBar->setValue(fx_timerCounter);
    }
}

void MainWindow::on_chkBoxRefresh_clicked()
{
    fx_timerMax = ui->timeCounter->value();
    if(ui->chkBoxRefresh->isChecked()){
        ui->label_refresh_info->setEnabled(true);
        ui->lcdTimer->setEnabled(true);
        ui->timeCounter->setEnabled(true);
        ui->timerBar->setMaximum(fx_timerMax);
        ui->timerBar->setValue(0);
        fx_timerCounter = 0;
        timer->start(1000);
    }else{
        timer->stop();
        ui->label_refresh_info->setEnabled(false);
        ui->lcdTimer->setEnabled(false);
        ui->timeCounter->setEnabled(false);
        ui->timerBar->setEnabled(false);
        fx_timerOVcnt = 0;
        ui->label_timerOVcnt->setNum(0);
    }
}

void MainWindow::on_timeCounter_valueChanged(int )
{
    fx_timerMax = ui->timeCounter->value();
    ui->lcdTimer->display(fx_timerMax);
    ui->timerBar->setMaximum(fx_timerMax);
    ui->timerBar->setValue(0);
}


