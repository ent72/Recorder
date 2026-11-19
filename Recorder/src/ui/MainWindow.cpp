#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QTimer>
#include "SettingsWindow.h"
#include "HistoryWindow.h"
#include <QSettings>
#include <QStandardPaths>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindowClass())
{
    ui->setupUi(this);
    QSettings settings("StudentLab", "ScreenRecorder");
    QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation) + "/Clips";

    bufferSec = settings.value("bufferSec", 10).toInt();
    fps = settings.value("fps", 30).toInt();
    savePath = settings.value("savePath", defaultDir).toString();

    logger = std::make_shared<ConsoleLogger>();
    recorder = std::make_unique<Application>(logger);
    recorder->initialize();
    recorder->updateConfig(fps, bufferSec, savePath.toStdString());
    updateRecordingUI(false);
}

MainWindow::~MainWindow()
{
    if (recorder) {
        recorder->stopRecording();
    }
    delete ui;
}

void MainWindow::updateRecordingUI(bool isRecording)
{
    if (isRecording) {
        ui->lblStatus->setText("● Іде запис");
        ui->lblStatus->setStyleSheet("color: #e53935; font-weight: bold; font-size: 14px;");

        ui->btnStart->setEnabled(false);
        ui->btnStop->setEnabled(true);
        ui->btnSave->setEnabled(true);
    }
    else {
        ui->lblStatus->setText("■ Зупинено");
        ui->lblStatus->setStyleSheet("color: #808080; font-weight: bold; font-size: 14px;");

        ui->btnStart->setEnabled(true);
        ui->btnStop->setEnabled(false);
        ui->btnSave->setEnabled(false);
    }
}

void MainWindow::on_btnStart_clicked()
{
    recorder->startRecording();
    updateRecordingUI(true);
    logger->logInfo("UI: Recording Started");
}

void MainWindow::on_btnStop_clicked()
{
    recorder->stopRecording();
    updateRecordingUI(false);
    logger->logInfo("UI: Recording Stopped");
}

void MainWindow::on_btnSave_clicked()
{
    recorder->saveHighlight("UI Button Clicked");
    ui->lblStatus->setText("✔ Кліп збережено!");
    ui->lblStatus->setStyleSheet("color: #43a047; font-weight: bold; font-size: 14px;");
    QTimer::singleShot(2500, this, [this]() {
        if (ui->btnStop->isEnabled()) {
            updateRecordingUI(true);
        }
        });
}
void MainWindow::on_btnSettings_clicked()
{
    SettingsWindow dlg(bufferSec, fps, savePath, hotkeyVk, this);
    if (dlg.exec() == QDialog::Accepted) {
        bufferSec = dlg.getBufferSec();
        fps = dlg.getFps();
        savePath = dlg.getSavePath();
        hotkeyVk = dlg.getHotkeyVk();

        recorder->updateConfig(fps, bufferSec, savePath.toStdString(), hotkeyVk);
    }
}
void MainWindow::on_btnHistory_clicked()
{
    HistoryWindow dlg(savePath, this);
    dlg.exec();
}