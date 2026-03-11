#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <array>
#include <functional>
#include <memory>

#include "obsclient.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class QLabel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onSaveSettingsClicked();
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

    void setupConnections();
    void applyWindowStyle();
    void loadSettings();
    void saveSettings();
    ClientConfig readClientConfigFromUi(int index) const;
    void writeClientConfigToUi(int index, const ClientConfig &config);
    QUrl buildUrl(const ClientConfig &config) const;
    QString sceneName() const;
    QString sceneCollectionName() const;
    QString clientDisplayName(int index) const;
    void connectClient(int index);
    void disconnectClient(int index);
    void switchSceneForClient(int index);
    void applySceneCollectionForClient(int index);
    void startStreamForClient(int index);
    void stopStreamForClient(int index);
    void startRecordingForClient(int index);
    void stopRecordingForClient(int index);
    void broadcast(const std::function<void(int)> &operation);
    void updateClientStatus(int index,
                            const QString &state,
                            const QString &detail = QString());
    void appendLog(const QString &message);

    Ui::MainWindow *ui;
    std::array<std::unique_ptr<OBSClient>, 2> m_clients;
};

#endif // MAINWINDOW_H
