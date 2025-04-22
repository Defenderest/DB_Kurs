#include "mainwindow.h"
#include "./ui_mainwindow.h" // Для доступу до ui->bannerLabel
#include <QLayout>
#include <QWidget>
#include <QPixmap>
#include <QDir>
#include <QCoreApplication>
#include <QDebug>
#include <QLabel> // Для QLabel у setupBannerImage

// Метод для очищення Layout
void MainWindow::clearLayout(QLayout* layout) {
    if (!layout) return;
    QLayoutItem* item;
    while ((item = layout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            delete item->widget(); // Видаляємо віджет
        }
        delete item; // Видаляємо елемент layout
    }
}

// Метод для програмного встановлення банера
void MainWindow::setupBannerImage()
{
    if (!ui->bannerLabel) {
        qWarning() << "setupBannerImage: bannerLabel is null!";
        return;
    }

    // Будуємо шлях відносно директорії виконуваного файлу
    QString appDir = QCoreApplication::applicationDirPath();
    QString imagePath = QDir(appDir).filePath("images/banner.jpg"); // Правильний шлях до images/banner.jpg
    qInfo() << "Attempting to load banner from path:" << imagePath;
    qInfo() << "Banner label current size:" << ui->bannerLabel->size(); // Розмір віджета на момент виклику

    QPixmap bannerPixmap(imagePath);

    if (bannerPixmap.isNull()) {
        qWarning() << "Failed to load banner image. Check if the file exists at the specified path and is readable.";
        // Встановлюємо запасний варіант (наприклад, колір фону)
        ui->bannerLabel->setStyleSheet(ui->bannerLabel->styleSheet() + " background-color: #e9ecef;"); // Додаємо сірий фон
        ui->bannerLabel->setText(tr("Не вдалося завантажити банер\n(%1)").arg(imagePath)); // Показуємо шлях у повідомленні
    } else {
        qInfo() << "Banner image loaded successfully. Original size:" << bannerPixmap.size();
        // Перевіряємо розмір віджета ще раз, можливо він змінився
        QSize labelSize = ui->bannerLabel->size();
        if (!labelSize.isValid() || labelSize.width() <= 0 || labelSize.height() <= 0) {
            qWarning() << "Banner label size is invalid or zero:" << labelSize << ". Using minimum size hint:" << ui->bannerLabel->minimumSizeHint();
            labelSize = ui->bannerLabel->minimumSizeHint(); // Спробуємо використати minimumSizeHint
            if (!labelSize.isValid() || labelSize.width() <= 0 || labelSize.height() <= 0) {
                 qWarning() << "Minimum size hint is also invalid. Cannot scale pixmap correctly.";
                 ui->bannerLabel->setText(tr("Помилка розміру віджета банера"));
                 return;
            }
        }

        // Масштабуємо зображення, щоб воно заповнило QLabel, зберігаючи пропорції та обрізаючи зайве (як background-size: cover)
        QPixmap scaledPixmap = bannerPixmap.scaled(labelSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        qInfo() << "Pixmap scaled to (at least):" << scaledPixmap.size() << "based on label size:" << labelSize;

        // Обрізаємо зображення до розміру QLabel, якщо воно більше
        if (scaledPixmap.width() > labelSize.width() || scaledPixmap.height() > labelSize.height()) {
             scaledPixmap = scaledPixmap.copy(
                 (scaledPixmap.width() - labelSize.width()) / 2,
                 (scaledPixmap.height() - labelSize.height()) / 2,
                 labelSize.width(),
                 labelSize.height()
             );
        }
        ui->bannerLabel->setPixmap(scaledPixmap);
        // Переконуємось, що текст видно (стилі кольору/тіні залишаються з UI)
    }
}
