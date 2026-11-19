#pragma once

#include <QMainWindow>
#include "ui_MainWindow.h"
#include "core/RecordCore.h"
#include <memory>

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget *parent = nullptr);
	~MainWindow();

private slots:
	void on_btnStart_clicked();
	void on_btnStop_clicked();
	void on_btnSave_clicked();
	void on_btnSettings_clicked();
	void on_btnHistory_clicked();

private:
	Ui::MainWindowClass* ui;
	std::shared_ptr<ConsoleLogger> logger;
	std::unique_ptr<Application> recorder;

	void updateRecordingUI(bool isRecording);
	int bufferSec = 10;
	int fps = 30;
	QString savePath = "./";
	int hotkeyVk = VK_F10;
};

