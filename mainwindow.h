#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <functional>
#include <map>
#include <memory>
#include <vector>

#include "obsclient.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class QColor;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onSaveSettingsClicked();
    void onAddClientClicked();
    void onSettingsClientChanged(int index);
    void onControlsClientChanged(int index);
    void onRefreshSceneCollectionsClicked();
    void onStatusSelectionChanged();
    void onConnectSelectedStatusClicked();
    void onDisconnectSelectedStatusClicked();
    void onConnectSelectedControlClicked();
    void onDisconnectSelectedControlClicked();
    void onApplySceneSelectedClicked();
    void onApplySceneCollectionSelectedClicked();
    void onStartStreamSelectedClicked();
    void onStopStreamSelectedClicked();
    void onStartRecordingSelectedClicked();
    void onStopRecordingSelectedClicked();
    void onConnectAllClicked();
    void onDisconnectAllClicked();
    void onApplySceneAllClicked();
    void onApplySceneCollectionAllClicked();
    void onStartStreamAllClicked();
    void onStopStreamAllClicked();
    void onStartRecordingAllClicked();
    void onStopRecordingAllClicked();

private:
    struct ClientConfig {
        QString name;
        QString host;
        int port = 4455;
        QString password;
    };

    struct ClientEntry {
        ClientConfig config;
        std::unique_ptr<OBSClient> client;
        QString state;
        QString detail;
    };

    void setupConnections();
    void applyWindowStyle();
    void loadSettings();
    void saveSettings();
    void addClient(const ClientConfig &config);
    void attachClientSignals(int index);
    void updateCurrentSettingsClientFromUi();
    void loadClientIntoSettingsForm(int index);
    void refreshClientSelectors();
    void refreshStatusTable();
    void refreshStatusButtons();
    void refreshControlsPanel();
    void populateSceneCollections(const QStringList &sceneCollections,
                                  const QString &selectedValue);
    void updateClientStatus(int index,
                            const QString &state,
                            const QString &detail = QString());
    void broadcast(const std::function<void(int)> &operation);
    void connectClient(int index);
    void disconnectClient(int index);
    void switchSceneForClient(int index);
    void applySceneCollectionForClient(int index);
    void startStreamForClient(int index);
    void stopStreamForClient(int index);
    void startRecordingForClient(int index);
    void stopRecordingForClient(int index);
    void appendLog(const QString &message);
    QUrl buildUrl(const ClientConfig &config) const;
    QString sceneName() const;
    QString sceneCollectionName() const;
    QString clientDisplayName(int index) const;
    QString endpointLabel(const ClientConfig &config) const;
    QColor statusColorForIndex(int index) const;
    int selectedStatusIndex() const;
    int selectedControlsIndex() const;

    Ui::MainWindow *ui;
    std::vector<ClientEntry> m_clients;
    std::map<QString, int> m_sceneCollectionRequests;
    bool m_syncingUi = false;
};

#endif // MAINWINDOW_H
