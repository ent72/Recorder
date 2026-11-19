#include "SettingsWindow.h"
#include <QFileDialog>
#include <QKeyEvent>
#include <windows.h>

static QString vkToString(int vk)
{
    UINT scanCode = MapVirtualKeyW(static_cast<UINT>(vk), MAPVK_VK_TO_VSC);
    switch (vk) {
    case VK_LEFT: case VK_UP: case VK_RIGHT: case VK_DOWN:
    case VK_PRIOR: case VK_NEXT: case VK_END: case VK_HOME:
    case VK_INSERT: case VK_DELETE: case VK_DIVIDE: case VK_NUMLOCK:
        scanCode |= 0x100;
        break;
    }

    wchar_t keyName[64] = { 0 };
    if (GetKeyNameTextW(static_cast<LONG>(scanCode << 16), keyName, 64) > 0) {
        return QString::fromWCharArray(keyName);
    }

    return QString("Key (0x%1)").arg(vk, 0, 16).toUpper();
}

SettingsWindow::SettingsWindow(int currentBufferSec, int currentFps, const QString& currentPath, int currentHotkeyVk, QWidget* parent)
    : QDialog(parent), ui(new Ui::SettingsWindowClass()), hotkeyVk(currentHotkeyVk)
{
    ui->setupUi(this);

    ui->spinBufferSec->setValue(currentBufferSec);
    ui->cmbFps->setCurrentText(QString::number(currentFps));
    ui->txtSavePath->setText(currentPath);
    ui->btnHotkey->setText(vkToString(hotkeyVk));
    ui->btnHotkey->installEventFilter(this);
}

SettingsWindow::~SettingsWindow()
{
    delete ui;
}

int SettingsWindow::getBufferSec() const
{
    return ui->spinBufferSec->value();
}
int SettingsWindow::getFps() const
{
    return ui->cmbFps->currentText().toInt();
}
QString SettingsWindow::getSavePath() const
{
    return ui->txtSavePath->text();
}
int SettingsWindow::getHotkeyVk() const
{
    return hotkeyVk;
}

void SettingsWindow::on_btnHotkey_clicked()
{
    isListening = true;
    ui->btnHotkey->setText("Натисніть клавішу...");
    ui->btnHotkey->setStyleSheet("background-color: #1976d2; color: white; font-weight: bold;");
    ui->btnHotkey->setFocus();
}

bool SettingsWindow::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == ui->btnHotkey && isListening) {
        if (event->type() == QEvent::KeyPress || event->type() == QEvent::ShortcutOverride) {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

            if (event->type() == QEvent::KeyPress && !keyEvent->isAutoRepeat()) {
                int vk = static_cast<int>(keyEvent->nativeVirtualKey());
                if (vk != 0) {
                    hotkeyVk = vk;
                    isListening = false;
                    ui->btnHotkey->setText(vkToString(hotkeyVk));
                    ui->btnHotkey->setStyleSheet("");
                }
            }
            return true;
        }
        if (event->type() == QEvent::FocusOut) {
            isListening = false;
            ui->btnHotkey->setText(vkToString(hotkeyVk));
            ui->btnHotkey->setStyleSheet("");
        }
    }

    return QDialog::eventFilter(obj, event);
}
void SettingsWindow::on_btnBrowse_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(
        this,
        "Оберіть папку для збереження кліпів",
        ui->txtSavePath->text()
    );
    if (!dir.isEmpty()) {
        ui->txtSavePath->setText(dir);
    }
}
void SettingsWindow::on_btnApply_clicked()
{
    accept();
}
void SettingsWindow::on_btnCancel_clicked()
{
    reject();
}