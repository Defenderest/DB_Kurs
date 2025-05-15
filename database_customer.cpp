#include "database.h"
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QCryptographicHash>
#include <QDate>

// Реалізація методу для отримання даних для входу
CustomerLoginInfo DatabaseManager::getCustomerLoginInfo(const QString &email) const
{
    CustomerLoginInfo loginInfo;
    loginInfo.found = false;
    if (!m_isConnected || !m_db.isOpen() || email.isEmpty()) {
        qWarning() << "Неможливо отримати дані для входу: немає з'єднання або email порожній.";
        return loginInfo;
    }

    const QString sql = getSqlQuery("GetCustomerLoginInfoByEmail");
    if (sql.isEmpty()) return loginInfo;

    QSqlQuery query(m_db);
    if (!query.prepare(sql)) {
        qCritical() << "Помилка підготовки запиту 'GetCustomerLoginInfoByEmail':" << query.lastError().text();
        return loginInfo;
    }
    query.bindValue(":email", email);

    qInfo() << "Executing SQL 'GetCustomerLoginInfoByEmail' for email:" << email;
    if (!query.exec()) {
        qCritical() << "Помилка при виконанні 'GetCustomerLoginInfoByEmail' для email '" << email << "':";
        qCritical() << query.lastError().text();
        qCritical() << "SQL запит:" << query.lastQuery();
        return loginInfo;
    }

    if (query.next()) {
        loginInfo.customerId = query.value("customer_id").toInt();
        loginInfo.passwordHash = query.value("password_hash").toString();
        loginInfo.found = true;
        qInfo() << "Login info found for email:" << email << "Customer ID:" << loginInfo.customerId;
    } else {
        qInfo() << "Login info not found for email:" << email;
        // loginInfo.found залишається false
    }

    return loginInfo;
}


// Реалізація нового методу для отримання повної інформації профілю користувача
CustomerProfileInfo DatabaseManager::getCustomerProfileInfo(int customerId) const
{
    CustomerProfileInfo profileInfo;
    profileInfo.found = false; // Явно встановлюємо за замовчуванням
    if (!m_isConnected || !m_db.isOpen() || customerId <= 0) {
        qWarning() << "Неможливо отримати профіль: немає з'єднання або невірний customerId.";
        return profileInfo; // Повертаємо порожню структуру
    }

    const QString sql = getSqlQuery("GetCustomerProfileInfoById");
    if (sql.isEmpty()) return profileInfo;

    QSqlQuery query(m_db);
    if (!query.prepare(sql)) {
        qCritical() << "Помилка підготовки запиту 'GetCustomerProfileInfoById':" << query.lastError().text();
        return profileInfo;
    }
    query.bindValue(":customerId", customerId);

    qInfo() << "Executing SQL 'GetCustomerProfileInfoById' for customer ID:" << customerId;
    if (!query.exec()) {
        qCritical() << "Помилка при виконанні 'GetCustomerProfileInfoById' для customer ID '" << customerId << "':";
        qCritical() << query.lastError().text();
        qCritical() << "SQL запит:" << query.lastQuery();
        return profileInfo;
    }

    if (query.next()) {
        profileInfo.customerId = query.value("customer_id").toInt();
        profileInfo.firstName = query.value("first_name").toString();
        profileInfo.lastName = query.value("last_name").toString();
        profileInfo.email = query.value("email").toString();
        profileInfo.phone = query.value("phone").toString();
        profileInfo.address = query.value("address").toString();
        profileInfo.joinDate = query.value("join_date").toDate();
        profileInfo.loyaltyProgram = query.value("loyalty_program").toBool();
        profileInfo.loyaltyPoints = query.value("loyalty_points").toInt();
        profileInfo.found = true;
        qInfo() << "Profile info found for customer ID:" << customerId;
    } else {
        qInfo() << "Profile info not found for customer ID:" << customerId;
    }

    return profileInfo;
}


