#pragma once

#include <QDialog>
#include <QListWidgetItem>
#include "ui_HistoryWindow.h"

class HistoryWindow : public QDialog
{
    Q_OBJECT

public:
    HistoryWindow(const QString& clipsDir, QWidget* parent = nullptr);
    ~HistoryWindow();

private slots:
    void on_btnDelete_clicked();
    void on_btnOpenFolder_clicked();
    void on_btnClose_clicked();
    void on_listClips_itemDoubleClicked(QListWidgetItem* item);

private:
    Ui::HistoryWindowClass* ui;
    QString saveDir;

    void loadClipsList();
};