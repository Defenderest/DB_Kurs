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
        // Заповнюємо поля текстом про помилку
        const QString errorText = tr("(Помилка завантаження)");
        ui->firstNameLabel->setText(errorText);
        ui->lastNameLabel->setText(errorText);
        ui->emailLabel->setText(errorText);
        ui->phoneLineEdit->setText(""); // Очищаємо поле вводу
        ui->phoneLineEdit->setPlaceholderText(errorText);
        ui->phoneLineEdit->setEnabled(false); // Блокуємо поле
        ui->addressLabel->setText(errorText);
        ui->joinDateLabel->setText(errorText);
        ui->loyaltyLabel->setText(errorText);
        ui->pointsLabel->setText("-");
        ui->buttonBox->button(QDialogButtonBox::Save)->setEnabled(false); // Блокуємо кнопку збереження
        return;
    }

    // Заповнюємо поля, використовуючи імена віджетів з profiledialog.ui
    ui->firstNameLabel->setText(profileInfo.firstName.isEmpty() ? tr("(не вказано)") : profileInfo.firstName);
    ui->lastNameLabel->setText(profileInfo.lastName.isEmpty() ? tr("(не вказано)") : profileInfo.lastName);
    ui->emailLabel->setText(profileInfo.email); // Email має бути завжди
    ui->phoneLineEdit->setText(profileInfo.phone); // Встановлюємо текст у QLineEdit
    ui->phoneLineEdit->setPlaceholderText(tr("Введіть номер телефону")); // Стандартний плейсхолдер
    ui->addressLabel->setText(profileInfo.address.isEmpty() ? tr("(не вказано)") : profileInfo.address);
    ui->joinDateLabel->setText(profileInfo.joinDate.isValid() ? profileInfo.joinDate.toString("dd.MM.yyyy") : tr("(невідомо)"));
    ui->loyaltyLabel->setText(profileInfo.loyaltyProgram ? tr("Так") : tr("Ні"));
    ui->pointsLabel->setText(QString::number(profileInfo.loyaltyPoints));

    // Робимо поля, які не редагуються, лише для читання (візуально)
    // (Можна також встановити setEnabled(false), якщо вони точно не мають змінюватись)
    // ui->firstNameLabel->setEnabled(false); // Наприклад
    // ui->lastNameLabel->setEnabled(false);
    // ui->emailLabel->setEnabled(false);
    // ui->addressLabel->setEnabled(false);
    // ui->joinDateLabel->setEnabled(false);
    // ui->loyaltyLabel->setEnabled(false);
    // ui->pointsLabel->setEnabled(false);
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

    // Отримуємо новий номер телефону
    QString newPhoneNumber = ui->phoneLineEdit->text().trimmed();

    // TODO: Додати валідацію номера телефону (наприклад, перевірка на цифри, довжину)
    // if (newPhoneNumber.isEmpty()) {
    //     QMessageBox::warning(this, tr("Збереження профілю"), tr("Номер телефону не може бути порожнім."));
    //     return; // Не закриваємо діалог
    // }

    // Викликаємо метод DatabaseManager для оновлення
    bool success = m_dbManager->updateCustomerPhone(m_customerId, newPhoneNumber);

    if (success) {
        QMessageBox::information(this, tr("Успіх"), tr("Номер телефону успішно оновлено!"));
        qInfo() << "Phone number updated successfully via ProfileDialog for customer ID:" << m_customerId;
        // Якщо успішно, викликаємо стандартний accept(), щоб закрити діалог з результатом Accepted
        accept();
    } else {
        QMessageBox::critical(this, tr("Помилка збереження"), tr("Не вдалося оновити номер телефону в базі даних. Перевірте журнал помилок."));
        qWarning() << "Failed to update phone number via ProfileDialog for customer ID:" << m_customerId << "Error:" << m_dbManager->lastError().text();
        // Не закриваємо діалог, щоб користувач міг спробувати ще раз або скасувати
    }
}