// Реалізація нового методу для реєстрації користувача
bool DatabaseManager::registerCustomer(const CustomerRegistrationInfo &regInfo, int &newCustomerId)
{
    newCustomerId = -1;
    if (!m_isConnected || !m_db.isOpen()) {
        qWarning() << "Неможливо зареєструвати користувача: немає з'єднання з БД.";
        return false;
    }
    if (regInfo.email.isEmpty() || regInfo.password.isEmpty() || regInfo.firstName.isEmpty() || regInfo.lastName.isEmpty()) {
        qWarning() << "Неможливо зареєструвати користувача: не всі поля заповнені.";
        return false;
    }

    // Хешуємо пароль
    QByteArray passwordHashBytes = QCryptographicHash::hash(regInfo.password.toUtf8(), QCryptographicHash::Sha256);
    QString passwordHashHex = QString::fromUtf8(passwordHashBytes.toHex());

    const QString sql = getSqlQuery("RegisterCustomer");
    if (sql.isEmpty()) return false; // Помилка завантаження запиту

    QSqlQuery query(m_db);
    if (!query.prepare(sql)) {
        qCritical() << "Помилка підготовки запиту 'RegisterCustomer':" << query.lastError().text();
        return false;
    }
    query.bindValue(":first_name", regInfo.firstName);
    query.bindValue(":last_name", regInfo.lastName);
    query.bindValue(":email", regInfo.email);
    query.bindValue(":password_hash", passwordHashHex);

    qInfo() << "Executing SQL 'RegisterCustomer' for email:" << regInfo.email;

    QVariant insertedId;
    // Використовуємо executeInsertQuery, який вже обробляє помилки та логування
    if (executeInsertQuery(query, QString("Register Customer %1").arg(regInfo.email), insertedId)) {
        newCustomerId = insertedId.toInt();
        qInfo() << "Customer registered successfully. Email:" << regInfo.email << "New ID:" << newCustomerId;
        return true;
    } else {

        QSqlError err = query.lastError();
        if (err.isValid() && (err.text().contains("customer_email_key") || err.text().contains("duplicate key value violates unique constraint"))) {
            qWarning() << "Registration failed: Email already exists -" << regInfo.email;
        } else if (err.isValid()) {
             qCritical() << "Registration failed for email '" << regInfo.email << "':" << err.text();
        } else {
             qCritical() << "Registration failed for email '" << regInfo.email << "' with unknown error after executeInsertQuery.";
        }
        return false;
    }
}

// Реалізація нового методу для оновлення імені та прізвища користувача
bool DatabaseManager::updateCustomerName(int customerId, const QString &firstName, const QString &lastName)
{
    if (!m_isConnected || !m_db.isOpen() || customerId <= 0) {
        qWarning() << "Неможливо оновити ім'я/прізвище: немає з'єднання або невірний customerId.";
        return false;
    }
    if (firstName.isEmpty() || lastName.isEmpty()) {
        qWarning() << "Неможливо оновити ім'я/прізвище: ім'я або прізвище порожні.";
        return false;
    }

    const QString sql = getSqlQuery("UpdateCustomerName");
    if (sql.isEmpty()) return false;
    QSqlQuery query(m_db);
    if (!query.prepare(sql)) {
        qCritical() << "Помилка підготовки запиту 'UpdateCustomerName':" << query.lastError().text();
        return false;
    }
    query.bindValue(":firstName", firstName);
    query.bindValue(":lastName", lastName);
    query.bindValue(":customerId", customerId);

    qInfo() << "Executing SQL 'UpdateCustomerName' for customer ID:" << customerId;
    if (!query.exec()) {
        qCritical() << "Помилка при виконанні 'UpdateCustomerName' для customer ID '" << customerId << "':";
        qCritical() << query.lastError().text();
        qCritical() << "SQL запит:" << query.lastQuery();
        qCritical() << "Bound values:" << query.boundValues();
        return false;
    }

    if (query.numRowsAffected() > 0) {
        qInfo() << "Name updated successfully for customer ID:" << customerId;
        return true;
    } else {
        const QString checkSql = getSqlQuery("CheckCustomerExistsById");
        if (checkSql.isEmpty()) return false;

        QSqlQuery checkQuery(m_db);
        if (!checkQuery.prepare(checkSql)) {
             qWarning() << "Failed to prepare 'CheckCustomerExistsById' query during name update check.";
             return false; // Не можемо перевірити
        }
        checkQuery.bindValue(":customerId", customerId);
        if (checkQuery.exec() && checkQuery.next()) {
             qInfo() << "Name update query executed, but no rows were affected for customer ID:" << customerId << "(Name likely unchanged)";
             return true; // Вважаємо успіхом, якщо користувач існує, але дані ті ж самі
        } else {
             qWarning() << "Name update query executed, but no rows were affected for customer ID:" << customerId << "(Customer might not exist)";
             return false; // Користувача не знайдено
        }
    }
}

