#include "obsclient.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QCryptographicHash>
#include <QDebug>

OBSClient::OBSClient(QObject *parent)
    : QObject(parent)
{
    connect(&m_socket, &QWebSocket::connected,
            this, &OBSClient::onConnected);

    connect(&m_socket, &QWebSocket::textMessageReceived,
            this, &OBSClient::onTextMessageReceived);
}

void OBSClient::connectToObs(const QUrl &url)
{
    m_socket.open(url);
}

void OBSClient::onConnected()
{
    qDebug() << "WebSocket connected, waiting for OBS Hello...";
}

void OBSClient::onTextMessageReceived(const QString &message)
{
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    QJsonObject root = doc.object();

    int op = root.value("op").toInt();
    QJsonObject d = root.value("d").toObject();

    // OBS Hello
    if (op == 0) {
        qDebug() << "Received Hello";
        qDebug()<< message;

        if (d.contains("authentication")) {
            QJsonObject auth = d["authentication"].toObject();

            QString salt = auth["salt"].toString();
            QString challenge = auth["challenge"].toString();

            QString authResponse =
                createAuthResponse(m_password, salt, challenge);

            QJsonObject identify;
            identify["op"] = 1;
            identify["d"] = QJsonObject{
                {"rpcVersion", 1},
                {"authentication", authResponse}
            };

            m_socket.sendTextMessage(
                QJsonDocument(identify).toJson(QJsonDocument::Compact)
                );
        }
        return;
    }

    // OBS Identified (success)
    if (op == 2) {
        qDebug() << "Authenticated successfully";
        emit authenticated();
        return;
    }

    emit messageReceived(root);
}

QString OBSClient::createAuthResponse(const QString &password,
                                      const QString &salt,
                                      const QString &challenge)
{
    // Step 1: base64(SHA256(password + salt))
    QByteArray secret = password.toUtf8() + salt.toUtf8();
    QByteArray secretHash =
        QCryptographicHash::hash(secret, QCryptographicHash::Sha256)
            .toBase64();

    // Step 2: base64(SHA256(secretHash + challenge))
    QByteArray authResponse =
        QCryptographicHash::hash(secretHash + challenge.toUtf8(),
                                 QCryptographicHash::Sha256)
            .toBase64();

    return QString::fromUtf8(authResponse);
}

void OBSClient::sendRequest(const QString &type,
                            const QJsonObject &data,
                            const QString &requestId)
{
    QJsonObject msg;
    msg["op"] = 6;
    msg["d"] = QJsonObject{
        {"requestType", type},
        {"requestId", requestId},
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
