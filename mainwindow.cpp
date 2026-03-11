#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QColor>
#include <QComboBox>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QUuid>

namespace {

constexpr int kMaxClients = 20;

QString timestampedMessage(const QString &message)
{
    return QStringLiteral("[%1] %2")
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss")),
             message);
}

QString defaultClientName(int index)
{
    return QStringLiteral("Client %1").arg(index + 1);
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle(QStringLiteral("OBS Stream Client"));

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
    updateCurrentSettingsClientFromUi();
    saveSettings();
    appendLog(QStringLiteral("Saved OBS connection and action settings."));
}

void MainWindow::onAddClientClicked()
{
    if (static_cast<int>(m_clients.size()) >= kMaxClients) {
        appendLog(QStringLiteral("Client limit reached. Maximum is %1.").arg(kMaxClients));
        return;
    }

    updateCurrentSettingsClientFromUi();

    ClientConfig config;
    config.name = defaultClientName(static_cast<int>(m_clients.size()));
    addClient(config);
    refreshClientSelectors();

    {
        const QSignalBlocker blocker(ui->settingsClientSelector);
        ui->settingsClientSelector->setCurrentIndex(static_cast<int>(m_clients.size()) - 1);
    }
    loadClientIntoSettingsForm(static_cast<int>(m_clients.size()) - 1);

    {
        const QSignalBlocker blocker(ui->controlsClientSelector);
        ui->controlsClientSelector->setCurrentIndex(static_cast<int>(m_clients.size()) - 1);
    }

    refreshStatusTable();
    refreshControlsPanel();
    saveSettings();
    appendLog(QStringLiteral("Added %1.")
                  .arg(clientDisplayName(static_cast<int>(m_clients.size()) - 1)));
}

void MainWindow::onSettingsClientChanged(int index)
{
    loadClientIntoSettingsForm(index);
}

void MainWindow::onControlsClientChanged(int index)
{
    Q_UNUSED(index)
    refreshControlsPanel();
}

void MainWindow::onRefreshSceneCollectionsClicked()
{
    updateCurrentSettingsClientFromUi();

    const int index = ui->settingsClientSelector->currentIndex();
    if (index < 0 || index >= static_cast<int>(m_clients.size())) {
        appendLog(QStringLiteral("Select a client before loading scene collections."));
        return;
    }

    OBSClient *client = m_clients[index].client.get();
    if (!client->isConnected() || !client->isAuthenticated()) {
        appendLog(QStringLiteral("Connect %1 before loading scene collections.")
                      .arg(clientDisplayName(index)));
        return;
    }

    const QString requestId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_sceneCollectionRequests[requestId] = index;
    client->sendRequest(QStringLiteral("GetSceneCollectionList"),
                        QJsonObject{},
                        requestId);
    appendLog(QStringLiteral("Requesting scene collections from %1.")
                  .arg(clientDisplayName(index)));
}

void MainWindow::onStatusSelectionChanged()
{
    refreshStatusButtons();
}

void MainWindow::onConnectSelectedStatusClicked()
{
    const int index = selectedStatusIndex();
    if (index >= 0) {
        connectClient(index);
    }
}

void MainWindow::onDisconnectSelectedStatusClicked()
{
    const int index = selectedStatusIndex();
    if (index >= 0) {
        disconnectClient(index);
    }
}

void MainWindow::onConnectSelectedControlClicked()
{
    const int index = selectedControlsIndex();
    if (index >= 0) {
        connectClient(index);
    }
}

void MainWindow::onDisconnectSelectedControlClicked()
{
    const int index = selectedControlsIndex();
    if (index >= 0) {
        disconnectClient(index);
    }
}

void MainWindow::onApplySceneSelectedClicked()
{
    const int index = selectedControlsIndex();
    if (index >= 0) {
        switchSceneForClient(index);
    }
}

void MainWindow::onApplySceneCollectionSelectedClicked()
{
    const int index = selectedControlsIndex();
    if (index >= 0) {
        applySceneCollectionForClient(index);
    }
}

void MainWindow::onStartStreamSelectedClicked()
{
    const int index = selectedControlsIndex();
    if (index >= 0) {
        startStreamForClient(index);
    }
}

void MainWindow::onStopStreamSelectedClicked()
{
    const int index = selectedControlsIndex();
    if (index >= 0) {
        stopStreamForClient(index);
    }
}

