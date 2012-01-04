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

static QHostAddress netAddress;
void MainWindow::fx_getLocalIpAddr()
{
    QList<QHostAddress> list = QNetworkInterface::allAddresses();
    foreach (QHostAddress address, list){
    if(address.protocol() == QAbstractSocket::IPv4Protocol)//����ʹ��IPv4��ַ
        netAddress = address;
    }
}

void MainWindow::on_radioBtn_COM_clicked()
{
    ui->groupBox_COM->setEnabled(true);
    ui->groupBox_NET->setEnabled(false);
    ui->groupBox_USB->setEnabled(false);
}

void MainWindow::on_radioBtn_NET_clicked()
{
    ui->groupBox_NET->setEnabled(true);
    ui->groupBox_COM->setEnabled(false);
    ui->groupBox_USB->setEnabled(false);
    /*QString localHostName = QHostInfo::localHostName();
    qDebug() <<"localHostName: "<<localHostName;
    QHostInfo info = QHostInfo::fromName(localHostName);
    qDebug() <<"IP Address: "<<info.addresses();
    foreach(QHostAddress address,info.addresses()){
        if(address.protocol() == QAbstractSocket::IPv4Protocol)
            qDebug() << address.toString();
    }*/
    fx_getLocalIpAddr();
    ui->netIpAddr->setText(netAddress.toString());
}

void MainWindow::on_radioBtn_USB_clicked()
{
    ui->groupBox_USB->setEnabled(true);
    ui->groupBox_COM->setEnabled(false);
    ui->groupBox_NET->setEnabled(false);
}


void MainWindow::on_BtnAddUart_clicked()
{
    bool ok = false;
    QString portname;

    portname = QInputDialog::getText(this, tr("��Ӵ���"), tr("�豸�ļ���"), QLineEdit::Normal, 0, &ok);
    if(ok && !portname.isEmpty()){
        ui->portNameComboBox->addItem(portname);
        ui->statusBar->showMessage(tr("��Ӵ��ڳɹ�����ѡ��˿ںţ����򿪴���"));
        //ui->portNameComboBox->currentText(portname);
    }
}

