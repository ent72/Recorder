#include "HistoryWindow.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QPainter>
#include <QPixmap>
#include <QImage>
#include <opencv2/opencv.hpp>

static QIcon createVideoThumbnail(const QString& videoPath, const QSize& thumbSize)
{
    QPixmap canvas(thumbSize);
    canvas.fill(QColor("#181818"));

    cv::VideoCapture cap(videoPath.toStdString());
    int durationSec = 0;

    if (cap.isOpened()) {
        double fps = cap.get(cv::CAP_PROP_FPS);
        double totalFrames = cap.get(cv::CAP_PROP_FRAME_COUNT);
        if (fps > 0.0 && totalFrames > 0.0) {
            durationSec = qRound(totalFrames / fps);
        }

        if (totalFrames > 5) {
            cap.set(cv::CAP_PROP_POS_FRAMES, totalFrames * 0.2);
        }

        cv::Mat frame;
        if (cap.read(frame) && !frame.empty()) {
            cv::cvtColor(frame, frame, cv::COLOR_BGR2RGB);
            QImage img(frame.data, frame.cols, frame.rows,
                static_cast<int>(frame.step), QImage::Format_RGB888);

            QPixmap scaled = QPixmap::fromImage(img).scaled(
                thumbSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation
            );

            int x = (scaled.width() - thumbSize.width()) / 2;
            int y = (scaled.height() - thumbSize.height()) / 2;
            canvas = scaled.copy(x, y, thumbSize.width(), thumbSize.height());
        }
        cap.release();
    }

    QPainter painter(&canvas);
    painter.setRenderHint(QPainter::Antialiasing);

    int mins = durationSec / 60;
    int secs = durationSec % 60;
    QString timeText = QString("%1:%2").arg(mins).arg(secs, 2, 10, QChar('0'));

    QFont font("Segoe UI", 9, QFont::Bold);
    painter.setFont(font);
    QFontMetrics fm(font);
    QRect textRect = fm.boundingRect(timeText).adjusted(-6, -2, 6, 2);
    textRect.moveBottomRight(QPoint(thumbSize.width() - 8, thumbSize.height() - 8));

    painter.setBrush(QColor(0, 0, 0, 200));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(textRect, 4, 4);

    painter.setPen(Qt::white);
    painter.drawText(textRect, Qt::AlignCenter, timeText);
    painter.end();

    return QIcon(canvas);
}

HistoryWindow::HistoryWindow(const QString& clipsDir, QWidget* parent)
    : QDialog(parent), ui(new Ui::HistoryWindowClass()), saveDir(clipsDir)
{
    ui->setupUi(this);

    ui->listClips->setViewMode(QListView::IconMode);
    ui->listClips->setIconSize(QSize(240, 135));
    ui->listClips->setGridSize(QSize(264, 195));
    ui->listClips->setResizeMode(QListView::Adjust);
    ui->listClips->setMovement(QListView::Static);
    ui->listClips->setWordWrap(true);
    ui->listClips->setSpacing(10);

    ui->listClips->setStyleSheet(
        "QListWidget { background-color: #0f0f0f; border: none; padding: 8px; }"
        "QListWidget::item { color: #f1f1f1; background-color: transparent; border-radius: 10px; padding: 6px; }"
        "QListWidget::item:hover { background-color: #272727; }"
        "QListWidget::item:selected { background-color: #3ea6ff; color: #0f0f0f; font-weight: bold; }"
    );

    loadClipsList();
}

HistoryWindow::~HistoryWindow()
{
    delete ui;
}

void HistoryWindow::loadClipsList()
{
    ui->listClips->clear();
    QDir dir(saveDir);

    QFileInfoList files = dir.entryInfoList(QStringList() << "*.mp4", QDir::Files, QDir::Time);

    if (files.isEmpty()) {
        ui->listClips->setViewMode(QListView::ListMode);
        ui->listClips->addItem("У цій папці ще немає збережених кліпів");
        ui->btnDelete->setEnabled(false);
        return;
    }

    ui->listClips->setViewMode(QListView::IconMode);
    ui->btnDelete->setEnabled(true);

    for (const QFileInfo& info : files) {
        double sizeMb = info.size() / (1024.0 * 1024.0);
        QString dateStr = info.lastModified().toString("dd.MM.yyyy HH:mm");
        QString title = QString("%1\n%2 • %3 МБ")
            .arg(info.fileName())
            .arg(dateStr)
            .arg(sizeMb, 0, 'f', 1);

        QListWidgetItem* item = new QListWidgetItem();
        item->setIcon(createVideoThumbnail(info.absoluteFilePath(), QSize(240, 135)));
        item->setText(title);
        item->setData(Qt::UserRole, info.fileName());
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignTop);

        ui->listClips->addItem(item);
    }
}

void HistoryWindow::on_btnDelete_clicked()
{
    QListWidgetItem* currentItem = ui->listClips->currentItem();
    if (!currentItem) {
        QMessageBox::information(this, "Увага", "Спочатку виберіть кліп зі списку.");
        return;
    }

    QString fileName = currentItem->data(Qt::UserRole).toString();
    if (fileName.isEmpty()) return;

    auto reply = QMessageBox::question(
        this,
        "Підтвердження видалення",
        "Видалити кліп \"" + fileName + "\"?",
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        QString fullPath = QDir(saveDir).filePath(fileName);
        if (QFile::remove(fullPath)) {
            loadClipsList();
        }
        else {
            QMessageBox::warning(this, "Помилка", "Не вдалося видалити файл. Перевірте, чи він не відкритий у плеєрі.");
        }
    }
}

void HistoryWindow::on_btnOpenFolder_clicked()
{
    QDir dir(saveDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(dir.absolutePath()));
}

void HistoryWindow::on_btnClose_clicked()
{
    accept();
}

void HistoryWindow::on_listClips_itemDoubleClicked(QListWidgetItem* item)
{
    QString fileName = item->data(Qt::UserRole).toString();
    if (fileName.isEmpty()) return;

    QString fullPath = QDir(saveDir).filePath(fileName);
    QDesktopServices::openUrl(QUrl::fromLocalFile(fullPath));
}