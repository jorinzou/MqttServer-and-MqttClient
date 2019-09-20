#include "mqttclient.h"
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QByteArray>

mqttClient::mqttClient(QObject *parent) : QObject(parent)
{
    subscriber_ = NULL;
    publisher_ = NULL;
    TimeCount = 0;
}

//参数依次为 mqtt服务器IP---mqtt服务器端口---订阅主题---发布主题---用户名---用户名对应的密码
void mqttClient::MqttClientInit(QString mqttHostName, int port, QString sub, QString pub, QString userName, QString userPwd)
{
    QHostAddress RemoteHost(mqttHostName);//ip
    //subscriber_ = new Subscriber(RemoteHost,port,sub,userName,userPwd);//订阅消息
    publisher_ = new Publisher(RemoteHost,port,pub,userName,userPwd);//发布消息
    //subscriber_->connectToHost();
    publisher_->connectToHost();

    //connect(subscriber_,SIGNAL(messageReceived(QString,QString)),this,SLOT(SubscribeMessageReceived(QString,QString)));
    connect(publisher_,SIGNAL(received(const QMQTT::Message)),this,SLOT(PublishMessageReceived(const QMQTT::Message)));

    Timer= new QTimer(this);
    connect(Timer,SIGNAL(timeout()),this,SLOT(TimeHandle()));//定时发布
    Timer->start(1000);//1s
}

//json对象格式化成字符串输出
QString  mqttClient::getStringFromJsonObject(const QJsonObject& jsonObject)
{
    return QString(QJsonDocument(jsonObject).toJson());
}

//字符串转化成json对象
QJsonObject  mqttClient::getJsonObjectFromString(const QString jsonString)
{
    QJsonDocument jsonDocument = QJsonDocument::fromJson(jsonString.toLocal8Bit().data());
    if( jsonDocument.isNull() ){
        qDebug()<< "===> please check the string "<< jsonString.toLocal8Bit().data();
    }
    QJsonObject jsonObject = jsonDocument.object();
    return jsonObject;
}

//定时发布测试
void mqttClient::TimeHandle()
{
    QJsonObject testjson;
    testjson.insert("Cluster_ID",0);
    testjson.insert("Command",95);//获取场景列表信息
    QString playload = getStringFromJsonObject(testjson);
    QString theme = "any";
    this->PublishMessage(playload,theme);
}

//推送接收数据
void mqttClient::PublishMessageReceived(const QMQTT::Message& message)
{
    ReceivePublishMessage = message.payload();
    ReceivePublishTheme = message.topic();
}

//订阅接收数据
void mqttClient::SubscribeMessageReceived(const QMQTT::Message& message)
{
    ReceivePublishMessage = message.payload();
}

QString mqttClient::GetPublishMessage()
{
    return ReceivePublishMessage;
}

QString mqttClient::GetPublishMessageTheme()
{
    return ReceivePublishTheme;
}

QString mqttClient::GetSubscribeMessage()
{
    return ReceiveSubscribeMessage;
}

//mqtt发布信息
void mqttClient::PublishMessage(QString msg, QString topic)
{
    publisher_->publishTopic(topic,msg);
}
