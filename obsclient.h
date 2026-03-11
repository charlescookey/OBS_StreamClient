#ifndef OBSCLIENT_H
#define OBSCLIENT_H

#include <QObject>
#include <QWebSocket>
#include <QJsonObject>

class OBSClient : public QObject
{
    Q_OBJECT
public:
    explicit OBSClient(QObject *parent = nullptr);

    void connectToObs(const QUrl &url);
    void sendRequest(const QString &type,
                     const QJsonObject &data = {},
                     const QString &requestId = "1");


    void setPassword(const QString &password);

signals:
    void authenticated();
    void messageReceived(const QJsonObject &json);

private slots:
    void onConnected();
    void onTextMessageReceived(const QString &message);

private:
    QString createAuthResponse(const QString &password,
                               const QString &salt,
                               const QString &challenge);

    QWebSocket m_socket;
    QString m_password;


};

#endif // OBSCLIENT_H
