#include <QApplication>
#include <QDebug>
#include <QMessageBox> // Для повідомлень про помилки підключення БД
#include "mainwindow.h"
#include "logindialog.h" // Додано для діалогу входу
#include "database.h"    // Додано для DatabaseManager

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QApplication::setApplicationName("Bookstore");
    QApplication::setOrganizationName("YourCompany");
    QApplication::setApplicationVersion("1.0");

    qInfo() << "Starting the Bookstore application...";

    // 1. Створюємо менеджер БД (один на весь додаток)
    DatabaseManager dbManager; // Створюємо на стеку, буде жити до кінця main

    // 2. Підключаємося до БД
    bool connected = dbManager.connectToDatabase(
        "127.127.126.49",      // Хост
        5432,             // Порт
        "postgres",       // Імя бази даних (перевірте, чи правильне)
        "postgres",       // Імя користувача
        "1234"            // Пароль
    );

    if (!connected) {
        QMessageBox::critical(nullptr, QObject::tr("Помилка підключення до БД"),
                              QObject::tr("Не вдалося підключитися до бази даних.\nДодаток не може продовжити роботу.\n") + dbManager.lastError().text());
        qCritical() << "Database connection failed. Application cannot start.";
        return 1; // Вихід з помилкою
    }
    qInfo() << "Database connection successful.";

    // // --- ТИМЧАСОВО: Створення/Заповнення БД (якщо потрібно при кожному запуску для тестування) ---
    // // Цей блок можна залишити тут або перенести в окрему логіку ініціалізації
    // if (!dbManager.createSchemaTables()) {
    //     QMessageBox::critical(nullptr, QObject::tr("Помилка створення схеми"),
    //                           QObject::tr("Не вдалося створити таблиці бази даних.\nДивіться логи для деталей."));
    //     // Можливо, варто вийти, якщо схема критична
    //     // return 1;
    // } else {
    //     if (!dbManager.populateTestData(30)) { // Заповнюємо даними
    //          QMessageBox::warning(nullptr, QObject::tr("Помилка заповнення даних"),
    //                               QObject::tr("Не вдалося заповнити таблиці тестовими даними.\nДивіться логи для деталей."));
    //     }
    // }
    // // --- КІНЕЦЬ ТИМЧАСОВОГО БЛОКУ ---


    // 3. Створюємо та показуємо діалог входу, передаючи менеджер БД
    LoginDialog loginDialog(&dbManager); // Передаємо вказівник на dbManager
    int loggedInUserId = -1;

    // Запускаємо діалог модально
    if (loginDialog.exec() == QDialog::Accepted) {
        // Якщо вхід успішний, отримуємо ID користувача
        loggedInUserId = loginDialog.getLoggedInCustomerId();
        qInfo() << "Login successful. User ID:" << loggedInUserId;

        // Перевіряємо, чи отримали валідний ID
        if (loggedInUserId <= 0) {
             QMessageBox::critical(nullptr, QObject::tr("Помилка входу"),
                                   QObject::tr("Не вдалося отримати ідентифікатор користувача після входу."));
             qCritical() << "Failed to retrieve valid user ID after login.";
             return 1; // Вихід з помилкою
        }

        // 4. Створюємо головне вікно, передаючи менеджер БД та ID користувача
        MainWindow w(&dbManager, loggedInUserId); // Передаємо вказівник та ID
        w.show();

        qInfo() << "Main window shown. Entering event loop...";
        // 5. Запускаємо цикл обробки подій
        return a.exec();

    } else {
        // Якщо користувач натиснув Cancel або закрив діалог
        qInfo() << "Login cancelled or failed. Exiting application.";
        // dbManager автоматично очиститься при виході з main
        return 0; // Нормальний вихід без показу головного вікна
    }
}
