#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QMessageBox>
#include <QDebug>
#include <QLabel> // Для populateProfilePanel
#include <QLineEdit> // Для populateProfilePanel, setProfileEditingEnabled, on_saveProfileButton_clicked
#include <QPushButton> // Для setProfileEditingEnabled
#include <QStatusBar> // Для on_saveProfileButton_clicked

// Слот для кнопки профілю в бічній панелі
void MainWindow::on_navProfileButton_clicked()
{
    qInfo() << "Navigating to profile page for customer ID:" << m_currentCustomerId;
    ui->contentStackedWidget->setCurrentWidget(ui->pageProfile);

    // Завантажуємо дані профілю при переході на сторінку
    if (m_currentCustomerId <= 0) {
        QMessageBox::warning(this, tr("Профіль користувача"), tr("Неможливо завантажити профіль, оскільки користувач не визначений."));
        populateProfilePanel(CustomerProfileInfo()); // Показати помилку в полях
        return;
    }
    if (!m_dbManager) {
         QMessageBox::critical(this, tr("Помилка"), tr("Помилка доступу до бази даних."));
         populateProfilePanel(CustomerProfileInfo()); // Показати помилку в полях
         return;
    }

    CustomerProfileInfo profile = m_dbManager->getCustomerProfileInfo(m_currentCustomerId);
    if (!profile.found) {
        QMessageBox::warning(this, tr("Профіль користувача"), tr("Не вдалося знайти інформацію для вашого профілю."));
    }
    populateProfilePanel(profile); // Заповнюємо сторінку профілю
    setProfileEditingEnabled(false); // Переконуємось, що режим редагування вимкнено при переході
}

// Слот для кнопки редагування профілю
void MainWindow::on_editProfileButton_clicked()
{
    setProfileEditingEnabled(true);
}