// Реалізація нового методу для оновлення адреси користувача
bool DatabaseManager::updateCustomerAddress(int customerId, const QString &newAddress)
{
    if (!m_isConnected || !m_db.isOpen() || customerId <= 0) {
        qWarning() << "Неможливо оновити адресу: немає з'єднання або невірний customerId.";
        return false;
    }

    const QString sql = getSqlQuery("UpdateCustomerAddress");
    if (sql.isEmpty()) return false; // Помилка завантаження запиту

    QSqlQuery query(m_db);
    if (!query.prepare(sql)) {
        qCritical() << "Помилка підготовки запиту 'UpdateCustomerAddress':" << query.lastError().text();
        return false;
    }
    // Дозволяємо встановлювати NULL, якщо рядок порожній
    query.bindValue(":address", newAddress.isEmpty() ? QVariant(QVariant::String) : newAddress);
    query.bindValue(":customerId", customerId);

    qInfo() << "Executing SQL 'UpdateCustomerAddress' for customer ID:" << customerId;
    if (!query.exec()) {
        qCritical() << "Помилка при виконанні 'UpdateCustomerAddress' для customer ID '" << customerId << "':";
        qCritical() << query.lastError().text();
        qCritical() << "SQL запит:" << query.lastQuery();
        qCritical() << "Bound values:" << query.boundValues();
        return false;
    }

    if (query.numRowsAffected() > 0) {
        qInfo() << "Address updated successfully for customer ID:" << customerId;
        return true;
    } else {
        // Перевіряємо, чи існує користувач
        const QString checkSql = getSqlQuery("CheckCustomerExistsById");
        if (checkSql.isEmpty()) return false; // Не можемо перевірити

        QSqlQuery checkQuery(m_db);
         if (!checkQuery.prepare(checkSql)) {
             qWarning() << "Failed to prepare 'CheckCustomerExistsById' query during address update check.";
             return false; // Не можемо перевірити
         }
        checkQuery.bindValue(":customerId", customerId);
         if (checkQuery.exec() && checkQuery.next()) {
            qInfo() << "Address update query executed, but no rows were affected for customer ID:" << customerId << "(Address likely unchanged)";
            return true; // Успіх, якщо користувач існує
        } else {
            qWarning() << "Address update query executed, but no rows were affected for customer ID:" << customerId << "(Customer might not exist)";
            return false; // Користувача не знайдено
        }
    }
}

