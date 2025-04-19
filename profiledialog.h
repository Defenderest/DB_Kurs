#ifndef PROFILEDIALOG_H
#define PROFILEDIALOG_H

#ifndef PROFILEDIALOG_H
#define PROFILEDIALOG_H

#include <QDialog>
#include "database.h" // Вже включено

// Forward declaration
class DatabaseManager;
struct CustomerProfileInfo;

namespace Ui {
class ProfileDialog;
}

class ProfileDialog : public QDialog
{
    Q_OBJECT

public:
    // Оновлений конструктор: приймає DatabaseManager та ID користувача
    explicit ProfileDialog(DatabaseManager *dbManager, int customerId, QWidget *parent = nullptr);
    ~ProfileDialog();

private slots:
    // Слот для обробки натискання кнопки "Зберегти" (підключається до accepted сигналу buttonBox)
    void on_buttonBox_accepted();

private:
    Ui::ProfileDialog *ui;
    DatabaseManager *m_dbManager; // Вказівник на менеджер БД
    int m_customerId;             // ID поточного користувача

    // Приватний метод для заповнення полів при створенні
    void populateProfileData();
};

#endif
