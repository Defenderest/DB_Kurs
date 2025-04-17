#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include "database.h" // Потрібно для DatabaseManager та CustomerLoginInfo

QT_BEGIN_NAMESPACE
namespace Ui { class LoginDialog; }
QT_END_NAMESPACE

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    // Конструктор приймає вказівник на DatabaseManager
    explicit LoginDialog(DatabaseManager *dbManager, QWidget *parent = nullptr);
    ~LoginDialog();

    // Метод для отримання ID користувача після успішного входу
    int getLoggedInCustomerId() const;

private slots:
    // Слот для обробки натискання кнопки OK (або Enter)
    void on_buttonBox_accepted();

private:
    Ui::LoginDialog *ui;
    DatabaseManager *m_dbManager; // Вказівник на менеджер БД (не володіє ним)
    int m_loggedInCustomerId = -1; // ID користувача, що увійшов

    // Допоміжна функція для перевірки логіну та паролю
    bool checkCredentials(const QString &email, const QString &password);
};

#endif // LOGINDIALOG_H