// Заповнення полів сторінки профілю даними
void MainWindow::populateProfilePanel(const CustomerProfileInfo &profileInfo)
{
    // Перевіряємо, чи вказівники на віджети існують (використовуємо нові LineEdit)
    if (!ui->profileFirstNameLineEdit || !ui->profileLastNameLineEdit || !ui->profileEmailLabel ||
        !ui->profilePhoneLineEdit || !ui->profileAddressLineEdit || !ui->profileJoinDateLabel ||
        !ui->profileLoyaltyLabel || !ui->profilePointsLabel)
    {
        qWarning() << "populateProfilePanel: One or more profile widgets are null!";
        // Не показуємо QMessageBox тут, щоб не заважати користувачу
        // Просто виходимо або встановлюємо текст помилки
        if(ui->pageProfile) { // Спробуємо показати помилку на самій сторінці
             clearLayout(ui->profilePageLayout); // Очистимо, щоб не було старих даних
             QLabel *errorLabel = new QLabel(tr("Помилка інтерфейсу: Не вдалося знайти поля для відображення профілю."), ui->pageProfile);
             errorLabel->setAlignment(Qt::AlignCenter);
             errorLabel->setWordWrap(true);
             ui->profilePageLayout->addWidget(errorLabel);
        }
        return;
    }

     // Перевіряємо, чи дані взагалі були знайдені
    if (!profileInfo.found || profileInfo.customerId <= 0) {
        // Заповнюємо поля текстом про помилку або відсутність даних (використовуємо нові LineEdit)
        const QString errorText = tr("(Помилка завантаження або дані відсутні)");
        ui->profileFirstNameLineEdit->setText("");
        ui->profileFirstNameLineEdit->setPlaceholderText(errorText);
        ui->profileFirstNameLineEdit->setEnabled(false);
        ui->profileLastNameLineEdit->setText("");
        ui->profileLastNameLineEdit->setPlaceholderText(errorText);
        ui->profileLastNameLineEdit->setEnabled(false);
        ui->profileEmailLabel->setText(errorText);
        ui->profilePhoneLineEdit->setText("");
        ui->profilePhoneLineEdit->setPlaceholderText(errorText);
        ui->profilePhoneLineEdit->setEnabled(false);
        ui->profileAddressLineEdit->setText("");
        ui->profileAddressLineEdit->setPlaceholderText(errorText);
        ui->profileAddressLineEdit->setEnabled(false);
        ui->profileJoinDateLabel->setText(errorText);
        ui->profileLoyaltyLabel->setText(errorText);
        ui->profilePointsLabel->setText("-");
        ui->saveProfileButton->setEnabled(false); // Блокуємо кнопку збереження
        return;
    }

    // Заповнюємо поля, використовуючи імена віджетів з mainwindow.ui (використовуємо нові LineEdit)
    ui->profileFirstNameLineEdit->setText(profileInfo.firstName);
    ui->profileFirstNameLineEdit->setPlaceholderText(tr("Введіть ім'я"));
    ui->profileFirstNameLineEdit->setEnabled(true);
    ui->profileLastNameLineEdit->setText(profileInfo.lastName);
    ui->profileLastNameLineEdit->setPlaceholderText(tr("Введіть прізвище"));
    ui->profileLastNameLineEdit->setEnabled(true);
    ui->profileEmailLabel->setText(profileInfo.email); // Email залишається QLabel
    ui->profilePhoneLineEdit->setText(profileInfo.phone);
    ui->profilePhoneLineEdit->setPlaceholderText(tr("Введіть номер телефону"));
    ui->profilePhoneLineEdit->setEnabled(true);
    ui->profileAddressLineEdit->setText(profileInfo.address);
    ui->profileAddressLineEdit->setPlaceholderText(tr("Введіть адресу"));
    ui->profileAddressLineEdit->setEnabled(true);
    ui->profileJoinDateLabel->setText(profileInfo.joinDate.isValid() ? profileInfo.joinDate.toString("dd.MM.yyyy") : tr("(невідомо)"));
    ui->profileLoyaltyLabel->setText(profileInfo.loyaltyProgram ? tr("Так") : tr("Ні"));
    ui->profilePointsLabel->setText(QString::number(profileInfo.loyaltyPoints));

    // Розблоковуємо кнопку збереження
    ui->saveProfileButton->setEnabled(true);

    // Поля, які не редагуються (Email, Дата реєстрації, Лояльність), можна зробити візуально неактивними
    ui->profileEmailLabel->setEnabled(false);
    ui->profileJoinDateLabel->setEnabled(false);
    ui->profileLoyaltyLabel->setEnabled(false);
    ui->profilePointsLabel->setEnabled(false);

    // Встановлюємо початковий стан редагування (зазвичай false)
    // setProfileEditingEnabled(false); // Перенесено в on_navProfileButton_clicked та конструктор
}


// Функція для ввімкнення/вимкнення режиму редагування профілю
void MainWindow::setProfileEditingEnabled(bool enabled)
{
    // Перевірка існування віджетів
    if (!ui->profileFirstNameLineEdit || !ui->profileLastNameLineEdit || !ui->profilePhoneLineEdit ||
        !ui->profileAddressLineEdit || !ui->editProfileButton || !ui->saveProfileButton)
    {
        qWarning() << "setProfileEditingEnabled: One or more profile widgets are null!";
        return;
    }

    // Вмикаємо/вимикаємо редагування полів
    ui->profileFirstNameLineEdit->setReadOnly(!enabled);
    ui->profileLastNameLineEdit->setReadOnly(!enabled);
    ui->profilePhoneLineEdit->setReadOnly(!enabled);
    ui->profileAddressLineEdit->setReadOnly(!enabled);

    // Показуємо/ховаємо кнопки
    ui->editProfileButton->setVisible(!enabled);
    ui->saveProfileButton->setVisible(enabled);

    // Змінюємо стиль полів для візуального розрізнення (опціонально)
    QString lineEditStyle = enabled
        ? "QLineEdit { background-color: white; border: 1px solid #86b7fe; }" // Стиль при редагуванні
        : "QLineEdit { background-color: #f8f9fa; border: 1px solid #dee2e6; }"; // Стиль при читанні
    ui->profileFirstNameLineEdit->setStyleSheet(lineEditStyle);
    ui->profileLastNameLineEdit->setStyleSheet(lineEditStyle);
    ui->profilePhoneLineEdit->setStyleSheet(lineEditStyle);
    ui->profileAddressLineEdit->setStyleSheet(lineEditStyle);

    // Встановлюємо фокус на перше поле при ввімкненні редагування
    if (enabled) {
        ui->profileFirstNameLineEdit->setFocus();
    }
}


