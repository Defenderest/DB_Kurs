#include "logindialog.h"
#include "ui_logindialog.h"
#include <QCryptographicHash>
#include <QMessageBox>
#include <QPushButton>

LoginDialog::LoginDialog(DatabaseManager *dbManager, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginDialog),
    m_dbManager(dbManager)
{
    ui->setupUi(this);

    ui->errorLabel->clear();

    ui->emailLineEdit->setFocus();

    if (!m_dbManager) {
        qCritical() << "LoginDialog: DatabaseManager is null!";
        ui->errorLabel->setText(tr("Помилка ініціалізації. Неможливо перевірити дані."));
        ui->okButton->setEnabled(false);
    }

    setMode(Login);

    connect(ui->okButton, &QPushButton::clicked, this, &LoginDialog::on_okButton_clicked);
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

int LoginDialog::getLoggedInCustomerId() const
{
    return m_loggedInCustomerId;
}


void LoginDialog::on_switchToRegisterButton_clicked()
{
    setMode(Register);
}

void LoginDialog::on_switchToLoginButton_clicked()
{
    setMode(Login);
}

void LoginDialog::setMode(Mode mode)
{
    m_currentMode = mode;
    ui->errorLabel->clear();
    m_loginAttempts.clear();
    ui->okButton->setEnabled(true);
    ui->emailLineEdit->setEnabled(true);
    ui->passwordLineEdit->setEnabled(true);

    if (mode == Login) {
        ui->titleLabel->setText(tr("Вхід до Книгарні"));
        ui->okButton->setText(tr("Увійти"));
        ui->switchToRegisterButton->setVisible(true);
        ui->switchToLoginButton->setVisible(false);
        ui->firstNameLabel->setVisible(false);
        ui->firstNameLineEdit->setVisible(false);
        ui->lastNameLabel->setVisible(false);
        ui->lastNameLineEdit->setVisible(false);
        ui->confirmPasswordLabel->setVisible(false);
        ui->confirmPasswordLineEdit->setVisible(false);
        ui->firstNameLineEdit->clear();
        ui->lastNameLineEdit->clear();
        ui->confirmPasswordLineEdit->clear();
        ui->emailLineEdit->setFocus();
    } else {
        ui->titleLabel->setText(tr("Реєстрація нового користувача"));
        ui->okButton->setText(tr("Зареєструватися"));
        ui->switchToRegisterButton->setVisible(false);
        ui->switchToLoginButton->setVisible(true);
        ui->firstNameLabel->setVisible(true);
        ui->firstNameLineEdit->setVisible(true);
        ui->lastNameLabel->setVisible(true);
        ui->lastNameLineEdit->setVisible(true);
        ui->confirmPasswordLabel->setVisible(true);
        ui->confirmPasswordLineEdit->setVisible(true);
        ui->passwordLineEdit->clear();
        ui->firstNameLineEdit->setFocus();
    }
}


void LoginDialog::on_okButton_clicked()
{
    ui->errorLabel->clear();

    if (m_currentMode == Login) {
        const QString email = ui->emailLineEdit->text().trimmed();
        const QString password = ui->passwordLineEdit->text();

        if (email.isEmpty() || password.isEmpty()) {
            ui->errorLabel->setText(tr("Будь ласка, введіть email та пароль."));
            return;
        }

        if (checkCredentials(email, password)) {
            qInfo() << "Login successful for user ID:" << m_loggedInCustomerId;
            m_loginAttempts.remove(email);
            accept();
        } else {
            int attempts = m_loginAttempts.value(email, 0) + 1;
            m_loginAttempts[email] = attempts;

            if (attempts >= MAX_LOGIN_ATTEMPTS) {
                ui->errorLabel->setText(tr("Забагато невдалих спроб. Спробуйте пізніше або скиньте пароль."));
                ui->okButton->setEnabled(false);
                ui->emailLineEdit->setEnabled(false);
                ui->passwordLineEdit->setEnabled(false);
            } else {
                int remaining = MAX_LOGIN_ATTEMPTS - attempts;
                ui->errorLabel->setText(tr("Невірний email або пароль. Залишилось спроб: %1").arg(remaining));
                ui->passwordLineEdit->clear();
                ui->passwordLineEdit->setFocus();
            }
        }
    } else {
        if (performRegistration()) {
             qInfo() << "Registration successful for user ID:" << m_loggedInCustomerId;
             accept();
        } else {
        }
    }
}


bool LoginDialog::performRegistration()
{
    if (!m_dbManager) {
        ui->errorLabel->setText(tr("Помилка бази даних. Реєстрація неможлива."));
        return false;
    }

    CustomerRegistrationInfo regInfo;
    regInfo.firstName = ui->firstNameLineEdit->text().trimmed();
    regInfo.lastName = ui->lastNameLineEdit->text().trimmed();
    regInfo.email = ui->emailLineEdit->text().trimmed();
    regInfo.password = ui->passwordLineEdit->text();
    const QString confirmPassword = ui->confirmPasswordLineEdit->text();

    if (regInfo.firstName.isEmpty() || regInfo.lastName.isEmpty() || regInfo.email.isEmpty() || regInfo.password.isEmpty()) {
        ui->errorLabel->setText(tr("Будь ласка, заповніть всі поля."));
        return false;
    }
    if (regInfo.password != confirmPassword) {
        ui->errorLabel->setText(tr("Паролі не співпадають."));
        ui->confirmPasswordLineEdit->clear();
        ui->passwordLineEdit->setFocus();
        ui->passwordLineEdit->selectAll();
        return false;
    }

    int newId = -1;
    if (m_dbManager->registerCustomer(regInfo, newId)) {
        m_loggedInCustomerId = newId;
        return true;
    } else {
        if (m_dbManager->lastError().text().contains("customer_email_key") || m_dbManager->lastError().text().contains("duplicate key value violates unique constraint")) {
             ui->errorLabel->setText(tr("Користувач з таким email вже існує."));
             ui->emailLineEdit->setFocus();
             ui->emailLineEdit->selectAll();
        } else {
            ui->errorLabel->setText(tr("Помилка реєстрації. Спробуйте пізніше."));
        }
        return false;
    }
}

bool LoginDialog::checkCredentials(const QString &email, const QString &password)
{
    if (!m_dbManager) return false;

    CustomerLoginInfo loginInfo = m_dbManager->getCustomerLoginInfo(email);

    if (!loginInfo.found) {
        qWarning() << "Login attempt failed: Email not found -" << email;
        return false;
    }

    QByteArray enteredPasswordHash = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256);
    QString enteredPasswordHashHex = QString::fromUtf8(enteredPasswordHash.toHex());

    if (enteredPasswordHashHex == loginInfo.passwordHash) {
        m_loggedInCustomerId = loginInfo.customerId;
        return true;
    } else {
        qWarning() << "Login attempt failed: Incorrect password for email -" << email;
        return false;
    }
}
