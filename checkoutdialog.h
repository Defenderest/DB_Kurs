#ifndef CHECKOUTDIALOG_H
#define CHECKOUTDIALOG_H

#include <QDialog>
#include "datatypes.h"

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

public slots:
    void accept() override;

private slots:

private:
    Ui::CheckoutDialog *ui;
    double m_totalAmount;

    void setupUiElements(const CustomerProfileInfo& customerInfo);
};

#endif // CHECKOUTDIALOG_H
