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
    // Слот для обробки натискання кнопки OK/Зареєструватися
    void on_buttonBox_accepted();
    // Слоти для кнопок перемикання режимів
    void on_switchToRegisterButton_clicked();
    void on_switchToLoginButton_clicked();

private:
    // Режими діалогу
    enum Mode {
        Login,
        Register
    };

    Ui::LoginDialog *ui;
    DatabaseManager *m_dbManager; // Вказівник на менеджер БД (не володіє ним)
    int m_loggedInCustomerId = -1; // ID користувача, що увійшов або зареєструвався
    Mode m_currentMode = Login;   // Поточний режим діалогу

    // Допоміжна функція для перевірки логіну та паролю
    bool checkCredentials(const QString &email, const QString &password);
    // Допоміжна функція для виконання реєстрації
    bool performRegistration();
    // Допоміжна функція для налаштування UI відповідно до режиму
    void setMode(Mode mode);
};

#endif // LOGINDIALOG_H