// Реалізація нового методу для додавання бонусних балів
bool DatabaseManager::addLoyaltyPoints(int customerId, int pointsToAdd)
{
    if (!m_isConnected || !m_db.isOpen() || customerId <= 0 || pointsToAdd <= 0) {
        qWarning() << "Неможливо додати бонусні бали: немає з'єднання, невірний customerId або кількість балів <= 0.";
        return false;
    }

    // Оновлюємо бали та вмикаємо програму лояльності, якщо вона ще не активна
    const QString sql = getSqlQuery("AddLoyaltyPoints");
    if (sql.isEmpty()) return false;
    QSqlQuery query(m_db);
    if (!query.prepare(sql)) {
        qCritical() << "Помилка підготовки запиту 'AddLoyaltyPoints':" << query.lastError().text();
        return false;
    }
    query.bindValue(":pointsToAdd", pointsToAdd);
    query.bindValue(":customerId", customerId);

    qInfo() << "Executing SQL 'AddLoyaltyPoints' to add" << pointsToAdd << "points for customer ID:" << customerId;
    if (!query.exec()) {
        qCritical() << "Помилка при виконанні 'AddLoyaltyPoints' для customer ID '" << customerId << "':";
        qCritical() << query.lastError().text();
        qCritical() << "SQL запит:" << query.lastQuery();
        qCritical() << "Bound values:" << query.boundValues();
        return false;
    }

    if (query.numRowsAffected() > 0) {
        qInfo() << "Loyalty points added successfully for customer ID:" << customerId;
        return true;
    } else {

        const QString checkSql = getSqlQuery("CheckCustomerExistsById");
        if (checkSql.isEmpty()) return false;
        QSqlQuery checkQuery(m_db);
        if (!checkQuery.prepare(checkSql)) {
             qWarning() << "Failed to prepare 'CheckCustomerExistsById' query during loyalty points update check.";
             return false;
        }
        checkQuery.bindValue(":customerId", customerId);
        if (checkQuery.exec() && checkQuery.next()) {
            qWarning() << "Loyalty points update query executed, but no rows were affected for customer ID:" << customerId << "(Should not happen unless pointsToAdd was 0)";

            return false;
        } else {
            qWarning() << "Loyalty points update query executed, but no rows were affected for customer ID:" << customerId << "(Customer might not exist)";
            return false;
        }
    }
}


// Реалізація нового методу для оновлення телефону користувача
bool DatabaseManager::updateCustomerPhone(int customerId, const QString &newPhone)
{
    if (!m_isConnected || !m_db.isOpen() || customerId <= 0) {
        qWarning() << "Неможливо оновити телефон: немає з'єднання або невірний customerId.";
        return false;
    }

    const QString sql = getSqlQuery("UpdateCustomerPhone");
    if (sql.isEmpty()) return false;
    QSqlQuery query(m_db);
    if (!query.prepare(sql)) {
        qCritical() << "Помилка підготовки запиту 'UpdateCustomerPhone':" << query.lastError().text();
        return false;
    }
    query.bindValue(":phone", newPhone.isEmpty() ? QVariant(QVariant::String) : newPhone); // Дозволяємо встановлювати NULL, якщо рядок порожній
    query.bindValue(":customerId", customerId);

    qInfo() << "Executing SQL 'UpdateCustomerPhone' for customer ID:" << customerId;
    if (!query.exec()) {
        qCritical() << "Помилка при виконанні 'UpdateCustomerPhone' для customer ID '" << customerId << "':";
        qCritical() << query.lastError().text();
        qCritical() << "SQL запит:" << query.lastQuery();
        qCritical() << "Bound values:" << query.boundValues();
        return false;
    }

    if (query.numRowsAffected() > 0) {
        qInfo() << "Phone number updated successfully for customer ID:" << customerId;
        return true;
    } else {

        const QString checkSql = getSqlQuery("CheckCustomerExistsById");
        if (checkSql.isEmpty()) return false;
        QSqlQuery checkQuery(m_db);
         if (!checkQuery.prepare(checkSql)) {
             qWarning() << "Failed to prepare 'CheckCustomerExistsById' query during phone update check.";
             return false;
         }
        checkQuery.bindValue(":customerId", customerId);
         if (checkQuery.exec() && checkQuery.next()) {
            qInfo() << "Phone update query executed, but no rows were affected for customer ID:" << customerId << "(Phone likely unchanged)";
            return true;
        } else {
            qWarning() << "Phone update query executed, but no rows were affected for customer ID:" << customerId << "(Customer might not exist)";
            return false;
        }
    }
}
