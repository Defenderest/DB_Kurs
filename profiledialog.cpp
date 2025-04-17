#include "profiledialog.h"
#include "ui_profiledialog.h"
#include <QDate> // Для форматування дати
#include <QDialogButtonBox> // Потрібно для доступу до buttonBox

ProfileDialog::ProfileDialog(const CustomerProfileInfo &profileInfo, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ProfileDialog)
{
    ui->setupUi(this);
    setWindowTitle(tr("Ваш профіль")); // Встановлюємо заголовок вікна

    // Заповнюємо поля даними
    populateProfileData(profileInfo);

    // Підключаємо стандартну кнопку Close з QDialogButtonBox до слота reject() діалогу
    // (Це вже зроблено автоматично через connections в .ui файлі, але можна і вручну)
    // connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &ProfileDialog::reject);
    // connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &ProfileDialog::accept); // Якщо є кнопка OK/Accept
}

ProfileDialog::~ProfileDialog()
{
    delete ui;
}

void ProfileDialog::populateProfileData(const CustomerProfileInfo &profileInfo)
{
    // Перевіряємо, чи дані взагалі були знайдені (хоча це має перевірятись до створення діалогу)
    if (!profileInfo.found) {
        // Можна показати повідомлення про помилку або заповнити поля відповідним текстом
        ui->firstNameLabel->setText(tr("(Помилка завантаження)"));
        ui->lastNameLabel->setText(tr("(Помилка завантаження)"));
        ui->emailLabel->setText(tr("(Помилка завантаження)"));
        ui->phoneLabel->setText(tr("(Помилка завантаження)"));
        ui->addressLabel->setText(tr("(Помилка завантаження)"));
        ui->joinDateLabel->setText(tr("(Помилка завантаження)"));
        ui->loyaltyLabel->setText(tr("(Помилка завантаження)"));
        ui->pointsLabel->setText(tr("(Помилка завантаження)"));
        return;
    }

    // Заповнюємо поля, використовуючи імена віджетів з profiledialog.ui
    ui->firstNameLabel->setText(profileInfo.firstName.isEmpty() ? tr("(не вказано)") : profileInfo.firstName);
    ui->lastNameLabel->setText(profileInfo.lastName.isEmpty() ? tr("(не вказано)") : profileInfo.lastName);
    ui->emailLabel->setText(profileInfo.email); // Email має бути завжди
    ui->phoneLabel->setText(profileInfo.phone.isEmpty() ? tr("(не вказано)") : profileInfo.phone);
    ui->addressLabel->setText(profileInfo.address.isEmpty() ? tr("(не вказано)") : profileInfo.address);
    ui->joinDateLabel->setText(profileInfo.joinDate.isValid() ? profileInfo.joinDate.toString("dd.MM.yyyy") : tr("(невідомо)"));
    ui->loyaltyLabel->setText(profileInfo.loyaltyProgram ? tr("Так") : tr("Ні"));
    ui->pointsLabel->setText(QString::number(profileInfo.loyaltyPoints));
}
