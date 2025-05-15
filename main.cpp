#include <QApplication>
#include <QDebug>
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

    DatabaseManager dbManager;

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

        MainWindow w(&dbManager, loggedInUserId);
        w.show();

        return a.exec();

    } else {
        return 0;
    }
}