void MainWindow::onStartRecordingSelectedClicked()
{
    const int index = selectedControlsIndex();
    if (index >= 0) {
        startRecordingForClient(index);
    }
}

void MainWindow::onStopRecordingSelectedClicked()
{
    const int index = selectedControlsIndex();
    if (index >= 0) {
        stopRecordingForClient(index);
    }
}

void MainWindow::onConnectAllClicked()
{
    updateCurrentSettingsClientFromUi();
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
    connect(ui->addClientButton, &QPushButton::clicked,
            this, &MainWindow::onAddClientClicked);
    connect(ui->refreshSceneCollectionsButton, &QPushButton::clicked,
            this, &MainWindow::onRefreshSceneCollectionsClicked);

    connect(ui->settingsClientSelector,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &MainWindow::onSettingsClientChanged);
    connect(ui->controlsClientSelector,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &MainWindow::onControlsClientChanged);

    connect(ui->statusTable, &QTableWidget::itemSelectionChanged,
            this, &MainWindow::onStatusSelectionChanged);

    connect(ui->connectSelectedStatusButton, &QPushButton::clicked,
            this, &MainWindow::onConnectSelectedStatusClicked);
    connect(ui->disconnectSelectedStatusButton, &QPushButton::clicked,
            this, &MainWindow::onDisconnectSelectedStatusClicked);
    connect(ui->connectSelectedControlButton, &QPushButton::clicked,
            this, &MainWindow::onConnectSelectedControlClicked);
    connect(ui->disconnectSelectedControlButton, &QPushButton::clicked,
            this, &MainWindow::onDisconnectSelectedControlClicked);
    connect(ui->applySceneSelectedButton, &QPushButton::clicked,
            this, &MainWindow::onApplySceneSelectedClicked);
    connect(ui->applySceneCollectionSelectedButton, &QPushButton::clicked,
            this, &MainWindow::onApplySceneCollectionSelectedClicked);
    connect(ui->startStreamSelectedButton, &QPushButton::clicked,
            this, &MainWindow::onStartStreamSelectedClicked);
    connect(ui->stopStreamSelectedButton, &QPushButton::clicked,
            this, &MainWindow::onStopStreamSelectedClicked);
    connect(ui->startRecordingSelectedButton, &QPushButton::clicked,
            this, &MainWindow::onStartRecordingSelectedClicked);
    connect(ui->stopRecordingSelectedButton, &QPushButton::clicked,
            this, &MainWindow::onStopRecordingSelectedClicked);

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

    auto updateCurrentClient = [this]() { updateCurrentSettingsClientFromUi(); };

    connect(ui->clientNameEdit, &QLineEdit::textChanged,
            this, updateCurrentClient);
    connect(ui->clientHostEdit, &QLineEdit::textChanged,
            this, updateCurrentClient);
    connect(ui->clientPortSpinBox,
            QOverload<int>::of(&QSpinBox::valueChanged),
            this,
            [updateCurrentClient](int) { updateCurrentClient(); });
    connect(ui->clientPasswordEdit, &QLineEdit::textChanged,
            this, updateCurrentClient);
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
        "QFrame#heroFrame {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #14233a, stop:1 #0f1727);"
        "  border: 1px solid #22324b;"
        "  border-radius: 18px;"
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
        "QLineEdit, QSpinBox, QPlainTextEdit, QComboBox, QTableWidget {"
        "  background: #0d1524;"
        "  color: #dce7f3;"
        "  border: 1px solid #2a3d59;"
        "  border-radius: 10px;"
        "  padding: 8px 10px;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background: #0d1524;"
        "  color: #dce7f3;"
        "  selection-background-color: #1e3657;"
        "  selection-color: #ffffff;"
        "  border: 1px solid #2a3d59;"
        "}"
        "QTableWidget {"
        "  gridline-color: #22324b;"
        "}"
        "QHeaderView::section {"
        "  background: #162338;"
        "  color: #dce7f3;"
        "  border: none;"
        "  padding: 8px;"
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
        "QPushButton#disconnectSelectedStatusButton, "
        "QPushButton#disconnectSelectedControlButton, "
        "QPushButton#disconnectAllButton, "
        "QPushButton#stopStreamSelectedButton, "
        "QPushButton#stopRecordingSelectedButton, "
        "QPushButton#stopStreamAllButton, "
        "QPushButton#stopRecordingAllButton {"
        "  background: #d14b5a;"
        "}"
        "QPushButton#disconnectSelectedStatusButton:hover, "
        "QPushButton#disconnectSelectedControlButton:hover, "
        "QPushButton#disconnectAllButton:hover, "
        "QPushButton#stopStreamSelectedButton:hover, "
        "QPushButton#stopRecordingSelectedButton:hover, "
        "QPushButton#stopStreamAllButton:hover, "
        "QPushButton#stopRecordingAllButton:hover {"
        "  background: #e25c6c;"
        "}"
        "QLabel#heroTitleLabel {"
        "  font-size: 24px;"
        "  font-weight: 700;"
        "  color: #f5f7fb;"
        "}"
        "QLabel#heroSubtitleLabel, QLabel#clientLimitLabel, QLabel#statusHintLabel {"
        "  color: #8ea2bf;"
        "  font-size: 13px;"
        "}"
        "QLabel#controlClientNameLabel {"
        "  font-size: 18px;"
        "  font-weight: 700;"
        "  color: #f5f7fb;"
        "}"
        "QLabel#controlStatusBadge {"
        "  border-radius: 12px;"
        "  padding: 8px 12px;"
        "  font-weight: 700;"
        "}"
        "QLabel#controlEndpointLabel, QLabel#controlDetailLabel {"
        "  color: #9ab0cb;"
        "}"
    ));
}

