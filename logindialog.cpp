#include "logindialog.h"
#include "ui_logindialog.h"
#include <QCryptographicHash> // Для хешування пароля для перевірки
#include <QMessageBox>      // Для повідомлень про помилки
#include <QPushButton>      // Для налаштування кнопки OK

LoginDialog::LoginDialog(DatabaseManager *dbManager, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginDialog),
    m_dbManager(dbManager) // Зберігаємо вказівник
{
    ui->setupUi(this);

    // Перейменовуємо стандартну кнопку OK на "Увійти"
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Увійти"));

    // Початкове очищення мітки помилки
    ui->errorLabel->clear();

    // Встановлюємо фокус на поле email
    ui->emailLineEdit->setFocus();

    // Перевіряємо, чи передано менеджер БД
    if (!m_dbManager) {
        qCritical() << "LoginDialog: DatabaseManager is null!";
        ui->errorLabel->setText(tr("Помилка ініціалізації. Неможливо перевірити дані."));
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false); // Блокуємо кнопку входу
    }
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

int LoginDialog::getLoggedInCustomerId() const
{
    return m_loggedInCustomerId;
}

// Слот, який викликається при натисканні кнопки "Увійти" (OK)
void LoginDialog::on_buttonBox_accepted()
{
    ui->errorLabel->clear(); // Очищаємо попередні помилки
    const QString email = ui->emailLineEdit->text().trimmed();
    const QString password = ui->passwordLineEdit->text();

    if (email.isEmpty() || password.isEmpty()) {
        ui->errorLabel->setText(tr("Будь ласка, введіть email та пароль."));
        return; // Не закриваємо діалог
    }

    if (checkCredentials(email, password)) {
        // Якщо дані вірні, діалог автоматично закриється через accept()
        // Ми вже зберегли ID в m_loggedInCustomerId
        qInfo() << "Login successful for user ID:" << m_loggedInCustomerId;
        accept(); // Закриваємо діалог з результатом Accepted
    } else {
        // Якщо дані невірні, показуємо помилку і залишаємо діалог відкритим
        ui->errorLabel->setText(tr("Невірний email або пароль."));
        // Не викликаємо accept() або reject()
    }
}

// Допоміжна функція для перевірки даних
bool LoginDialog::checkCredentials(const QString &email, const QString &password)
{
    if (!m_dbManager) return false; // Перевірка наявності менеджера

    CustomerLoginInfo loginInfo = m_dbManager->getCustomerLoginInfo(email);

    if (!loginInfo.found) {
        qWarning() << "Login attempt failed: Email not found -" << email;
        return false; // Користувача не знайдено
    }

    // Хешуємо введений пароль тим же методом, що й при збереженні
    QByteArray enteredPasswordHash = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256);
    QString enteredPasswordHashHex = QString::fromUtf8(enteredPasswordHash.toHex());

    // Порівнюємо хеш введеного пароля з хешем з бази даних
    if (enteredPasswordHashHex == loginInfo.passwordHash) {
        m_loggedInCustomerId = loginInfo.customerId; // Зберігаємо ID користувача
        return true; // Пароль вірний
    } else {
        qWarning() << "Login attempt failed: Incorrect password for email -" << email;
        return false; // Пароль невірний
    }
}
