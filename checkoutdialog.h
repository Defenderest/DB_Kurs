#ifndef CHECKOUTDIALOG_H
#define CHECKOUTDIALOG_H

#include <QDialog>
#include "datatypes.h" // Для CustomerProfileInfo

// Forward declaration для Ui::CheckoutDialog
namespace Ui {
class CheckoutDialog;
}

class CheckoutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CheckoutDialog(const CustomerProfileInfo& customerInfo, double totalAmount, QWidget *parent = nullptr);
    ~CheckoutDialog();

    QString getShippingAddress() const;
    QString getPaymentMethod() const;

private slots:
    // Слоти для стандартних кнопок Ok/Cancel (підключаються автоматично або через connect)
    // void on_confirmButton_clicked(); // Не потрібен, якщо використовуємо accept()
    // void on_cancelButton_clicked(); // Не потрібен, якщо використовуємо reject()

private:
    Ui::CheckoutDialog *ui; // Вказівник на UI, згенерований з .ui файлу
    double m_totalAmount;

    void setupUiElements(const CustomerProfileInfo& customerInfo); // Допоміжна функція для налаштування UI
};

#endif // CHECKOUTDIALOG_H