void MainWindow::loadSettings()
{
    QSettings settings(QStringLiteral("OBSStreamClient"),
                       QStringLiteral("OBSStreamClient"));

    m_clients.clear();
    m_sceneCollectionRequests.clear();

    const int count = qMin(settings.beginReadArray(QStringLiteral("clients")),
                           kMaxClients);

    for (int index = 0; index < count; ++index) {
        settings.setArrayIndex(index);

        ClientConfig config;
        config.name = settings.value(QStringLiteral("name"),
                                     defaultClientName(index)).toString();
        config.host = settings.value(QStringLiteral("host")).toString();
        config.port = settings.value(QStringLiteral("port"), 4455).toInt();
        config.password = settings.value(QStringLiteral("password")).toString();
        addClient(config);
    }

    settings.endArray();

    if (m_clients.empty()) {
        addClient(ClientConfig{defaultClientName(0), QString(), 4455, QString()});
    }

    ui->sceneNameEdit->setText(
        settings.value(QStringLiteral("actions/sceneName"),
                       QStringLiteral("Scene")).toString());
    populateSceneCollections(QStringList(),
                             settings.value(QStringLiteral("actions/sceneCollection"))
                                 .toString());

    refreshClientSelectors();
    loadClientIntoSettingsForm(0);
    refreshStatusTable();
    refreshControlsPanel();
    ui->logOutput->clear();
    appendLog(QStringLiteral("Loaded %1 client profile(s).").arg(m_clients.size()));
}

void MainWindow::saveSettings()
{
    QSettings settings(QStringLiteral("OBSStreamClient"),
                       QStringLiteral("OBSStreamClient"));

    settings.remove(QStringLiteral("clients"));
    settings.beginWriteArray(QStringLiteral("clients"), static_cast<int>(m_clients.size()));

    for (int index = 0; index < static_cast<int>(m_clients.size()); ++index) {
        settings.setArrayIndex(index);
        settings.setValue(QStringLiteral("name"), m_clients[index].config.name);
        settings.setValue(QStringLiteral("host"), m_clients[index].config.host);
        settings.setValue(QStringLiteral("port"), m_clients[index].config.port);
        settings.setValue(QStringLiteral("password"), m_clients[index].config.password);
    }

    settings.endArray();
    settings.setValue(QStringLiteral("actions/sceneName"), sceneName());
    settings.setValue(QStringLiteral("actions/sceneCollection"), sceneCollectionName());
}

void MainWindow::addClient(const ClientConfig &config)
{
    ClientEntry entry;
    entry.config = config;
    entry.config.name = entry.config.name.trimmed();
    if (entry.config.name.isEmpty()) {
        entry.config.name = defaultClientName(static_cast<int>(m_clients.size()));
    }

    entry.client = std::make_unique<OBSClient>();
    entry.state = QStringLiteral("Disconnected");
    entry.detail = QStringLiteral("Not connected");

    m_clients.push_back(std::move(entry));
    attachClientSignals(static_cast<int>(m_clients.size()) - 1);
}

