#pragma once

#include <QDialog>
#include <QEvent>
#include "ui_SettingsWindow.h"

class SettingsWindow : public QDialog
{
    Q_OBJECT
public:
    SettingsWindow(int currentBufferSec, int currentFps, const QString& currentPath, int currentHotkeyVk, QWidget* parent = nullptr);
    ~SettingsWindow();

    int getBufferSec() const;
    int getFps() const;
    QString getSavePath() const;
    int getHotkeyVk() const;
protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void on_btnBrowse_clicked();
    void on_btnHotkey_clicked();
    void on_btnApply_clicked();
    void on_btnCancel_clicked();
private:
    Ui::SettingsWindowClass* ui;
    int hotkeyVk;
    bool isListening = false;
};