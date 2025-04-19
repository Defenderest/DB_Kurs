#include "profiledialog.h"
#include "ui_profiledialog.h"
#include "database.h" // Включаємо для доступу до DatabaseManager та структур
#include <QDate> // Для форматування дати
#include <QDialogButtonBox> // Потрібно для доступу до buttonBox
#include <QPushButton>      // Додано для QPushButton
#include <QLineEdit>        // Потрібно для доступу до profilePhoneLineEdit
#include <QMessageBox>      // Для показу повідомлень
#include <QDebug>           // Для логування

// Оновлений конструктор
ProfileDialog::ProfileDialog(DatabaseManager *dbManager, int customerId, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ProfileDialog),
    m_dbManager(dbManager),
    m_customerId(customerId)
{
    ui->setupUi(this);
    setWindowTitle(tr("Редагування профілю")); // Змінюємо заголовок

    // Перевірка вхідних даних
    if (!m_dbManager) {
        qCritical() << "ProfileDialog: DatabaseManager is null!";
        QMessageBox::critical(this, tr("Критична помилка"), tr("Менеджер бази даних не ініціалізовано."));
        // Блокуємо поля або закриваємо діалог
        ui->buttonBox->button(QDialogButtonBox::Save)->setEnabled(false);
        // Можна відхилити діалог одразу: QTimer::singleShot(0, this, &ProfileDialog::reject);
        return;
    }
    if (m_customerId <= 0) {
        qWarning() << "ProfileDialog: Invalid customer ID:" << m_customerId;
        QMessageBox::warning(this, tr("Помилка"), tr("Не вдалося визначити користувача для редагування профілю."));
        ui->buttonBox->button(QDialogButtonBox::Save)->setEnabled(false);
        return;
    }

    // Завантажуємо та заповнюємо дані
    populateProfileData();

    // Підключаємо сигнал accepted від buttonBox до нашого слота on_buttonBox_accepted
    // Стандартні reject() для Cancel та accept() для Save вже підключені в .ui,
    // але нам потрібна своя логіка перед accept().
    // Відключаємо стандартний accept, щоб він не закривав вікно передчасно.
    disconnect(ui->buttonBox, &QDialogButtonBox::accepted, this, &ProfileDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &ProfileDialog::on_buttonBox_accepted);
    // reject() для Cancel залишаємо стандартним.
}

ProfileDialog::~ProfileDialog()
{
    delete ui;
}

// Приватний метод для заповнення полів
void ProfileDialog::populateProfileData()
{
    CustomerProfileInfo profileInfo = m_dbManager->getCustomerProfileInfo(m_customerId);

    // Перевіряємо, чи дані взагалі були знайдені
    if (!profileInfo.found) {
        QMessageBox::warning(this, tr("Помилка завантаження"), tr("Не вдалося завантажити дані профілю."));
        // Заповнюємо поля текстом про помилку (використовуємо нові LineEdit)
        const QString errorText = tr("(Помилка завантаження)");
        ui->firstNameLineEdit->setText("");
        ui->firstNameLineEdit->setPlaceholderText(errorText);
        ui->firstNameLineEdit->setEnabled(false);
        ui->lastNameLineEdit->setText("");
        ui->lastNameLineEdit->setPlaceholderText(errorText);
        ui->lastNameLineEdit->setEnabled(false);
        ui->emailLabel->setText(errorText); // Email залишається QLabel
        ui->phoneLineEdit->setText("");
        ui->phoneLineEdit->setPlaceholderText(errorText);
        ui->phoneLineEdit->setEnabled(false);
        ui->addressLineEdit->setText("");
        ui->addressLineEdit->setPlaceholderText(errorText);
        ui->addressLineEdit->setEnabled(false);
        ui->joinDateLabel->setText(errorText);
        ui->loyaltyLabel->setText(errorText);
        ui->pointsLabel->setText("-");
        ui->buttonBox->button(QDialogButtonBox::Save)->setEnabled(false); // Блокуємо кнопку збереження
        return;
    }

    // Заповнюємо поля, використовуючи імена віджетів з profiledialog.ui (використовуємо нові LineEdit)
    ui->firstNameLineEdit->setText(profileInfo.firstName);
    ui->firstNameLineEdit->setPlaceholderText(tr("Введіть ім'я"));
    ui->lastNameLineEdit->setText(profileInfo.lastName);
    ui->lastNameLineEdit->setPlaceholderText(tr("Введіть прізвище"));
    ui->emailLabel->setText(profileInfo.email); // Email залишається QLabel
    ui->phoneLineEdit->setText(profileInfo.phone);
    ui->phoneLineEdit->setPlaceholderText(tr("Введіть номер телефону"));
    ui->addressLineEdit->setText(profileInfo.address);
    ui->addressLineEdit->setPlaceholderText(tr("Введіть адресу"));
    ui->joinDateLabel->setText(profileInfo.joinDate.isValid() ? profileInfo.joinDate.toString("dd.MM.yyyy") : tr("(невідомо)"));
    ui->loyaltyLabel->setText(profileInfo.loyaltyProgram ? tr("Так") : tr("Ні"));
    ui->pointsLabel->setText(QString::number(profileInfo.loyaltyPoints));

    // Поля, які не редагуються (Email, Дата реєстрації, Лояльність), можна зробити візуально неактивними
    ui->emailLabel->setEnabled(false);
    ui->joinDateLabel->setEnabled(false);
    ui->loyaltyLabel->setEnabled(false);
    ui->pointsLabel->setEnabled(false);
}


