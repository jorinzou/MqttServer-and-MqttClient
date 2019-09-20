#include "mainwindow.h"

void MainWindow::CommonWinInit(void)
{
    //发布消息至server
    PublishInfoTtoGw.setParent(&CommonWin);
    PublishInfoTtoGw.resize(470,550);
    PublishInfoTtoGw.move(680,110);
    PublishInfoTtoGw.setStyleSheet("background:rgb(162,223,172)");
    QJsonObject PublishInfoTtoGwJsonData;
    PublishInfoTtoGwJsonData.insert("Cluster_ID",0);
    PublishInfoTtoGwJsonData.insert("Command",6201);
    QString playload = getStringFromJsonObject(PublishInfoTtoGwJsonData);
    PublishInfoTtoGw.appendPlainText(playload);

    //主题
    PublishThemeTtoGw.setParent(&CommonWin);
    PublishThemeTtoGw.resize(470,50);
    PublishThemeTtoGw.move(680,20);
    PublishThemeTtoGw.setStyleSheet("background:rgb(162,223,172)");
    PublishThemeTtoGw.appendPlainText("输入主题");

    //发布消息按钮
    PublishInfoTtoGwButton.setParent(&CommonWin);
    PublishInfoTtoGwButton.move(680,80);
    PublishInfoTtoGwButton.resize(140,20);
    PublishInfoTtoGwButton.setText("发布消息");
    PublishInfoTtoGwButton.setStyleSheet("background:rgb(162,223,172)");

    //接收数据文本框大小
    GwResponInfoShow.setParent(&CommonWin);
    GwResponInfoShow.resize(495,640);
    GwResponInfoShow.move(10,20);
    GwResponInfoShow.setStyleSheet("background:rgb(162,223,172)");

    //文本框清除按钮
    TeClearButton.setParent(&CommonWin);
    TeClearButton.move(10,670);
    TeClearButton.resize(80,40);
    TeClearButton.setText("清除");
    TeClearButton.setStyleSheet("background:rgb(162,223,172)");

    //server端口
    GwMqttPortLineEdit.setParent(&CommonWin);
    GwMqttPortLineEdit.move(520,165);
    GwMqttPortLineEdit.resize(40,20);
    GwMqttPortLineEdit.setText("1883");

    //mqtt server addr
    MqttServerAddr.setParent(&CommonWin);
    MqttServerAddr.move(520,190);
    MqttServerAddr.resize(140,20);

    //连接
    ConnectGwButton.setParent(&CommonWin);
    ConnectGwButton.move(520,120);
    ConnectGwButton.resize(60,40);
    ConnectGwButton.setText("连接");
    ConnectGwButton.setStyleSheet("background:rgb(162,223,172)");

    //断开连接
    DisConnectGwButton.setParent(&CommonWin);
    DisConnectGwButton.move(600,120);
    DisConnectGwButton.resize(60,40);
    DisConnectGwButton.setText("断开");
    DisConnectGwButton.setStyleSheet("background:rgb(162,223,172)");

    //连接标志
    IsConnectGw = 0;
    //广播点击标志
    ISClickBroadcastButton = 0;

    publisher_ = NULL;

    connect(&TeClearButton,SIGNAL(clicked(bool)),this,SLOT(TeClearButtonClick()));
    connect(&ConnectGwButton,SIGNAL(clicked(bool)),this,SLOT(ConnectGwButtonClick()));
    connect(&DisConnectGwButton,SIGNAL(clicked(bool)),this,SLOT(DisConnectGwButtonClick()));
    connect(&PublishInfoTtoGwButton,SIGNAL(clicked(bool)),this,SLOT(PublishInfoTtoGwButtonClick()));
}

//文本框清除按钮
void MainWindow::TeClearButtonClick(void)
{
    GwResponInfoShow.clear();
}

//自定义消息发布
void MainWindow::PublishInfoTtoGwButtonClick(void)
{
    QString Theme = PublishThemeTtoGw.toPlainText();
    QString PlayLoad = PublishInfoTtoGw.toPlainText();
    this->PublishMessageToGw(PlayLoad,Theme);
}


//开始连接
void MainWindow::ConnectGwButtonClick(void)
{
    QString mqttHostName = MqttServerAddr.text().trimmed();
    int port = GwMqttPortLineEdit.text().toInt();
    QHostAddress RemoteHost(mqttHostName);//ip

    if (IsConnectGw == 0) {
        publisher_ = new Publisher(RemoteHost,port,"connnect","gw","123456");//发布消息
        publisher_->connectToHost();
        connect(publisher_,SIGNAL(received(const QMQTT::Message)),this,SLOT(PublishMessageReceived(const QMQTT::Message)));
        connect(publisher_,SIGNAL(connected()),this,SLOT(ConnectAckReponse()));
        connect(publisher_,SIGNAL(disconnected()),this,SLOT(GwDisconnect()));
    }
}

void MainWindow::GwDisconnect(void)
{
    if (publisher_) {
        delete publisher_;
        IsConnectGw = 0;
        publisher_ = 0;
        ConnectGwButton.setStyleSheet("background:rgb(255,0,0)");
    }
}

//连接gw响应
void MainWindow::ConnectAckReponse(void)
{
    qDebug() << "ConnectAckReponse";
    IsConnectGw = 1;
    ConnectGwButton.setStyleSheet("background:rgb(0,255,0)");
}

//接收数据
void MainWindow::PublishMessageReceived(const QMQTT::Message &message)
{
    ReceivePublishMessage = message.payload();
    ReceivePublishTheme = message.topic();
    ShowPublishReceiveMessage();
}

//断开连接
void MainWindow::DisConnectGwButtonClick(void)
{
    if (IsConnectGw == 1)
    {
        publisher_->cleanSession();
        delete publisher_;
        publisher_ = NULL;
        IsConnectGw = 0;
        qDebug() << IsConnectGw;
        ConnectGwButton.setStyleSheet("background:rgb(255,0,0)");
    }
}

//json对象格式化成字符串输出
QString  MainWindow::getStringFromJsonObject(const QJsonObject& jsonObject)
{
    return QString(QJsonDocument(jsonObject).toJson());
}

//字符串转化成json对象
QJsonObject MainWindow::getJsonObjectFromString(const QString jsonString)
{
    QJsonDocument jsonDocument = QJsonDocument::fromJson(jsonString.toLocal8Bit().data());
    if( jsonDocument.isNull() ){
        qDebug()<< "===> please check the string "<< jsonString.toLocal8Bit().data();
    }
    QJsonObject jsonObject = jsonDocument.object();
    return jsonObject;
}

//设置字显示颜色
void MainWindow::setInsertTextColor(QColor col)
{
     QTextCharFormat fmt;
     fmt.setForeground(col);
     QTextCursor cursor = QTextCursor();
     cursor.mergeCharFormat(fmt);
     GwResponInfoShow.mergeCurrentCharFormat(fmt);
}

//显示接收推送数据至文本框
void MainWindow::ShowPublishReceiveMessage(void)
{
    if (IsConnectGw == 0)
        return;

    GwResponInfoShow.appendPlainText("");//完美的分隔线
    GwResponInfoShow.appendPlainText("-----------------------------------------------------");
    GwResponInfoShow.appendPlainText(ReceivePublishTheme);
    GwResponInfoShow.appendPlainText(ReceivePublishMessage);
    QColor color = QColor();
    color.setNamedColor("red");//显示字符颜色
    this->setInsertTextColor(color);
}

//推送消息至server
void MainWindow::PublishMessageToGw(QString msg, QString topic)
{
    if (publisher_) {
        publisher_->publishTopic(topic,msg);
    }
}
