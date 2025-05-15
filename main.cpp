#include <QApplication>
#include <QDebug> // Залишаємо для qWarning/qCritical
#include <QMessageBox>
#include "mainwindow.h"
#include "logindialog.h"
#include "database.h"
#include "testdata.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon("D:/projects/DB_Kurs/QtAPP/untitled/icons/app_icon.png"));
    QApplication::setApplicationName("Bookstore");
    QApplication::setOrganizationName("Patsera_Ihor");
    QApplication::setApplicationVersion("1.0");

    // 1. Створюємо менеджер БД
    DatabaseManager dbManager;

    // 2. Підключаємося до БД
    bool connected = dbManager.connectToDatabase(
        "127.127.126.49",
        5432,
        "postgres",
        "postgres",
        "1234"
    );

    if (!connected) {
        QMessageBox::critical(nullptr, QObject::tr("Помилка підключення до БД"),
                              QObject::tr("Не вдалося підключитися до бази даних.\nДодаток не може продовжити роботу.\n") + dbManager.lastError().text());
        qCritical() << "Database connection failed. Application cannot start.";
        return 1;
    }

    // --- Опціонально: Створення/Заповнення БД при першому запуску або для тестування ---
    // if (!dbManager.createSchemaTables()) {
    //     QMessageBox::critical(nullptr, QObject::tr("Помилка створення схеми"),
    //                           QObject::tr("Не вдалося створити таблиці бази даних.\nДивіться логи для деталей."));
    //     // return 1;
    // } else {
    //     if (!populateTestData(&dbManager, 30)) {
    //          QMessageBox::warning(nullptr, QObject::tr("Помилка заповнення даних"),
    //                               QObject::tr("Не вдалося заповнити таблиці тестовими даними.\nДивіться логи для деталей."));
    //     }
    // }
    // --- Кінець опціонального блоку ---


    // 3. Створюємо та показуємо діалог входу
    LoginDialog loginDialog(&dbManager);
    int loggedInUserId = -1;

    if (loginDialog.exec() == QDialog::Accepted) {
        loggedInUserId = loginDialog.getLoggedInCustomerId();

        if (loggedInUserId <= 0) {
             QMessageBox::critical(nullptr, QObject::tr("Помилка входу"),
                                   QObject::tr("Не вдалося отримати ідентифікатор користувача після входу."));
             qCritical() << "Failed to retrieve valid user ID after login.";
             return 1;
        }

        // 4. Створюємо головне вікно
        MainWindow w(&dbManager, loggedInUserId);
        w.show();

        // 5. Запускаємо цикл обробки подій
        return a.exec();

    } else {
        // Користувач скасував вхід або закрив діалог
        return 0; // Нормальний вихід
    }
}
