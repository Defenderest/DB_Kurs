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

// Функція setupBannerImage() була видалена, оскільки її замінено на setupAutoBanner() в mainwindow.cpp
