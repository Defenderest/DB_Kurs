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

    // Початкове очищення мітки помилки
    ui->errorLabel->clear();

    // Встановлюємо фокус на поле email
    ui->emailLineEdit->setFocus();

    // Перевіряємо, чи передано менеджер БД
    if (!m_dbManager) {
        qCritical() << "LoginDialog: DatabaseManager is null!";
        ui->errorLabel->setText(tr("Помилка ініціалізації. Неможливо перевірити дані."));
        ui->okButton->setEnabled(false); // Вимикаємо нашу кнопку OK
    }

    // Налаштовуємо початковий режим (Вхід)
    setMode(Login); // Це також очистить m_loginAttempts

    // З'єднуємо кнопки перемикання режимів зі слотами (якщо не використовується авто-з'єднання за іменем)
    // connect(ui->switchToRegisterButton, &QPushButton::clicked, this, &LoginDialog::on_switchToRegisterButton_clicked);
    // connect(ui->switchToLoginButton, &QPushButton::clicked, this, &LoginDialog::on_switchToLoginButton_clicked);

    // З'єднуємо нашу кнопку OK зі слотом
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


// Слот для кнопки "Немає акаунту? Зареєструватися"
void LoginDialog::on_switchToRegisterButton_clicked()
{
    setMode(Register);
}

// Слот для кнопки "Вже є акаунт? Увійти"
void LoginDialog::on_switchToLoginButton_clicked()
{
    setMode(Login);
}

// Допоміжна функція для налаштування UI відповідно до режиму
void LoginDialog::setMode(Mode mode)
{
    m_currentMode = mode;
    ui->errorLabel->clear(); // Очищаємо помилки при зміні режиму
    m_loginAttempts.clear(); // Скидаємо лічильники спроб при зміні режиму
    ui->okButton->setEnabled(true); // Завжди вмикаємо кнопку при зміні режиму
    ui->emailLineEdit->setEnabled(true); // Переконуємось, що поля ввімкнені
    ui->passwordLineEdit->setEnabled(true);

    if (mode == Login) {
        // --- Режим Входу ---
        ui->titleLabel->setText(tr("Вхід до Книгарні"));
        ui->okButton->setText(tr("Увійти")); // Змінюємо текст нашої кнопки
        // Показуємо кнопку переходу до реєстрації, ховаємо кнопку переходу до входу
        ui->switchToRegisterButton->setVisible(true);
        ui->switchToLoginButton->setVisible(false);
        // Ховаємо поля реєстрації
        ui->firstNameLabel->setVisible(false);
        ui->firstNameLineEdit->setVisible(false);
        ui->lastNameLabel->setVisible(false);
        ui->lastNameLineEdit->setVisible(false);
        ui->confirmPasswordLabel->setVisible(false);
        ui->confirmPasswordLineEdit->setVisible(false);
        // Очищаємо поля реєстрації
        ui->firstNameLineEdit->clear();
        ui->lastNameLineEdit->clear();
        ui->confirmPasswordLineEdit->clear();
        // Встановлюємо фокус на email
        ui->emailLineEdit->setFocus();
        // Змінюємо розмір вікна (опціонально, може потребувати налаштування)
        // resize(width(), minimumHeight()); // Або фіксований розмір
        // adjustSize(); // Прибираємо автоматичне підлаштування розміру
    } else {
        // --- Режим Реєстрації ---
        ui->titleLabel->setText(tr("Реєстрація нового користувача"));
        ui->okButton->setText(tr("Зареєструватися")); // Змінюємо текст нашої кнопки
        // Ховаємо кнопку переходу до реєстрації, показуємо кнопку переходу до входу
        ui->switchToRegisterButton->setVisible(false);
        ui->switchToLoginButton->setVisible(true);
        // Показуємо поля реєстрації
        ui->firstNameLabel->setVisible(true);
        ui->firstNameLineEdit->setVisible(true);
        ui->lastNameLabel->setVisible(true);
        ui->lastNameLineEdit->setVisible(true);
        ui->confirmPasswordLabel->setVisible(true);
        ui->confirmPasswordLineEdit->setVisible(true);
        // Очищаємо поля входу (крім email, якщо він був введений)
        ui->passwordLineEdit->clear();
        // Встановлюємо фокус на ім'я
        ui->firstNameLineEdit->setFocus();
        // Змінюємо розмір вікна (опціонально)
        // resize(width(), preferredHeight()); // Або фіксований розмір
        // adjustSize(); // Прибираємо автоматичне підлаштування розміру
    }
}


