#include "mainwindow.h"
#include "./ui_mainwindow.h" // Keep UI include first if it depends on Qt headers
#include "database.h"      // Include DatabaseManager implementation
#include <QStatusBar>      // For showing status messages
#include <QMessageBox>     // For showing error dialogs (optional)
#include <QDebug>          // For logging

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 1. Создаем экземпляр DatabaseManager
    m_dbManager = new DatabaseManager(this); // 'this' устанавливает родителя для управления памятью

    // 2. Подключаемся к базе данных (замените на ваши реальные данные)
    bool connected = m_dbManager->connectToDatabase(
        "127.127.126.49",      // Хост
        5432,             // Порт
        "postgres",   // Имя базы данных
        "postgres",       // Имя пользователя
        "1234"            // Пароль (ВНИМАНИЕ: Не храните пароли в коде в реальных приложениях!)
    );

    // 3. Отображаем статус подключения в строке состояния
    if (connected) {
        ui->statusBar->showMessage(tr("Успішно підключено до бази даних."), 5000); // Сообщение на 5 секунд
        qInfo() << "Database connection successful.";

        // --- ВРЕМЕННО: Создаем схему и заполняем данными при запуске ---
        // В реальном приложении это должно быть по кнопке или при первом запуске
        if (!m_dbManager->createSchemaTables()) {
             QMessageBox::critical(this, tr("Помилка створення схеми"),
                                  tr("Не вдалося створити таблиці бази даних.\nДивіться логи для деталей."));
             ui->statusBar->showMessage(tr("Помилка створення схеми БД!"), 0); // Постоянное сообщение
        } else {
             ui->statusBar->showMessage(tr("Схема БД успішно створена/оновлена."), 5000);
             // Заполняем тестовыми данными (например, 30 записей)
             if (!m_dbManager->populateTestData(30)) {
                 QMessageBox::warning(this, tr("Помилка заповнення даних"),
                                      tr("Не вдалося заповнити таблиці тестовими даними.\nДивіться логи для деталей."));
                 ui->statusBar->showMessage(tr("Помилка заповнення БД тестовими даними!"), 0);
             } else {
                 ui->statusBar->showMessage(tr("Тестові дані успішно додані."), 5000);
                 // Опционально: вывести все данные в консоль для проверки
                 // m_dbManager->printAllData();
             }
        }
        // --- КОНЕЦ ВРЕМЕННОГО БЛОКА ---

    } else {
        ui->statusBar->showMessage(tr("Помилка підключення до бази даних!"), 0); // Постоянное сообщение об ошибке
        QMessageBox::critical(this, tr("Помилка підключення"),
                              tr("Не вдалося підключитися до бази даних.\nПеревірте налаштування та логи.\nДодаток може працювати некоректно."));
        qCritical() << "Database connection failed:" << m_dbManager->lastError().text();
    }

}

MainWindow::~MainWindow()
{
    // DatabaseManager будет удален автоматически благодаря установке родителя (this)
    // Но соединение лучше закрыть явно, если оно еще открыто
    if (m_dbManager) {
        m_dbManager->closeConnection();
        // delete m_dbManager; // Не нужно, если 'this' был родителем
    }
    delete ui;
}
