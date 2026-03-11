#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDateTime>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QStringList>

namespace {

constexpr int kClientCount = 2;

QString timestampedMessage(const QString &message)
{
    return QStringLiteral("[%1] %2")
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss")),
             message);
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle(QStringLiteral("OBS Stream Client"));

    for (int index = 0; index < kClientCount; ++index) {
        m_clients[index] = std::make_unique<OBSClient>();
    }

    applyWindowStyle();
    setupConnections();
    loadSettings();

    ui->tabWidget->setCurrentIndex(0);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onSaveSettingsClicked()
{
    saveSettings();
    appendLog(QStringLiteral("Saved OBS connection and action settings."));
}

void MainWindow::onConnectAllClicked()
{
    saveSettings();
    broadcast([this](int index) { connectClient(index); });
}

void MainWindow::onDisconnectAllClicked()
{
    broadcast([this](int index) { disconnectClient(index); });
}

void MainWindow::onApplySceneAllClicked()
{
    broadcast([this](int index) { switchSceneForClient(index); });
}

void MainWindow::onApplySceneCollectionAllClicked()
{
    broadcast([this](int index) { applySceneCollectionForClient(index); });
}

void MainWindow::onStartStreamAllClicked()
{
    broadcast([this](int index) { startStreamForClient(index); });
}

void MainWindow::onStopStreamAllClicked()
{
    broadcast([this](int index) { stopStreamForClient(index); });
}

void MainWindow::onStartRecordingAllClicked()
{
    broadcast([this](int index) { startRecordingForClient(index); });
}

void MainWindow::onStopRecordingAllClicked()
{
    broadcast([this](int index) { stopRecordingForClient(index); });
}

void MainWindow::setupConnections()
{
    connect(ui->saveSettingsButton, &QPushButton::clicked,
            this, &MainWindow::onSaveSettingsClicked);
    connect(ui->connectAllButton, &QPushButton::clicked,
            this, &MainWindow::onConnectAllClicked);
    connect(ui->disconnectAllButton, &QPushButton::clicked,
            this, &MainWindow::onDisconnectAllClicked);
    connect(ui->applySceneAllButton, &QPushButton::clicked,
            this, &MainWindow::onApplySceneAllClicked);
    connect(ui->applySceneCollectionAllButton, &QPushButton::clicked,
            this, &MainWindow::onApplySceneCollectionAllClicked);
    connect(ui->startStreamAllButton, &QPushButton::clicked,
            this, &MainWindow::onStartStreamAllClicked);
    connect(ui->stopStreamAllButton, &QPushButton::clicked,
            this, &MainWindow::onStopStreamAllClicked);
    connect(ui->startRecordingAllButton, &QPushButton::clicked,
            this, &MainWindow::onStartRecordingAllClicked);
    connect(ui->stopRecordingAllButton, &QPushButton::clicked,
            this, &MainWindow::onStopRecordingAllClicked);

    connect(ui->client1ConnectButton, &QPushButton::clicked,
            this, [this]() { saveSettings(); connectClient(0); });
    connect(ui->client1DisconnectButton, &QPushButton::clicked,
            this, [this]() { disconnectClient(0); });
    connect(ui->client1SceneButton, &QPushButton::clicked,
            this, [this]() { switchSceneForClient(0); });
    connect(ui->client1SceneCollectionButton, &QPushButton::clicked,
            this, [this]() { applySceneCollectionForClient(0); });
    connect(ui->client1StartStreamButton, &QPushButton::clicked,
            this, [this]() { startStreamForClient(0); });
    connect(ui->client1StopStreamButton, &QPushButton::clicked,
            this, [this]() { stopStreamForClient(0); });
    connect(ui->client1StartRecordingButton, &QPushButton::clicked,
            this, [this]() { startRecordingForClient(0); });
    connect(ui->client1StopRecordingButton, &QPushButton::clicked,
            this, [this]() { stopRecordingForClient(0); });

    connect(ui->client2ConnectButton, &QPushButton::clicked,
            this, [this]() { saveSettings(); connectClient(1); });
    connect(ui->client2DisconnectButton, &QPushButton::clicked,
            this, [this]() { disconnectClient(1); });
    connect(ui->client2SceneButton, &QPushButton::clicked,
            this, [this]() { switchSceneForClient(1); });
    connect(ui->client2SceneCollectionButton, &QPushButton::clicked,
            this, [this]() { applySceneCollectionForClient(1); });
    connect(ui->client2StartStreamButton, &QPushButton::clicked,
            this, [this]() { startStreamForClient(1); });
    connect(ui->client2StopStreamButton, &QPushButton::clicked,
            this, [this]() { stopStreamForClient(1); });
    connect(ui->client2StartRecordingButton, &QPushButton::clicked,
            this, [this]() { startRecordingForClient(1); });
    connect(ui->client2StopRecordingButton, &QPushButton::clicked,
            this, [this]() { stopRecordingForClient(1); });

    for (int index = 0; index < kClientCount; ++index) {
        OBSClient *client = m_clients[index].get();

        connect(client, &OBSClient::connected, this, [this, index]() {
            updateClientStatus(index, QStringLiteral("Connected"),
                               QStringLiteral("Waiting for OBS authentication"));
            appendLog(QStringLiteral("%1 socket connected.")
                          .arg(clientDisplayName(index)));
        });

        connect(client, &OBSClient::disconnected, this, [this, index]() {
            updateClientStatus(index, QStringLiteral("Disconnected"));
            appendLog(QStringLiteral("%1 disconnected.")
                          .arg(clientDisplayName(index)));
        });

        connect(client, &OBSClient::authenticated, this, [this, index]() {
            updateClientStatus(index, QStringLiteral("Ready"),
                               QStringLiteral("Authenticated with OBS"));
            appendLog(QStringLiteral("%1 authenticated.")
                          .arg(clientDisplayName(index)));
        });

        connect(client, &OBSClient::errorOccurred, this,
                [this, index](const QString &message) {
            updateClientStatus(index, QStringLiteral("Error"), message);
            appendLog(QStringLiteral("%1 error: %2")
                          .arg(clientDisplayName(index), message));
        });

        connect(client, &OBSClient::requestSucceeded, this,
                [this, index](const QString &requestType, const QString &) {
            updateClientStatus(index, QStringLiteral("Ready"),
                               QStringLiteral("Last action: %1").arg(requestType));
            appendLog(QStringLiteral("%1 completed %2.")
                          .arg(clientDisplayName(index), requestType));
        });

        connect(client, &OBSClient::requestFailed, this,
                [this, index](const QString &requestType, const QString &,
                              const QString &comment, int code) {
            const QString detail =
                QStringLiteral("%1 failed (%2): %3")
                    .arg(requestType)
                    .arg(code)
                    .arg(comment.isEmpty() ? QStringLiteral("No details from OBS")
                                           : comment);

            updateClientStatus(index, QStringLiteral("Request failed"), detail);
            appendLog(QStringLiteral("%1 %2")
                          .arg(clientDisplayName(index), detail));
        });
    }
}

void MainWindow::applyWindowStyle()
{
    setStyleSheet(QStringLiteral(
        "QMainWindow {"
        "  background: #0b1220;"
        "}"
        "QWidget {"
        "  color: #dce7f3;"
        "  font-size: 14px;"
        "}"
        "QTabWidget::pane {"
        "  border: 1px solid #24324a;"
        "  background: #111a2b;"
        "  border-radius: 12px;"
        "  top: -1px;"
        "}"
        "QTabBar::tab {"
        "  background: #172338;"
        "  color: #98abc5;"
        "  padding: 10px 18px;"
        "  margin-right: 6px;"
        "  border-top-left-radius: 10px;"
        "  border-top-right-radius: 10px;"
        "}"
        "QTabBar::tab:selected {"
        "  background: #1e3657;"
        "  color: #ffffff;"
        "}"
        "QGroupBox {"
        "  border: 1px solid #24324a;"
        "  border-radius: 16px;"
        "  margin-top: 18px;"
        "  padding: 18px 14px 14px 14px;"
        "  background: #121d31;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  left: 14px;"
        "  padding: 0 6px;"
        "  color: #f5f7fb;"
        "  font-weight: 600;"
        "}"
        "QLineEdit, QSpinBox, QPlainTextEdit {"
        "  background: #0d1524;"
        "  border: 1px solid #2a3d59;"
        "  border-radius: 10px;"
        "  padding: 8px 10px;"
        "}"
        "QPushButton {"
        "  background: #3a7afe;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 10px;"
        "  padding: 10px 14px;"
        "  font-weight: 600;"
        "}"
        "QPushButton:hover {"
        "  background: #4f8cff;"
        "}"
        "QPushButton:pressed {"
        "  background: #2b65da;"
        "}"
        "QPushButton#disconnectAllButton, "
        "QPushButton#client1DisconnectButton, "
        "QPushButton#client2DisconnectButton, "
        "QPushButton#stopStreamAllButton, "
        "QPushButton#stopRecordingAllButton, "
        "QPushButton#client1StopStreamButton, "
        "QPushButton#client2StopStreamButton, "
        "QPushButton#client1StopRecordingButton, "
        "QPushButton#client2StopRecordingButton {"
        "  background: #d14b5a;"
        "}"
        "QPushButton#disconnectAllButton:hover, "
        "QPushButton#client1DisconnectButton:hover, "
        "QPushButton#client2DisconnectButton:hover, "
        "QPushButton#stopStreamAllButton:hover, "
        "QPushButton#stopRecordingAllButton:hover, "
        "QPushButton#client1StopStreamButton:hover, "
        "QPushButton#client2StopStreamButton:hover, "
        "QPushButton#client1StopRecordingButton:hover, "
        "QPushButton#client2StopRecordingButton:hover {"
        "  background: #e25c6c;"
        "}"
        "QLabel#heroTitleLabel {"
        "  font-size: 24px;"
        "  font-weight: 700;"
        "  color: #f5f7fb;"
        "}"
        "QLabel#heroSubtitleLabel {"
        "  color: #8ea2bf;"
        "  font-size: 13px;"
        "}"
        "QLabel#client1StatusLabel, "
        "QLabel#client2StatusLabel {"
        "  background: #0d1524;"
        "  border: 1px solid #2a3d59;"
        "  border-radius: 10px;"
        "  padding: 10px;"
        "  font-weight: 600;"
        "}"
    ));
}

void MainWindow::loadSettings()
{
    QSettings settings(QStringLiteral("OBSStreamClient"),
                       QStringLiteral("OBSStreamClient"));

    for (int index = 0; index < kClientCount; ++index) {
        const QString prefix = QStringLiteral("clients/%1").arg(index);
        ClientConfig config;
        config.name = settings.value(prefix + QStringLiteral("/name"),
                                     QStringLiteral("Client %1").arg(index + 1))
                          .toString();
        config.host = settings.value(prefix + QStringLiteral("/host")).toString();
        config.port = settings.value(prefix + QStringLiteral("/port"), 4455).toInt();
        config.password = settings.value(prefix + QStringLiteral("/password")).toString();

        writeClientConfigToUi(index, config);
        updateClientStatus(index, QStringLiteral("Idle"),
                           QStringLiteral("Configure this client and connect."));
    }

    ui->sceneNameEdit->setText(
        settings.value(QStringLiteral("actions/sceneName"),
                       QStringLiteral("Scene")).toString());
    ui->sceneCollectionEdit->setText(
        settings.value(QStringLiteral("actions/sceneCollection")).toString());
    ui->logOutput->clear();
    appendLog(QStringLiteral("Loaded saved settings."));
}

void MainWindow::saveSettings()
{
    QSettings settings(QStringLiteral("OBSStreamClient"),
                       QStringLiteral("OBSStreamClient"));

    for (int index = 0; index < kClientCount; ++index) {
        const QString prefix = QStringLiteral("clients/%1").arg(index);
        const ClientConfig config = readClientConfigFromUi(index);

        settings.setValue(prefix + QStringLiteral("/name"), config.name);
        settings.setValue(prefix + QStringLiteral("/host"), config.host);
        settings.setValue(prefix + QStringLiteral("/port"), config.port);
        settings.setValue(prefix + QStringLiteral("/password"), config.password);
    }

    settings.setValue(QStringLiteral("actions/sceneName"), sceneName());
    settings.setValue(QStringLiteral("actions/sceneCollection"), sceneCollectionName());
}

MainWindow::ClientConfig MainWindow::readClientConfigFromUi(int index) const
{
    ClientConfig config;

    if (index == 0) {
        config.name = ui->client1NameEdit->text().trimmed();
        config.host = ui->client1HostEdit->text().trimmed();
        config.port = ui->client1PortSpinBox->value();
        config.password = ui->client1PasswordEdit->text();
    } else {
        config.name = ui->client2NameEdit->text().trimmed();
        config.host = ui->client2HostEdit->text().trimmed();
        config.port = ui->client2PortSpinBox->value();
        config.password = ui->client2PasswordEdit->text();
    }

    if (config.name.isEmpty()) {
        config.name = QStringLiteral("Client %1").arg(index + 1);
    }

    return config;
}

void MainWindow::writeClientConfigToUi(int index, const ClientConfig &config)
{
    if (index == 0) {
        ui->client1NameEdit->setText(config.name);
        ui->client1HostEdit->setText(config.host);
        ui->client1PortSpinBox->setValue(config.port);
        ui->client1PasswordEdit->setText(config.password);
    } else {
        ui->client2NameEdit->setText(config.name);
        ui->client2HostEdit->setText(config.host);
        ui->client2PortSpinBox->setValue(config.port);
        ui->client2PasswordEdit->setText(config.password);
    }
}

QUrl MainWindow::buildUrl(const ClientConfig &config) const
{
    QString hostValue = config.host.trimmed();
    if (hostValue.isEmpty()) {
        return QUrl();
    }

    QUrl url = hostValue.contains(QStringLiteral("://"))
                   ? QUrl(hostValue)
                   : QUrl(QStringLiteral("ws://%1").arg(hostValue));

    if (!url.isValid()) {
        return QUrl();
    }

    if (url.port() == -1 && config.port > 0) {
        url.setPort(config.port);
    }

    return url;
}

QString MainWindow::sceneName() const
{
    return ui->sceneNameEdit->text().trimmed();
}

QString MainWindow::sceneCollectionName() const
{
    return ui->sceneCollectionEdit->text().trimmed();
}

QString MainWindow::clientDisplayName(int index) const
{
    return readClientConfigFromUi(index).name;
}

void MainWindow::connectClient(int index)
{
    const ClientConfig config = readClientConfigFromUi(index);
    const QUrl url = buildUrl(config);

    if (!url.isValid()) {
        updateClientStatus(index, QStringLiteral("Invalid settings"),
                           QStringLiteral("Enter a valid OBS host or ws:// URL."));
        appendLog(QStringLiteral("%1 has invalid connection settings.")
                      .arg(config.name));
        return;
    }

    m_clients[index]->setPassword(config.password);
    updateClientStatus(index, QStringLiteral("Connecting"),
                       QStringLiteral("Opening %1").arg(url.toString()));
    appendLog(QStringLiteral("Connecting %1 to %2")
                  .arg(config.name, url.toString()));
    m_clients[index]->connectToObs(url);
}

void MainWindow::disconnectClient(int index)
{
    appendLog(QStringLiteral("Disconnecting %1.").arg(clientDisplayName(index)));
    m_clients[index]->disconnectFromObs();
}

void MainWindow::switchSceneForClient(int index)
{
    const QString name = sceneName();
    if (name.isEmpty()) {
        appendLog(QStringLiteral("Set a scene name before switching scenes."));
        return;
    }

    m_clients[index]->sendRequest(
        QStringLiteral("SetCurrentProgramScene"),
        QJsonObject{{QStringLiteral("sceneName"), name}}
        );
}

void MainWindow::applySceneCollectionForClient(int index)
{
    const QString name = sceneCollectionName();
    if (name.isEmpty()) {
        appendLog(QStringLiteral("Set a scene collection name before applying it."));
        return;
    }

    m_clients[index]->sendRequest(
        QStringLiteral("SetCurrentSceneCollection"),
        QJsonObject{{QStringLiteral("sceneCollectionName"), name}}
        );
}

void MainWindow::startStreamForClient(int index)
{
    m_clients[index]->sendRequest(QStringLiteral("StartStream"));
}

void MainWindow::stopStreamForClient(int index)
{
    m_clients[index]->sendRequest(QStringLiteral("StopStream"));
}

void MainWindow::startRecordingForClient(int index)
{
    m_clients[index]->sendRequest(QStringLiteral("StartRecord"));
}

void MainWindow::stopRecordingForClient(int index)
{
    m_clients[index]->sendRequest(QStringLiteral("StopRecord"));
}

void MainWindow::broadcast(const std::function<void(int)> &operation)
{
    for (int index = 0; index < kClientCount; ++index) {
        operation(index);
    }
}

void MainWindow::updateClientStatus(int index,
                                    const QString &state,
                                    const QString &detail)
{
    QLabel *label = index == 0 ? ui->client1StatusLabel : ui->client2StatusLabel;
    const QString text = detail.isEmpty()
                             ? QStringLiteral("%1\n%2")
                                   .arg(clientDisplayName(index), state)
                             : QStringLiteral("%1\n%2\n%3")
                                   .arg(clientDisplayName(index), state, detail);
    label->setText(text);
}

void MainWindow::appendLog(const QString &message)
{
    ui->logOutput->appendPlainText(timestampedMessage(message));
}
