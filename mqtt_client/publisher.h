#ifndef PUBLISHER_H
#define PUBLISHER_H

#include <QTimer>
#include <QHostAddress>
#include <iostream>
#include <mqtt/qmqtt.h>

class Publisher : public QMQTT::Client
{
    Q_OBJECT
public:
    Publisher(const QHostAddress& host = QHostAddress::LocalHost,//
              const quint16 port = 1883,        //mqtt服务器端口
              QString topic = "/",              //主题
              QString userName = "",         //用户名
              QString userPwd = "",          //用户名密码
              QObject* parent = NULL,           //
              bool encrypt = false)             //加密方式
        : QMQTT::Client(host, port, parent)
        , _number(0)
    {
        topic_ = topic;
        encrypt_ = encrypt;
        this->setUsername(userName);
        this->setPassword(userPwd.toUtf8());
        connect(this, &Publisher::connected, this, &Publisher::onConnected);
        connect(this, &Publisher::disconnected, this, &Publisher::onDisconnected);
    }
    virtual ~Publisher() {}

    void publishOne(QString msg){
        QString sendmsg = msg;

        if(encrypt_)
            sendmsg = encrypt(msg);

        QMQTT::Message message(_number, topic_,sendmsg.toUtf8());
        publish(message);
        _number++;
    }
    void publishTopic(QString topic,QString msg){
        QString sendmsg = msg;

        if(encrypt_)
            sendmsg = encrypt(msg);

        QMQTT::Message message(_number, topic,sendmsg.toUtf8());
        publish(message);
        _number++;
    }
    bool encrypt_;
    quint16 _number;
    QString topic_;

    QString encrypt(QString msg){

        // TODO

        return msg;
    }

public slots:
    void onConnected(){
        subscribe(topic_, 0);
    }

    void onDisconnected(){}
};


#endif // PUBLISHER_H
