#ifndef OBSCLIENT_H
#define OBSCLIENT_H

#include <QJsonObject>
#include <QAbstractSocket>
#include <QObject>
#include <QUrl>
#include <QWebSocket>

class OBSClient : public QObject
{
    Q_OBJECT

public:
    explicit OBSClient(QObject *parent = nullptr);

    void connectToObs(const QUrl &url);
    void disconnectFromObs();
    void sendRequest(const QString &type,
                     const QJsonObject &data = {},
                     const QString &requestId = QString());
    void setPassword(const QString &password);

    bool isConnected() const;
    bool isAuthenticated() const;

signals:
    void connected();
    void disconnected();
    void authenticated();
    void errorOccurred(const QString &message);
    void requestSucceeded(const QString &requestType,
                          const QString &requestId);
    void requestFailed(const QString &requestType,
                       const QString &requestId,
                       const QString &comment,
                       int code);
    void messageReceived(const QJsonObject &json);

private slots:
    void onConnected();
    void onDisconnected();
    void onTextMessageReceived(const QString &message);
    void onSocketError(QAbstractSocket::SocketError error);

private:
    QString createAuthResponse(const QString &password,
                               const QString &salt,
                               const QString &challenge) const;
    QString nextRequestId() const;
    void sendIdentify(const QString &authentication = QString());

    QWebSocket m_socket;
    QString m_password;
    bool m_authenticated = false;
};

#endif // OBSCLIENT_H
