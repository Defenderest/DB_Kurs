#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QMap>
#include "database.h"

QT_BEGIN_NAMESPACE
namespace Ui { class LoginDialog; }
QT_END_NAMESPACE

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(DatabaseManager *dbManager, QWidget *parent = nullptr);
    ~LoginDialog();

    int getLoggedInCustomerId() const;

private slots:
    void on_switchToRegisterButton_clicked();
    void on_switchToLoginButton_clicked();
    void on_okButton_clicked();

private:
    enum Mode {
        Login,
        Register
    };

    Ui::LoginDialog *ui;
    DatabaseManager *m_dbManager;
    int m_loggedInCustomerId = -1;
    Mode m_currentMode = Login;
    QMap<QString, int> m_loginAttempts;
    static const int MAX_LOGIN_ATTEMPTS = 5;

    bool checkCredentials(const QString &email, const QString &password);
    bool performRegistration();
    void setMode(Mode mode);
};

#endif // LOGINDIALOG_H
