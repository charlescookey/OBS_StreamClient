#include "obsclient.h"

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QUuid>

OBSClient::OBSClient(QObject *parent)
    : QObject(parent)
{
    connect(&m_socket, &QWebSocket::connected,
            this, &OBSClient::onConnected);
    connect(&m_socket, &QWebSocket::disconnected,
            this, &OBSClient::onDisconnected);
    connect(&m_socket, &QWebSocket::textMessageReceived,
            this, &OBSClient::onTextMessageReceived);

    connect(&m_socket,
            QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
            this,
            &OBSClient::onSocketError);
}

void OBSClient::connectToObs(const QUrl &url)
{
    m_authenticated = false;
    m_socket.open(url);
}

void OBSClient::disconnectFromObs()
{
    m_authenticated = false;
    m_socket.close();
}

void OBSClient::sendRequest(const QString &type,
                            const QJsonObject &data,
                            const QString &requestId)
{
    if (!isConnected() || !isAuthenticated()) {
        emit errorOccurred(QStringLiteral("Cannot send %1 before OBS authentication completes.")
                               .arg(type));
        return;
    }

    QJsonObject msg;
    msg["op"] = 6;
    msg["d"] = QJsonObject{
        {"requestType", type},
        {"requestId", requestId.isEmpty() ? nextRequestId() : requestId},
        {"requestData", data}
    };

    m_socket.sendTextMessage(
        QJsonDocument(msg).toJson(QJsonDocument::Compact)
        );
}

void OBSClient::setPassword(const QString &password)
{
    m_password = password;
}

bool OBSClient::isConnected() const
{
    return m_socket.state() == QAbstractSocket::ConnectedState;
}

bool OBSClient::isAuthenticated() const
{
    return m_authenticated;
}

void OBSClient::onConnected()
{
    emit connected();
}

void OBSClient::onDisconnected()
{
    m_authenticated = false;
    emit disconnected();
}

void OBSClient::onTextMessageReceived(const QString &message)
{
    const QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (!doc.isObject()) {
        emit errorOccurred(QStringLiteral("OBS sent a non-JSON message."));
        return;
    }

    const QJsonObject root = doc.object();
    const int op = root.value("op").toInt(-1);
    const QJsonObject d = root.value("d").toObject();

    if (op == 0) {
        const QJsonObject auth = d.value("authentication").toObject();
        const QString salt = auth.value("salt").toString();
        const QString challenge = auth.value("challenge").toString();

        if (!auth.isEmpty()) {
            sendIdentify(createAuthResponse(m_password, salt, challenge));
        } else {
            sendIdentify();
        }

        return;
    }

    if (op == 2) {
        m_authenticated = true;
        emit authenticated();
        return;
    }

    if (op == 7) {
        const QJsonObject requestStatus = d.value("requestStatus").toObject();
        const bool result = requestStatus.value("result").toBool();
        const QString requestType = d.value("requestType").toString();
        const QString requestId = d.value("requestId").toString();
        const QString comment = requestStatus.value("comment").toString();
        const int code = requestStatus.value("code").toInt();

        if (result) {
            emit requestSucceeded(requestType, requestId);
        } else {
            emit requestFailed(requestType, requestId, comment, code);
        }
    }

    emit messageReceived(root);
}

void OBSClient::onSocketError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error)
    emit errorOccurred(m_socket.errorString());
}

QString OBSClient::createAuthResponse(const QString &password,
                                      const QString &salt,
                                      const QString &challenge) const
{
    const QByteArray secret = password.toUtf8() + salt.toUtf8();
    const QByteArray secretHash =
        QCryptographicHash::hash(secret, QCryptographicHash::Sha256)
            .toBase64();

    const QByteArray authResponse =
        QCryptographicHash::hash(secretHash + challenge.toUtf8(),
                                 QCryptographicHash::Sha256)
            .toBase64();

    return QString::fromUtf8(authResponse);
}

QString OBSClient::nextRequestId() const
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

void OBSClient::sendIdentify(const QString &authentication)
{
    QJsonObject identifyData{
        {"rpcVersion", 1}
    };

    if (!authentication.isEmpty()) {
        identifyData["authentication"] = authentication;
    }

    QJsonObject identify;
    identify["op"] = 1;
    identify["d"] = identifyData;

    m_socket.sendTextMessage(
        QJsonDocument(identify).toJson(QJsonDocument::Compact)
        );
}
