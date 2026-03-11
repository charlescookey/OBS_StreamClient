#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //client.setPassword("qpghBiDZqyQYZGk8");
    client.setPassword("cCZOwoZrTMRGrzYl");

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{

    //client.connectToObs(QUrl("ws://192.168.1.124:4455"));
    //client.connectToObs(QUrl("ws://172.24.31.113:4455"));
    client.connectToObs(QUrl("ws://aru@10.255.32.29:4455"));


    QObject::connect(&client, &OBSClient::authenticated, [&]() {
        qDebug() << "Ready to send OBS commands!";

        // Example: change scene
        client.sendRequest(
            "SetCurrentProgramScene",
            QJsonObject{{"sceneName", "me_pic"}},
            "scene-change-1"
            );
    });
}


void MainWindow::on_pushButton_2_clicked()
{
    if(clients.contains("Me")){

    }else{
        OBSClient *new_client =new OBSClient();
        clients["Me"] = new_client;
        clients["Me"]->setPassword("qpghBiDZqyQYZGk8");
        clients["Me"]->connectToObs(QUrl("ws://172.24.31.113:4455"));
    }
}


void MainWindow::on_pushButton_swicth_clicked()
{
    for (auto client : clients) {
        client->sendRequest(
            "SetCurrentProgramScene",
            QJsonObject{{"sceneName", "Scene"}},
            "scene-change-1"
            );
    }

}


void MainWindow::on_pushButton_Aru_clicked()
{
    if(clients.contains("Aru")){

    }else{
        OBSClient *new_client =new OBSClient();
        clients["Aru"] = new_client;
        clients["Aru"]->setPassword("cCZOwoZrTMRGrzYl");
        clients["Aru"]->connectToObs(QUrl("ws://aru@10.255.32.29:4455"));
    }

}


void MainWindow::on_pushButton_3_clicked()
{
    for (auto client : clients) {
        client->sendRequest(
            "StartStream",
            QJsonObject{},
            "start-stream-1"
            );
    }

}

