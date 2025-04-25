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
    // Перевіряємо вказівники на ключові віджети нового дизайну
    // Мітки дати, лояльності та балів тепер у верхній секції, тому їх окремо не перевіряємо тут
    if (!ui->profilePictureLabel || !ui->profileFullNameLabel || !ui->profileEmailDisplayLabel ||
        !ui->profileFirstNameLineEdit || !ui->profileLastNameLineEdit ||
        !ui->profilePhoneLineEdit || !ui->profileAddressLineEdit ||
        /* Видалено перевірку: !ui->profileJoinDateDisplayLabel || */
        /* Видалено перевірку: !ui->profileLoyaltyDisplayLabel || */
        /* Видалено перевірку: !ui->profilePointsDisplayLabel || */
        !ui->editProfileButton || !ui->saveProfileButton) // Додамо перевірку кнопок для надійності
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

    // Встановлюємо іконку профілю (поки що стандартну)
    // TODO: Додати логіку завантаження фото користувача, якщо воно є
    ui->profilePictureLabel->setText("👤");
    ui->profilePictureLabel->setAlignment(Qt::AlignCenter);

    // Перевіряємо, чи дані взагалі були знайдені
    if (!profileInfo.found || profileInfo.customerId <= 0) {
        const QString errorText = tr("(Помилка завантаження)");
        const QString noDataText = tr("(Дані відсутні)");
        // Верхня секція
        ui->profileFullNameLabel->setText(errorText);
        ui->profileEmailDisplayLabel->setText(errorText);
        // Редаговані поля
        ui->profileFirstNameLineEdit->setText("");
        ui->profileFirstNameLineEdit->setPlaceholderText(noDataText);
        ui->profileFirstNameLineEdit->setEnabled(false);
        ui->profileLastNameLineEdit->setText("");
        ui->profileLastNameLineEdit->setPlaceholderText(noDataText);
        ui->profileLastNameLineEdit->setEnabled(false);
        ui->profilePhoneLineEdit->setText("");
        ui->profilePhoneLineEdit->setPlaceholderText(noDataText);
        ui->profilePhoneLineEdit->setEnabled(false);
        ui->profileAddressLineEdit->setText("");
        ui->profileAddressLineEdit->setPlaceholderText(noDataText);
        ui->profileAddressLineEdit->setEnabled(false);
        // Інформація про акаунт (вже у верхній секції)
        // Перевіряємо, чи існують мітки перед встановленням тексту помилки
        if (ui->profileJoinDateDisplayLabel) ui->profileJoinDateDisplayLabel->setText(errorText);
        if (ui->profileLoyaltyDisplayLabel) ui->profileLoyaltyDisplayLabel->setText(errorText);
        if (ui->profilePointsDisplayLabel) ui->profilePointsDisplayLabel->setText("-");
        // Кнопки
        ui->editProfileButton->setEnabled(false);
        ui->saveProfileButton->setEnabled(false);
        return;
    }

    // Заповнюємо поля даними
    // Верхня секція (не редагується напряму)
    ui->profileFullNameLabel->setText(profileInfo.firstName + " " + profileInfo.lastName);
    ui->profileEmailDisplayLabel->setText(profileInfo.email);

    // Редаговані поля (заповнюємо для перегляду/редагування)
    ui->profileFirstNameLineEdit->setText(profileInfo.firstName);
    ui->profileFirstNameLineEdit->setPlaceholderText(tr("Введіть ім'я"));
    ui->profileFirstNameLineEdit->setEnabled(true); // Дозволяємо редагування (керується setProfileEditingEnabled)
    ui->profileLastNameLineEdit->setText(profileInfo.lastName);
    ui->profileLastNameLineEdit->setPlaceholderText(tr("Введіть прізвище"));
    ui->profileLastNameLineEdit->setEnabled(true);
    ui->profilePhoneLineEdit->setText(profileInfo.phone);
    ui->profilePhoneLineEdit->setPlaceholderText(tr("Введіть номер телефону"));
    ui->profilePhoneLineEdit->setEnabled(true);
    ui->profileAddressLineEdit->setText(profileInfo.address);
    ui->profileAddressLineEdit->setPlaceholderText(tr("Введіть адресу"));
    ui->profileAddressLineEdit->setEnabled(true);

    // Інформація про акаунт (не редагується, тепер у верхній секції)
    // Перевіряємо існування міток перед встановленням тексту
    if (ui->profileJoinDateDisplayLabel) {
        ui->profileJoinDateDisplayLabel->setText(profileInfo.joinDate.isValid() ? profileInfo.joinDate.toString("dd.MM.yyyy") : tr("(невідомо)"));
    }
    if (ui->profileLoyaltyDisplayLabel) {
        ui->profileLoyaltyDisplayLabel->setText(profileInfo.loyaltyProgram ? tr("Так") : tr("Ні"));
    }
    if (ui->profilePointsDisplayLabel) {
        ui->profilePointsDisplayLabel->setText(QString::number(profileInfo.loyaltyPoints));
    }

    // Керування кнопками
    ui->editProfileButton->setEnabled(true); // Дозволяємо почати редагування
    ui->saveProfileButton->setEnabled(true); // Дозволяємо зберегти (стан видимості керується setProfileEditingEnabled)

    // Поля, які відображають інформацію, але не редагуються, робимо візуально неактивними
    // (можна додати стиль в .ui або тут)
    // ui->profileFullNameLabel->setEnabled(false); // Не потрібно, це просто текст
    // ui->profileEmailDisplayLabel->setEnabled(false);
    // ui->profileJoinDateDisplayLabel->setEnabled(false);
    // ui->profileLoyaltyDisplayLabel->setEnabled(false);
    // ui->profilePointsDisplayLabel->setEnabled(false);

    // Початковий стан - не редагування
    // setProfileEditingEnabled(false); // Викликається в on_navProfileButton_clicked
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

    // Вмикаємо/вимикаємо редагування полів LineEdit
    ui->profileFirstNameLineEdit->setReadOnly(!enabled);
    ui->profileLastNameLineEdit->setReadOnly(!enabled);
    ui->profilePhoneLineEdit->setReadOnly(!enabled);
    ui->profileAddressLineEdit->setReadOnly(!enabled);

    // Показуємо/ховаємо відповідні кнопки
    ui->editProfileButton->setVisible(!enabled);
    ui->saveProfileButton->setVisible(enabled);

    // Змінюємо стиль редагованих полів для візуального розрізнення
    // Використовуємо стандартні стилі Qt для стану readOnly
    // Або можна додати спеціальні властивості/стилі в mainwindow.ui
    QString lineEditStyleBase = "QLineEdit { padding: 8px 10px; border: 1px solid %1; border-radius: 4px; background-color: %2; color: #212529; min-height: 34px; font-size: 10pt; }";
    QString focusStyle = "QLineEdit:focus { border-color: #adb5bd; }"; // Зберігаємо стиль фокусу

    if (enabled) {
        // Стиль для редагування (білий фон, синя рамка при фокусі - стандартно)
        QString editStyle = lineEditStyleBase.arg("#ced4da", "#ffffff") + focusStyle;
        ui->profileFirstNameLineEdit->setStyleSheet(editStyle);
        ui->profileLastNameLineEdit->setStyleSheet(editStyle);
        ui->profilePhoneLineEdit->setStyleSheet(editStyle);
        ui->profileAddressLineEdit->setStyleSheet(editStyle);
    } else {
        // Стиль для читання (світло-сірий фон, стандартна рамка)
        QString readOnlyStyle = lineEditStyleBase.arg("#dee2e6", "#f8f9fa"); // Не додаємо focusStyle для readOnly
        ui->profileFirstNameLineEdit->setStyleSheet(readOnlyStyle);
        ui->profileLastNameLineEdit->setStyleSheet(readOnlyStyle);
        ui->profilePhoneLineEdit->setStyleSheet(readOnlyStyle);
        ui->profileAddressLineEdit->setStyleSheet(readOnlyStyle);
    }

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

        // Оновлюємо нередаговані поля (FullName) на основі збережених даних
        ui->profileFullNameLabel->setText(newFirstName + " " + newLastName);

        setProfileEditingEnabled(false); // Вимикаємо режим редагування

        // Не потрібно явно викликати populateProfilePanel тут,
        // оскільки ми вже оновили FullName і вимкнули редагування.
        // Якщо інші нередаговані поля (наприклад, бали лояльності) могли змінитися
        // в результаті інших дій, тоді перезавантаження було б доцільним.
        // CustomerProfileInfo profile = m_dbManager->getCustomerProfileInfo(m_currentCustomerId);
        // populateProfilePanel(profile);
    } else {
        QString errorMessage = tr("Не вдалося оновити дані профілю:\n");
        if (!nameSuccess) errorMessage += tr("- Помилка оновлення імені/прізвища. (%1)\n").arg(m_dbManager->lastError().text());
        if (!phoneSuccess) errorMessage += tr("- Помилка оновлення телефону. (%1)\n").arg(m_dbManager->lastError().text());
        if (!addressSuccess) errorMessage += tr("- Помилка оновлення адреси. (%1)\n").arg(m_dbManager->lastError().text());
        // errorMessage += tr("\nПеревірте журнал помилок."); // Помилки вже включені
        QMessageBox::critical(this, tr("Помилка збереження"), errorMessage);
        qWarning() << "Failed to update profile data for customer ID:" << m_currentCustomerId;
        // Не вимикаємо режим редагування, щоб користувач міг виправити помилку
    }
}
