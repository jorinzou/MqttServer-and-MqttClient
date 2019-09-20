#ifndef MQTTCLIENT_H
#define MQTTCLIENT_H

#include <QObject>
#include <QTimer>
#include "publisher.h"
#include "subscriber.h"
#include <QPlainTextEdit>
#include <QString>

class mqttClient : public QObject
{
    Q_OBJECT
public:
    explicit mqttClient(QObject *parent = nullptr);
    void MqttClientInit(QString mqttHostName, int port, QString sub, QString pub, QString userName, QString userPwd);//初始化MQTT连接
    void PublishMessage(QString msg, QString topic);//mqtt发布信息
    static QString getStringFromJsonObject(const QJsonObject& jsonObject);
    static QJsonObject getJsonObjectFromString(const QString jsonString);
    QString GetPublishMessage();//获取推送消息
    QString GetSubscribeMessage();//获取订阅消息
    void setInsertTextColor(QColor col);
    QString GetPublishMessageTheme();

signals:

public slots:
    void SubscribeMessageReceived(const QMQTT::Message& message);
    void PublishMessageReceived(const QMQTT::Message& message);
    void TimeHandle();//定时时间到执行的槽函数

private:
   Subscriber *subscriber_;
   Publisher *publisher_;
   QTimer *Timer;
   int TimeCount;
   QString ReceivePublishMessage;//回复的推送消息
   QString ReceivePublishTheme;
   QString ReceiveSubscribeMessage;//回复的订阅消息
};

#endif // MQTTCLIENT_H
