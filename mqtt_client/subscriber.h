#ifndef SUBSCRIBER_H
#define SUBSCRIBER_H


#include <QHostAddress>
#include <QTextStream>
#include <mqtt/qmqtt.h>


class Subscriber : public QMQTT::Client
{
    Q_OBJECT
public:
    Subscriber(const QHostAddress& host = QHostAddress::LocalHost,
                        const quint16 port = 1883,  //mqtt服务器端口
                        QString topic = "#",        //主题
                        QString userName = "",      //mqtt服务器用户名
                        QString userPwd = "",
                        QObject* parent = NULL,     //
                        bool encrypt = false)       //加密方式
        : QMQTT::Client(host, port, parent)
        , _qout(stdout){
        topic_ = topic;
        encrypt_ = encrypt;

        //this->setClientId("IOT_SERVER");
        this->setUsername(userName);
        this->setPassword(userPwd.toUtf8());
        connect(this, &Subscriber::connected, this, &Subscriber::onConnected);
        connect(this, &Subscriber::subscribed, this, &Subscriber::onSubscribed);
        connect(this, &Subscriber::received, this, &Subscriber::onReceived);
    }

    virtual ~Subscriber() {}

    bool encrypt_;

    QTextStream _qout;
    QString topic_;

    QString encrypt(QString msg){

        // TODO

        return msg;
    }

public slots:
    void onConnected(){
        subscribe(topic_, 0);
    }

    void onSubscribed(const QString& topic){
        emit subscribedTo(topic);
    }

    void onReceived(const QMQTT::Message& message){
        QString msg = QString::fromUtf8(message.payload());
        QString topic = message.topic();
        if(encrypt_)
            msg = encrypt(msg);
        emit messageReceived(msg,topic);
    }

signals:
    void messageReceived(QString msg,QString topic);
    void subscribedTo(QString topic);
};

#endif // SUBSCRIBER_H