// Слот для обробки натискання кнопки "Зберегти"
void ProfileDialog::on_buttonBox_accepted()
{
    qInfo() << "Attempting to save profile changes via ProfileDialog for customer ID:" << m_customerId;

    if (!m_dbManager || m_customerId <= 0) {
        QMessageBox::critical(this, tr("Помилка"), tr("Неможливо зберегти зміни через внутрішню помилку."));
        return;
    }
    if (!ui->phoneLineEdit) {
        QMessageBox::critical(this, tr("Помилка інтерфейсу"), tr("Не вдалося знайти поле для номера телефону."));
        return;
    }
    // Перевіряємо вказівники на нові LineEdit
    if (!ui->firstNameLineEdit || !ui->lastNameLineEdit || !ui->phoneLineEdit || !ui->addressLineEdit) {
        QMessageBox::critical(this, tr("Помилка інтерфейсу"), tr("Не вдалося знайти одне або декілька полів для збереження."));
        return;
    }

    // Отримуємо нові значення з полів
    QString newFirstName = ui->firstNameLineEdit->text().trimmed();
    QString newLastName = ui->lastNameLineEdit->text().trimmed();
    QString newPhoneNumber = ui->phoneLineEdit->text().trimmed();
    QString newAddress = ui->addressLineEdit->text().trimmed();

    // Валідація (приклад: ім'я та прізвище не можуть бути порожніми)
    if (newFirstName.isEmpty()) {
        QMessageBox::warning(this, tr("Збереження профілю"), tr("Ім'я не може бути порожнім."));
        ui->firstNameLineEdit->setFocus(); // Встановлюємо фокус на проблемне поле
        return; // Не закриваємо діалог
    }
    if (newLastName.isEmpty()) {
        QMessageBox::warning(this, tr("Збереження профілю"), tr("Прізвище не може бути порожнім."));
        ui->lastNameLineEdit->setFocus();
        return; // Не закриваємо діалог
    }
    // TODO: Додати валідацію номера телефону та адреси за потреби

    // Викликаємо методи DatabaseManager для оновлення
    // Можна об'єднати в транзакцію в DatabaseManager, якщо потрібно
    bool nameSuccess = m_dbManager->updateCustomerName(m_customerId, newFirstName, newLastName);
    bool phoneSuccess = m_dbManager->updateCustomerPhone(m_customerId, newPhoneNumber);
    bool addressSuccess = m_dbManager->updateCustomerAddress(m_customerId, newAddress);

    if (nameSuccess && phoneSuccess && addressSuccess) {
        QMessageBox::information(this, tr("Успіх"), tr("Дані профілю успішно оновлено!"));
        qInfo() << "Profile data updated successfully via ProfileDialog for customer ID:" << m_customerId;
        // Якщо успішно, викликаємо стандартний accept(), щоб закрити діалог з результатом Accepted
        accept();
    } else {
        // Збираємо повідомлення про помилки
        QString errorMessage = tr("Не вдалося оновити дані профілю в базі даних:\n");
        if (!nameSuccess) errorMessage += tr("- Помилка оновлення імені/прізвища.\n");
        if (!phoneSuccess) errorMessage += tr("- Помилка оновлення телефону.\n");
        if (!addressSuccess) errorMessage += tr("- Помилка оновлення адреси.\n");
        errorMessage += tr("\nПеревірте журнал помилок.");

        QMessageBox::critical(this, tr("Помилка збереження"), errorMessage);
        qWarning() << "Failed to update profile data via ProfileDialog for customer ID:" << m_customerId << "Errors:" << m_dbManager->lastError().text();
        // Не закриваємо діалог
    }
}
