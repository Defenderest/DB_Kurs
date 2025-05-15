#ifndef PROFILEDIALOG_H
#define PROFILEDIALOG_H

#include <QDialog>
#include "database.h"

class DatabaseManager;
struct CustomerProfileInfo;

namespace Ui {
class ProfileDialog;
}

class ProfileDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProfileDialog(DatabaseManager *dbManager, int customerId, QWidget *parent = nullptr);
    ~ProfileDialog();

private slots:
    void on_buttonBox_accepted();

private:
    Ui::ProfileDialog *ui;
    DatabaseManager *m_dbManager;
    int m_customerId;

    void populateProfileData();
};

#endif // PROFILEDIALOG_H