// Слот для кнопки збереження змін у профілі
void MainWindow::on_saveProfileButton_clicked()
{
    qInfo() << "Attempting to save profile changes for customer ID:" << m_currentCustomerId;

    if (m_currentCustomerId <= 0) {
        QMessageBox::warning(this, tr("Збереження профілю"), tr("Неможливо зберегти зміни, користувач не визначений."));
        return;
    }
    if (!m_dbManager) {
         QMessageBox::critical(this, tr("Помилка"), tr("Помилка доступу до бази даних. Неможливо зберегти зміни."));
         return;
    }
    // Перевіряємо вказівники на нові LineEdit
    if (!ui->profileFirstNameLineEdit || !ui->profileLastNameLineEdit || !ui->profilePhoneLineEdit || !ui->profileAddressLineEdit) {
        QMessageBox::critical(this, tr("Помилка інтерфейсу"), tr("Не вдалося знайти одне або декілька полів профілю."));
        return;
    }

    // Отримуємо нові значення з полів
    QString newFirstName = ui->profileFirstNameLineEdit->text().trimmed();
    QString newLastName = ui->profileLastNameLineEdit->text().trimmed();
    QString newPhoneNumber = ui->profilePhoneLineEdit->text().trimmed();
    QString newAddress = ui->profileAddressLineEdit->text().trimmed();

    // Валідація (приклад)
    if (newFirstName.isEmpty()) {
        QMessageBox::warning(this, tr("Збереження профілю"), tr("Ім'я не може бути порожнім."));
        ui->profileFirstNameLineEdit->setFocus();
        return;
    }
    if (newLastName.isEmpty()) {
        QMessageBox::warning(this, tr("Збереження профілю"), tr("Прізвище не може бути порожнім."));
        ui->profileLastNameLineEdit->setFocus();
        return;
    }
    // TODO: Додати валідацію номера телефону та адреси

    // Викликаємо методи DatabaseManager для оновлення
    bool nameSuccess = m_dbManager->updateCustomerName(m_currentCustomerId, newFirstName, newLastName);
    bool phoneSuccess = m_dbManager->updateCustomerPhone(m_currentCustomerId, newPhoneNumber);
    bool addressSuccess = m_dbManager->updateCustomerAddress(m_currentCustomerId, newAddress);

    if (nameSuccess && phoneSuccess && addressSuccess) {
        ui->statusBar->showMessage(tr("Дані профілю успішно оновлено!"), 5000);
        qInfo() << "Profile data updated successfully for customer ID:" << m_currentCustomerId;
        setProfileEditingEnabled(false); // Вимикаємо режим редагування після успішного збереження
        // Можна перезавантажити дані, щоб переконатися, що все відображається коректно
        // CustomerProfileInfo profile = m_dbManager->getCustomerProfileInfo(m_currentCustomerId);
        // populateProfilePanel(profile); // Це оновить поля, але знову вимкне редагування
    } else {
        QString errorMessage = tr("Не вдалося оновити дані профілю:\n");
        if (!nameSuccess) errorMessage += tr("- Помилка оновлення імені/прізвища.\n");
        if (!phoneSuccess) errorMessage += tr("- Помилка оновлення телефону.\n");
        if (!addressSuccess) errorMessage += tr("- Помилка оновлення адреси.\n");
        errorMessage += tr("\nПеревірте журнал помилок.");
        QMessageBox::critical(this, tr("Помилка збереження"), errorMessage);
        qWarning() << "Failed to update profile data for customer ID:" << m_currentCustomerId << "Error:" << m_dbManager->lastError().text();
    }
}
