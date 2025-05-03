#include "checkoutdialog.h"
#include "ui_checkoutdialog.h" // Включаємо згенерований UI header
#include <QMessageBox>
#include <QLineEdit>     // Для доступу до addressLineEdit
#include <QComboBox>     // Для доступу до paymentMethodComboBox
#include <QLabel>        // Для доступу до міток
#include <QPushButton>   // Для доступу до кнопок (якщо потрібно)

CheckoutDialog::CheckoutDialog(const CustomerProfileInfo& customerInfo, double totalAmount, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CheckoutDialog), // Створюємо об'єкт UI
    m_totalAmount(totalAmount)
{
    ui->setupUi(this); // Налаштовуємо UI згідно з .ui файлом
    setWindowTitle(tr("Оформлення замовлення"));

    setupUiElements(customerInfo); // Викликаємо допоміжну функцію

    // Підключення стандартних кнопок QDialogButtonBox до слотів accept/reject
    // Це робиться автоматично в Designer'і, але можна зробити і тут:
    // connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &CheckoutDialog::accept);
    // connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &CheckoutDialog::reject);

    // Встановлюємо фокус на поле адреси
    ui->addressLineEdit->setFocus();
}

CheckoutDialog::~CheckoutDialog()
{
    delete ui; // Звільняємо пам'ять від UI
}

// Допоміжна функція для налаштування елементів UI
void CheckoutDialog::setupUiElements(const CustomerProfileInfo& customerInfo)
{
    // Відображаємо ім'я та email (тільки для читання)
    ui->nameLabel->setText(customerInfo.firstName + " " + customerInfo.lastName);
    ui->emailLabel->setText(customerInfo.email);

    // Заповнюємо адресу з профілю, дозволяємо редагування
    ui->addressLineEdit->setText(customerInfo.address);

    // Додаємо методи оплати (можна розширити)
    ui->paymentMethodComboBox->clear(); // Очищаємо, якщо там щось було
    ui->paymentMethodComboBox->addItem(tr("Готівка при отриманні"));
    ui->paymentMethodComboBox->addItem(tr("Картка Visa/Mastercard"));
    // Можна додати інші методи...

    // Відображаємо загальну суму
    ui->totalAmountLabel->setText(tr("Загальна сума: %1 грн").arg(QString::number(m_totalAmount, 'f', 2)));
}


QString CheckoutDialog::getShippingAddress() const
{
    // Повертаємо текст з поля адреси, видаляючи зайві пробіли
    return ui->addressLineEdit->text().trimmed();
}

QString CheckoutDialog::getPaymentMethod() const
{
    // Повертаємо вибраний текст з комбо-боксу
    return ui->paymentMethodComboBox->currentText();
}

// Перевизначаємо слот accept() для додавання валідації
void CheckoutDialog::accept()
{
    // Базова валідація адреси
    if (getShippingAddress().isEmpty()) {
        QMessageBox::warning(this, tr("Потрібна адреса"), tr("Будь ласка, введіть адресу доставки."));
        ui->addressLineEdit->setFocus(); // Повертаємо фокус на поле адреси
        return; // Не закриваємо діалог
    }

    // Якщо валідація пройшла, викликаємо базовий метод accept() для закриття діалогу з результатом Accepted
    QDialog::accept();
}

// Слот reject() викликається автоматично кнопкою Cancel/Відміна
// void CheckoutDialog::reject()
// {
//     // Можна додати логування або іншу логіку перед закриттям
//     QDialog::reject(); // Викликаємо базовий метод
// }
