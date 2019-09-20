#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "mqttclient.h"
#include <QPlainTextEdit>
#include <QTimer>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QColor>
#include <QPushButton>
#include <QUdpSocket>
#include <QComboBox>
#include <QTextEdit>
#include <QLineEdit>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTabWidget>
#include <QTextCursor>
#include <QHostAddress>
#include <QByteArray>
#include <QNetworkInterface>
#include <QList>
#include "publisher.h"
#include <QNetworkAccessManager>
#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QAuthenticator>
#include <QTime>

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void setInsertTextColor(QColor col);
    void MainWinIinit(void);
    void CommonWinInit(void);
    static QString getStringFromJsonObject(const QJsonObject& jsonObject);
    static QJsonObject getJsonObjectFromString(const QString jsonString);
    void getHostIpAddress(void);
    void PublishMessageToGw(QString msg, QString topic);
    void DevManageInit(void);
    void AppToCloudInit(void);
    void EncryptSendToCloud(QString Theme,QString PlayLoad);
    void manualBackupInit(void);

private:
    Ui::MainWindow *ui;
    QPlainTextEdit GwResponInfoShow;//server回复数据显示
    QPushButton PublishInfoTtoGwButton;//发布消息按钮
    QPlainTextEdit PublishInfoTtoGw;//发布消息to server
    QPlainTextEdit PublishThemeTtoGw;//发布主题to server
    QTimer t1;//定时发布
    QPushButton TeClearButton;

    QLineEdit MqttServerAddr;
    QLineEdit GwMqttPortLineEdit;
    QPushButton ConnectGwButton;
    QPushButton DisConnectGwButton;
    Publisher *publisher_ ;
    QString ReceivePublishTheme;
    QString ReceivePublishMessage;
    QLineEdit LocalIpLineEdit;
    int IsConnectGw;//1连接上server,0没有连接上server
    int ISClickBroadcastButton;//1点击，0没有点击

    QTabWidget TabWidget;//分页
    QMainWindow CommonWin; //通用分页窗口

public slots:
    void ShowPublishReceiveMessage(void);
    void TeClearButtonClick(void);
    void ConnectGwButtonClick(void);
    void DisConnectGwButtonClick(void);
    void PublishMessageReceived(const QMQTT::Message &message);
    void ConnectAckReponse(void);
    void GwDisconnect(void);
    void PublishInfoTtoGwButtonClick(void);
};

#endif // MAINWINDOW_H
