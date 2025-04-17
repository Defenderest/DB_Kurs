#ifndef PROFILEDIALOG_H
#define PROFILEDIALOG_H

#include <QDialog>
#include "database.h"

namespace Ui {
class ProfileDialog;
}

class ProfileDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProfileDialog(const CustomerProfileInfo &profileInfo, QWidget *parent = nullptr);
    ~ProfileDialog();

private:
    Ui::ProfileDialog *ui;

    void populateProfileData(const CustomerProfileInfo &profileInfo);
};

#endif