// Обробник натискання кнопки "Увійти" або "Зареєструватися"
void LoginDialog::on_okButton_clicked()
{
    ui->errorLabel->clear(); // Очищаємо попередні помилки

    if (m_currentMode == Login) {
        // --- Логіка Входу ---
        const QString email = ui->emailLineEdit->text().trimmed();
        const QString password = ui->passwordLineEdit->text();

        if (email.isEmpty() || password.isEmpty()) {
            ui->errorLabel->setText(tr("Будь ласка, введіть email та пароль."));
            return; // Не закриваємо діалог
        }

        if (checkCredentials(email, password)) {
            qInfo() << "Login successful for user ID:" << m_loggedInCustomerId;
            m_loginAttempts.remove(email); // Скидаємо лічильник при успішному вході
            accept(); // Закриваємо діалог з результатом Accepted
        } else {
            // Невдала спроба входу
            int attempts = m_loginAttempts.value(email, 0) + 1; // Отримуємо поточну спробу + 1
            m_loginAttempts[email] = attempts; // Зберігаємо нову кількість спроб

            if (attempts >= MAX_LOGIN_ATTEMPTS) {
                ui->errorLabel->setText(tr("Забагато невдалих спроб. Спробуйте пізніше або скиньте пароль."));
                ui->okButton->setEnabled(false); // Блокуємо кнопку
                ui->emailLineEdit->setEnabled(false); // Блокуємо поля
                ui->passwordLineEdit->setEnabled(false);
            } else {
                int remaining = MAX_LOGIN_ATTEMPTS - attempts;
                ui->errorLabel->setText(tr("Невірний email або пароль. Залишилось спроб: %1").arg(remaining));
                ui->passwordLineEdit->clear(); // Очищаємо поле пароля
                ui->passwordLineEdit->setFocus();
            }
        }
    } else {
        // --- Логіка Реєстрації ---
        if (performRegistration()) {
             qInfo() << "Registration successful for user ID:" << m_loggedInCustomerId;
             // Можна автоматично залогінити користувача або просто закрити діалог
             accept(); // Закриваємо діалог з результатом Accepted
        } else {
            // Помилка вже виведена в performRegistration
            // Залишаємо діалог відкритим
        }
    }
}


// Допоміжна функція для виконання реєстрації
bool LoginDialog::performRegistration()
{
    if (!m_dbManager) {
        ui->errorLabel->setText(tr("Помилка бази даних. Реєстрація неможлива."));
        return false;
    }

    // Отримуємо дані з полів
    CustomerRegistrationInfo regInfo;
    regInfo.firstName = ui->firstNameLineEdit->text().trimmed();
    regInfo.lastName = ui->lastNameLineEdit->text().trimmed();
    regInfo.email = ui->emailLineEdit->text().trimmed();
    regInfo.password = ui->passwordLineEdit->text();
    const QString confirmPassword = ui->confirmPasswordLineEdit->text();

    // Валідація
    if (regInfo.firstName.isEmpty() || regInfo.lastName.isEmpty() || regInfo.email.isEmpty() || regInfo.password.isEmpty()) {
        ui->errorLabel->setText(tr("Будь ласка, заповніть всі поля."));
        return false;
    }
    if (regInfo.password != confirmPassword) {
        ui->errorLabel->setText(tr("Паролі не співпадають."));
        ui->confirmPasswordLineEdit->clear(); // Очистити поле підтвердження
        ui->passwordLineEdit->setFocus();     // Фокус на перше поле пароля
        ui->passwordLineEdit->selectAll();
        return false;
    }
    // TODO: Додати складнішу валідацію email та пароля за потреби

    // Викликаємо метод реєстрації в DatabaseManager
    int newId = -1;
    if (m_dbManager->registerCustomer(regInfo, newId)) {
        m_loggedInCustomerId = newId; // Зберігаємо ID нового користувача
        return true; // Успішна реєстрація
    } else {
        // Перевіряємо, чи помилка пов'язана з email
        // (DatabaseManager вже вивів попередження в консоль)
        if (m_dbManager->lastError().text().contains("customer_email_key") || m_dbManager->lastError().text().contains("duplicate key value violates unique constraint")) {
             ui->errorLabel->setText(tr("Користувач з таким email вже існує."));
             ui->emailLineEdit->setFocus();
             ui->emailLineEdit->selectAll();
        } else {
            ui->errorLabel->setText(tr("Помилка реєстрації. Спробуйте пізніше."));
        }
        return false; // Помилка реєстрації
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
