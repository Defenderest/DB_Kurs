#include "checkoutdialog.h"
#include "ui_checkoutdialog.h"
#include <QMessageBox>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>

CheckoutDialog::CheckoutDialog(const CustomerProfileInfo& customerInfo, double totalAmount, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CheckoutDialog),
    m_totalAmount(totalAmount)
{
    ui->setupUi(this);
    setWindowTitle(tr("Оформлення замовлення"));

    setupUiElements(customerInfo);

    ui->addressLineEdit->setFocus();
}

CheckoutDialog::~CheckoutDialog()
{
    delete ui;
}

void CheckoutDialog::setupUiElements(const CustomerProfileInfo& customerInfo)
{
    ui->nameLabel->setText(customerInfo.firstName + " " + customerInfo.lastName);
    ui->emailLabel->setText(customerInfo.email);

    ui->addressLineEdit->setText(customerInfo.address);

    ui->paymentMethodComboBox->clear();
    ui->paymentMethodComboBox->addItem(tr("Готівка при отриманні"));
    ui->paymentMethodComboBox->addItem(tr("Картка Visa/Mastercard"));

    ui->totalAmountLabel->setText(tr("Загальна сума: %1 грн").arg(QString::number(m_totalAmount, 'f', 2)));
}


QString CheckoutDialog::getShippingAddress() const
{
    return ui->addressLineEdit->text().trimmed();
}

QString CheckoutDialog::getPaymentMethod() const
{
    return ui->paymentMethodComboBox->currentText();
}

void CheckoutDialog::accept()
{
    if (getShippingAddress().isEmpty()) {
        QMessageBox::warning(this, tr("Потрібна адреса"), tr("Будь ласка, введіть адресу доставки."));
        ui->addressLineEdit->setFocus();
        return;
    }

    QDialog::accept();
}
