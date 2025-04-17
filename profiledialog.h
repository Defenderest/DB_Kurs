#ifndef PROFILEDIALOG_H
#define PROFILEDIALOG_H

#include <QDialog>
#include "database.h" // Для структури CustomerProfileInfo

namespace Ui {
class ProfileDialog;
}

class ProfileDialog : public QDialog
{
    Q_OBJECT

public:
    // Конструктор приймає дані профілю та батьківський віджет
    explicit ProfileDialog(const CustomerProfileInfo &profileInfo, QWidget *parent = nullptr);
    ~ProfileDialog();

private:
    Ui::ProfileDialog *ui;

    // Метод для заповнення полів діалогу даними
    void populateProfileData(const CustomerProfileInfo &profileInfo);
};

#endif // PROFILEDIALOG_H