void MainWindow::attachClientSignals(int index)
{
    OBSClient *client = m_clients[index].client.get();

    connect(client, &OBSClient::connected, this, [this, index]() {
        updateClientStatus(index,
                           QStringLiteral("Connected"),
                           QStringLiteral("Waiting for OBS authentication"));
        appendLog(QStringLiteral("%1 socket connected.")
                      .arg(clientDisplayName(index)));
    });

    connect(client, &OBSClient::disconnected, this, [this, index]() {
        updateClientStatus(index,
                           QStringLiteral("Disconnected"),
                           QStringLiteral("Not connected"));
        appendLog(QStringLiteral("%1 disconnected.")
                      .arg(clientDisplayName(index)));
    });

    connect(client, &OBSClient::authenticated, this, [this, index]() {
        updateClientStatus(index,
                           QStringLiteral("Ready"),
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
            [this, index](const QString &requestType, const QString &requestId,
                          const QJsonObject &responseData) {
        updateClientStatus(index,
                           QStringLiteral("Ready"),
                           QStringLiteral("Last action: %1").arg(requestType));
        appendLog(QStringLiteral("%1 completed %2.")
                      .arg(clientDisplayName(index), requestType));

        const auto pending = m_sceneCollectionRequests.find(requestId);
        if (pending != m_sceneCollectionRequests.end()
            && requestType == QStringLiteral("GetSceneCollectionList")) {
            QStringList sceneCollections;
            const QJsonArray names =
                responseData.value(QStringLiteral("sceneCollections")).toArray();

            for (const QJsonValue &value : names) {
                sceneCollections.append(value.toString());
            }

            populateSceneCollections(
                sceneCollections,
                responseData.value(QStringLiteral("currentSceneCollectionName"))
                    .toString());

            appendLog(QStringLiteral("Loaded %1 scene collections from %2.")
                          .arg(sceneCollections.size())
                          .arg(clientDisplayName(pending->second)));
            m_sceneCollectionRequests.erase(pending);
        }
    });

    connect(client, &OBSClient::requestFailed, this,
            [this, index](const QString &requestType, const QString &requestId,
                          const QString &comment, int code) {
        const QString detail = QStringLiteral("%1 failed (%2): %3")
                                   .arg(requestType)
                                   .arg(code)
                                   .arg(comment.isEmpty()
                                            ? QStringLiteral("No details from OBS")
                                            : comment);
        updateClientStatus(index, QStringLiteral("Request failed"), detail);
        appendLog(QStringLiteral("%1 %2")
                      .arg(clientDisplayName(index), detail));

        m_sceneCollectionRequests.erase(requestId);
    });
}

void MainWindow::updateCurrentSettingsClientFromUi()
{
    if (m_syncingUi || m_clients.empty()) {
        return;
    }

    const int index = ui->settingsClientSelector->currentIndex();
    if (index < 0 || index >= static_cast<int>(m_clients.size())) {
        return;
    }

    ClientConfig &config = m_clients[index].config;
    config.name = ui->clientNameEdit->text().trimmed();
    config.host = ui->clientHostEdit->text().trimmed();
    config.port = ui->clientPortSpinBox->value();
    config.password = ui->clientPasswordEdit->text();

    if (config.name.isEmpty()) {
        config.name = defaultClientName(index);
    }

    refreshClientSelectors();
    refreshStatusTable();
    refreshControlsPanel();
}

void MainWindow::loadClientIntoSettingsForm(int index)
{
    if (index < 0 || index >= static_cast<int>(m_clients.size())) {
        return;
    }

    m_syncingUi = true;
    ui->clientNameEdit->setText(m_clients[index].config.name);
    ui->clientHostEdit->setText(m_clients[index].config.host);
    ui->clientPortSpinBox->setValue(m_clients[index].config.port);
    ui->clientPasswordEdit->setText(m_clients[index].config.password);
    m_syncingUi = false;
}

void MainWindow::refreshClientSelectors()
{
    const int settingsIndex = ui->settingsClientSelector->currentIndex();
    const int controlsIndex = ui->controlsClientSelector->currentIndex();

    const QSignalBlocker settingsBlocker(ui->settingsClientSelector);
    const QSignalBlocker controlsBlocker(ui->controlsClientSelector);

    ui->settingsClientSelector->clear();
    ui->controlsClientSelector->clear();

    for (int index = 0; index < static_cast<int>(m_clients.size()); ++index) {
        const QString label = clientDisplayName(index);
        ui->settingsClientSelector->addItem(label);
        ui->controlsClientSelector->addItem(label);
    }

    if (!m_clients.empty()) {
        const int lastIndex = static_cast<int>(m_clients.size()) - 1;
        ui->settingsClientSelector->setCurrentIndex(qBound(0, settingsIndex, lastIndex));
        ui->controlsClientSelector->setCurrentIndex(qBound(0, controlsIndex, lastIndex));
    }

    ui->clientLimitLabel->setText(
        QStringLiteral("%1 / %2 client slots used")
            .arg(m_clients.size())
            .arg(kMaxClients));
}

void MainWindow::refreshStatusTable()
{
    ui->statusTable->setRowCount(static_cast<int>(m_clients.size()));

    for (int index = 0; index < static_cast<int>(m_clients.size()); ++index) {
        const ClientEntry &entry = m_clients[index];

        QTableWidgetItem *nameItem = new QTableWidgetItem(clientDisplayName(index));
        QTableWidgetItem *endpointItem = new QTableWidgetItem(endpointLabel(entry.config));
        QTableWidgetItem *statusItem = new QTableWidgetItem(entry.state);

        statusItem->setForeground(Qt::white);
        statusItem->setBackground(statusColorForIndex(index));

        ui->statusTable->setItem(index, 0, nameItem);
        ui->statusTable->setItem(index, 1, endpointItem);
        ui->statusTable->setItem(index, 2, statusItem);
    }

    if (ui->statusTable->rowCount() > 0 && selectedStatusIndex() < 0) {
        ui->statusTable->selectRow(0);
    }

    refreshStatusButtons();
}

void MainWindow::refreshStatusButtons()
{
    const bool hasSelection = selectedStatusIndex() >= 0;
    ui->connectSelectedStatusButton->setEnabled(hasSelection);
    ui->disconnectSelectedStatusButton->setEnabled(hasSelection);
}

void MainWindow::refreshControlsPanel()
{
    const int index = selectedControlsIndex();
    const bool hasSelection = index >= 0;

    ui->singleClientControlsGroup->setEnabled(hasSelection);

    if (!hasSelection) {
        ui->controlClientNameLabel->setText(QStringLiteral("No client selected"));
        ui->controlEndpointLabel->setText(QStringLiteral("Select a client to control."));
        ui->controlDetailLabel->clear();
        ui->controlStatusBadge->setText(QStringLiteral("Disconnected"));
        ui->controlStatusBadge->setStyleSheet(
            QStringLiteral("background: #8b2635; color: white;"));
        return;
    }

    const ClientEntry &entry = m_clients[index];
    const QColor statusColor = statusColorForIndex(index);

    ui->controlClientNameLabel->setText(clientDisplayName(index));
    ui->controlEndpointLabel->setText(endpointLabel(entry.config));
    ui->controlDetailLabel->setText(entry.detail);
    ui->controlStatusBadge->setText(entry.state);
    ui->controlStatusBadge->setStyleSheet(
        QStringLiteral("background: %1; color: white;")
            .arg(statusColor.name()));
}

void MainWindow::populateSceneCollections(const QStringList &sceneCollections,
                                          const QString &selectedValue)
{
    const QSignalBlocker blocker(ui->sceneCollectionComboBox);

    QString current = selectedValue.trimmed();
    if (current.isEmpty()) {
        current = ui->sceneCollectionComboBox->currentText().trimmed();
    }

    ui->sceneCollectionComboBox->clear();
    ui->sceneCollectionComboBox->addItems(sceneCollections);

    if (!current.isEmpty() && ui->sceneCollectionComboBox->findText(current) < 0) {
        ui->sceneCollectionComboBox->addItem(current);
    }

    if (!current.isEmpty()) {
        ui->sceneCollectionComboBox->setCurrentText(current);
    }
}

void MainWindow::updateClientStatus(int index,
                                    const QString &state,
                                    const QString &detail)
{
    if (index < 0 || index >= static_cast<int>(m_clients.size())) {
        return;
    }

    m_clients[index].state = state;
    m_clients[index].detail = detail;

    refreshStatusTable();
    refreshControlsPanel();
}

void MainWindow::broadcast(const std::function<void(int)> &operation)
{
    for (int index = 0; index < static_cast<int>(m_clients.size()); ++index) {
        operation(index);
    }
}

void MainWindow::connectClient(int index)
{
    if (index < 0 || index >= static_cast<int>(m_clients.size())) {
        return;
    }

    const ClientConfig &config = m_clients[index].config;
    const QUrl url = buildUrl(config);

    if (!url.isValid()) {
        updateClientStatus(index,
                           QStringLiteral("Error"),
                           QStringLiteral("Enter a valid OBS host or ws:// URL."));
        appendLog(QStringLiteral("%1 has invalid connection settings.")
                      .arg(clientDisplayName(index)));
        return;
    }

    m_clients[index].client->setPassword(config.password);
    updateClientStatus(index,
                       QStringLiteral("Connecting"),
                       QStringLiteral("Opening %1").arg(url.toString()));
    appendLog(QStringLiteral("Connecting %1 to %2")
                  .arg(clientDisplayName(index), url.toString()));
    m_clients[index].client->connectToObs(url);
}

void MainWindow::disconnectClient(int index)
{
    if (index < 0 || index >= static_cast<int>(m_clients.size())) {
        return;
    }

    appendLog(QStringLiteral("Disconnecting %1.").arg(clientDisplayName(index)));
    m_clients[index].client->disconnectFromObs();
}

void MainWindow::switchSceneForClient(int index)
{
    if (index < 0 || index >= static_cast<int>(m_clients.size())) {
        return;
    }

    const QString name = sceneName();
    if (name.isEmpty()) {
        appendLog(QStringLiteral("Set a scene name before switching scenes."));
        return;
    }

    m_clients[index].client->sendRequest(
        QStringLiteral("SetCurrentProgramScene"),
        QJsonObject{{QStringLiteral("sceneName"), name}}
        );
}

void MainWindow::applySceneCollectionForClient(int index)
{
    if (index < 0 || index >= static_cast<int>(m_clients.size())) {
        return;
    }

    const QString name = sceneCollectionName();
    if (name.isEmpty()) {
        appendLog(QStringLiteral("Set a scene collection name before applying it."));
        return;
    }

    m_clients[index].client->sendRequest(
        QStringLiteral("SetCurrentSceneCollection"),
        QJsonObject{{QStringLiteral("sceneCollectionName"), name}}
        );
}

void MainWindow::startStreamForClient(int index)
{
    if (index >= 0 && index < static_cast<int>(m_clients.size())) {
        m_clients[index].client->sendRequest(QStringLiteral("StartStream"));
    }
}

void MainWindow::stopStreamForClient(int index)
{
    if (index >= 0 && index < static_cast<int>(m_clients.size())) {
        m_clients[index].client->sendRequest(QStringLiteral("StopStream"));
    }
}

void MainWindow::startRecordingForClient(int index)
{
    if (index >= 0 && index < static_cast<int>(m_clients.size())) {
        m_clients[index].client->sendRequest(QStringLiteral("StartRecord"));
    }
}

void MainWindow::stopRecordingForClient(int index)
{
    if (index >= 0 && index < static_cast<int>(m_clients.size())) {
        m_clients[index].client->sendRequest(QStringLiteral("StopRecord"));
    }
}

void MainWindow::appendLog(const QString &message)
{
    ui->logOutput->appendPlainText(timestampedMessage(message));
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
    return ui->sceneCollectionComboBox->currentText().trimmed();
}

QString MainWindow::clientDisplayName(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_clients.size())) {
        return QStringLiteral("Unknown Client");
    }

    const QString name = m_clients[index].config.name.trimmed();
    return name.isEmpty() ? defaultClientName(index) : name;
}

QString MainWindow::endpointLabel(const ClientConfig &config) const
{
    const QUrl url = buildUrl(config);
    return url.isValid() ? url.toString() : QStringLiteral("No endpoint configured");
}

QColor MainWindow::statusColorForIndex(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_clients.size())) {
        return QColor(QStringLiteral("#8b2635"));
    }

    const QString state = m_clients[index].state;
    if (state == QStringLiteral("Connecting")) {
        return QColor(QStringLiteral("#b77714"));
    }

    if (m_clients[index].client->isConnected()) {
        return QColor(QStringLiteral("#1f8f52"));
    }

    return QColor(QStringLiteral("#8b2635"));
}

int MainWindow::selectedStatusIndex() const
{
    const QModelIndexList rows = ui->statusTable->selectionModel()->selectedRows();
    return rows.isEmpty() ? -1 : rows.first().row();
}

int MainWindow::selectedControlsIndex() const
{
    const int index = ui->controlsClientSelector->currentIndex();
    return index >= 0 && index < static_cast<int>(m_clients.size()) ? index : -1;
}