void MainWindow::on_BtnOpenUart_clicked()
{
    QString portName = ui->portNameComboBox->currentText();   //��ȡ������
    myCom = new Win_QextSerialPort(portName, QextSerialBase::EventDriven);
    //����QextSerialBase::QueryModeӦ��ʹ��QextSerialBase::Polling

    if(myCom->open(QIODevice::ReadWrite)){
        QMessageBox::information(this, tr("�򿪳ɹ�"), tr("�ѳɹ��򿪴��� ") + portName, QMessageBox::Ok);
    }else{
        QMessageBox::critical(this, tr("��ʧ��"), tr("δ�ܴ򿪴��� ") + portName + tr("\n�ô����豸�����ڻ��ѱ�ռ��"), QMessageBox::Ok);
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
    ui->BtnOpenUart->setText(portName + tr("�Ѵ�"));
    ui->chkBoxRefresh->setEnabled(true);

    QPalette pal = ui->BtnOpenUart->palette();
    pal.setColor(QPalette::ButtonText,QColor(255,0,0));
    ui->BtnOpenUart->setPalette(pal);
    QFont serifFont("����", 9, QFont::Bold);
    ui->BtnOpenUart->setFont(serifFont);
    ui->statusBar->showMessage(tr("�ɹ��򿪴��� ") + portName);
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
    ui->BtnOpenUart->setText(tr("�򿪴���"));
    ui->BtnOpenUart->setEnabled(true);
    ui->BtnCloseUart->setEnabled(false);
    //ui->BtnTest->setEnabled(false);

    ui->BtnLampOn->setEnabled(false);
    ui->BtnLampOff->setEnabled(false);
    ui->BtnPWMoutEn->setEnabled(false);
    ui->BtnPWMin->setEnabled(false);
    ui->BtnNetTop->setEnabled(false);
    QFont serifFont("΢���ź�", 12, QFont::Bold);
    ui->BtnOpenUart->setFont(serifFont);
}

// =======================  ���緽ʽ =================
void MainWindow::on_btnNetStart_clicked()
{
    tcpServer = new QTcpServer(this);
    connect(tcpServer, SIGNAL(newConnection()), this,
                        SLOT(tcpServerAcceptConnection()));
    netRcvCounter = 0;
    if(! tcpServer->listen(netAddress,ui->netPortNumb->text().toShort())){
        qDebug() << tcpServer->errorString();
        return;
    }
    ui->netLabelStatus->setText(tr("״̬������"));

    ui->btnNetStart->setText(tr("�Ͽ�����"));
    QPalette pal = ui->btnNetStart->palette();
    pal.setColor(QPalette::ButtonText,QColor(255,0,0));
    ui->btnNetStart->setPalette(pal);
}

void MainWindow::tcpServerAcceptConnection()
{
    tcpServerConnection = tcpServer->nextPendingConnection();
    connect(tcpServerConnection, SIGNAL(readyRead()), this,
            SLOT(readNetMessage()));
    connect(tcpServerConnection,
            SIGNAL(displayError(QAbstractSocket::SocketError)), this,
            SLOT(tcpServerDisplayError(QAbstractSocket::SocketError)));
    ui->netLabelStatus->setText(tr("��������"));
    tcpServer->close();
}

void MainWindow::tcpServerDisplayError(QAbstractSocket::SocketError)
{
    qDebug() << tcpServerConnection->errorString();
    tcpServerConnection->close();
    ui->netLabelStatus->setText(tr("����˾���"));
    ui->btnNetStart->setText(tr("����TCP������"));
}

void MainWindow::readNetMessage()
{
    qDebug() << "Net read:" << tcpServerConnection->bytesAvailable() << "bytes.";
    QByteArray temp = tcpServerConnection->readAll();
    ui->textBrowser->append(tr("TCP <") + temp.toHex().toUpper() + tr(" >"));
    //qDebug() << "read:" << tcpSocket->bytesAvailable() << "bytes";
    //QByteArray temp = myCom->readAll();
   // ui->textBrowser->append(tr("���� < ")+temp.toHex().toUpper() + tr(" >"));
}

// =================================================
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

static unsigned short fx_timerCounter = 0, fx_timerMax = 120, fx_timerOVcnt = 0;   // ��ʱˢ�����

int MainWindow::check_uart_is_open()
{
    if (!myCom->isOpen()){
        //QMessageBox::information(this, tr("������δ�ɹ���"), QMessageBox::Ok);
        QMessageBox::critical(this, "����",
                QString("�޷��򿪴��ڣ�%1\nָ���Ĵ��ڲ����ڻ��߱�ռ�á�").arg(myCom->portName()));
        ui->statusBar->showMessage(tr("������δ�ɹ���"));
        return 0;
    }
    return 1;
}

static short addrOP, addrED;
// opt-type �������ͣ�����01�� �ص�02�����PWM 03
// char *ָ����ָ���ַ
// int ����ָ���
int MainWindow::get_LampCtrlStyle(unsigned char opt_type, char **cmd)
{
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
    if(opt_type == 0x01){
        if(flag) buf = fx_mul_lamp_on;
        else buf = fx_sig_lamp_on;
    }else if(opt_type == 0x02){
        if(flag) buf = fx_mul_lamp_off;
        else buf = fx_sig_lamp_off;
    }else{
        if(flag){
            fx_mul_lamp_pwm_out[14] = (char) (100 - ui->PWMout_value->value());
            buf = fx_mul_lamp_pwm_out;
        }else{
            fx_sig_lamp_pwm_out[11] = (char) (100 - ui->PWMout_value->value());
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
        //QMessageBox::information(this, tr("��ַ����д������"), QMessageBox::Ok);
        ui->statusBar->showMessage(tr("��ַ��Ϣ��д������"));
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
        ui->statusBar->showMessage(tr("���Ϳ����ź�ָ��"));
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
        ui->statusBar->showMessage(tr("���͹ص��ź�ָ��"));
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
        ui->statusBar->showMessage(tr("���͵����ź�ָ��"));
        qDebug() << "write: "<<myCom->bytesToWrite()<<"bytes";
    }
}

static const char hex2ascii[] = "0123456789ABCDEF";

void MainWindow::ReceiveDataHandle(QByteArray rcvBuf, unsigned short len)
{
    quint16 addr, pwm, freq, cnt;
    //char *buf = rcvBuf.data();

    //if(*buf != 0x80)
    //ui->usbVIDvalue->setText("error");
    //else
    //ui->statusBar->showMessage(tr("��ʼ������������"));
    if((0xAA == (quint8)rcvBuf[0]) && (0x80 == (quint8)rcvBuf[1])){
        if(0xA1 == (quint8)rcvBuf[2]){	// PWM INPUT
            ui->statusBar->showMessage(tr("����·��״̬��Ϣָ��"));
            ui->textBrowser->clear();
            ui->textBrowser->append(tr("���� < ")+rcvBuf.toHex().toUpper() + tr(" >"));
            cnt = 3;
            while(cnt < 1000){
                if(0xEE == (quint8)rcvBuf[cnt]) break;

                addr = ((quint16)rcvBuf[cnt] << 8) + rcvBuf[cnt + 1];
                pwm = (quint16)rcvBuf[cnt + 2];
                freq = rcvBuf[cnt + 4];
                freq *= 256;
                freq += (quint8)rcvBuf[cnt + 3];
                if(freq > 10000)    // 65535 - 10k - 1.28Mhz
                    freq = 0;

                QString strAddr, strDuty, strFreq;
                strAddr.setNum(addr, 16);
                strDuty.setNum(pwm, 10);
                strFreq.setNum(freq, 10);
                ui->textBrowser->append(tr("--------------------------------------------"
                                           "--------------------------------------------"));
                ui->textBrowser->append(tr("�ڵ��ַ��") + strAddr.toUpper() +
                                        tr("  �����ѹռ�ձ�:") + strDuty +
                                        tr("%  �������Ƶ��ֵ:") + strFreq + tr("Hz"));
                if(freq == 0)
                    ui->textBrowser->append(tr("�ýڵ��������Ƶ��ֵ ���� ������"));

                cnt += 7;
            }
            if(cnt == 3) ui->textBrowser->append(tr("*** û��·��״̬��Ϣ���� ***"));
            else ui->textBrowser->append(tr("****************************"
                                            "*********************************************"));
        }else if(0xA3 == (quint8)rcvBuf[2]){    // �ڵ�������Ϣ�ϱ�
            QString strAddr;
            cnt = 0;    // AA 80 A3 01 AA CC 02 AE 01 AE 02 EE DD XOR
            while(cnt < 15){
                if(cnt >= rcvBuf[3]) break;
                addr = ((quint16)rcvBuf[7 + cnt * 2] << 8) + rcvBuf[8 + cnt * 2];
                strAddr.setNum(addr, 16);
                ui->textBrowser->append(strAddr.toUpper() + tr(" �ڵ�����"));
                cnt ++;
            }
        }
    }
}

void MainWindow::readMyCom()
{
    qDebug() << "read:" << myCom->bytesAvailable() << "bytes";
    QByteArray temp = myCom->readAll();
    ui->textBrowser->append(tr("���� < ")+temp.toHex().toUpper() + tr(" >"));
    ReceiveDataHandle(temp, myCom->bytesAvailable());
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
    ui->statusBar->showMessage(tr("���Ͷ�ȡ�������˽ṹָ��"));
    qDebug() << "write: "<<myCom->bytesToWrite()<<"bytes";
}

void MainWindow::on_BtnPWMin_clicked()
{
    if(check_uart_is_open() == 0) return;
    myCom->write(fx_msg_read_pwmin, 6);
    ui->statusBar->showMessage(tr("���Ͷ�ȡ·��״̬��Ϣָ��"));
    fx_timerCounter = 0;
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
    ui->lampPowerValue->setMaximum(lampPowerMaxValue);
    ui->lampPowerValue->setMinimum(lampPowerMaxValue / 2);
    ui->lampPowerValue->setValue(lampPowerMaxValue * (ui->PWMout_value->value()/2 + 50) / 100);
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
    //ui->label_powerPercent->setText(ui->lampBar->text());
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
    //ui->label_powerPercent->setText(ui->lampBar->text());
}

void MainWindow::on_lampSlider_valueChanged(int value)
{
    if(ui->radio_slider->isChecked() == false) return;

    unsigned short v;
    v = lampPowerMaxValue * (value + 100) / 200;
    ui->lampPowerValue->setValue(v);
    ui->lampBar->setValue(value/2 + 50);
    ui->PWMout_value->setValue(value);
    //ui->label_powerPercent->setText(ui->lampBar->text());
}

void MainWindow::on_radio_slider_clicked()
{
    ui->lampSlider->setEnabled(true);
    ui->lampPowerValue->setEnabled(false);
    ui->PWMout_value->setEnabled(false);
    ui->lampSlider->setBackgroundRole(QPalette::Background);
    ui->lampSlider->setStyleSheet("background-color: rgb(255, 170, 127)");
}

void MainWindow::on_radio_percent_clicked()
{
    ui->PWMout_value->setEnabled(true);
    ui->lampPowerValue->setEnabled(false);
    ui->lampSlider->setEnabled(false);
    ui->lampSlider->setStyleSheet("background-color: rgb(255, 255, 127)");
}

void MainWindow::on_radio_power_clicked()
{
    ui->lampPowerValue->setEnabled(true);
    ui->PWMout_value->setEnabled(false);
    ui->lampSlider->setEnabled(false);
    ui->lampSlider->setStyleSheet("background-color: rgb(255, 255, 127)");
}

void MainWindow::timerUpdate()
{
    if(ui->chkBoxRefresh->isChecked()){
        fx_timerCounter ++;
        if(fx_timerCounter >= fx_timerMax){
            fx_timerCounter = 0;
            if(ui->BtnPWMin->isEnabled())
                on_BtnPWMin_clicked();
            fx_timerOVcnt ++;
            if(fx_timerOVcnt > 998) fx_timerOVcnt = 0;
            ui->label_timerOVcnt->setNum(fx_timerOVcnt);
            ui->statusBar->showMessage(tr("��ʱ��ȡ·��״̬��Ϣ"));
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
        //ui->lcdTimer->setEnabled(true);
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

void MainWindow::on_textBrowserFontSlider_valueChanged(int value)
{
    QString str;
    QFont serifFont("Arial", value, QFont::Bold);
    ui->textBrowser->setFont(serifFont);
    str.setNum(value);
    //ui->textBrowser->fontInfo();
    ui->textBrowserFontLabel->setText(ui->textBrowser->font().toString()); //str + tr("������"));
}
