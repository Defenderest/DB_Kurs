#include "database.h"
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QCryptographicHash> // Для хешування пароля
#include <QDate> // Для дат

// Реалізація методу для отримання даних для входу
CustomerLoginInfo DatabaseManager::getCustomerLoginInfo(const QString &email) const
{
    CustomerLoginInfo loginInfo;
    loginInfo.found = false; // Явно встановлюємо за замовчуванням
    if (!m_isConnected || !m_db.isOpen() || email.isEmpty()) {
        qWarning() << "Неможливо отримати дані для входу: немає з'єднання або email порожній.";
        return loginInfo; // Повертаємо порожню структуру
    }

    const QString sql = R"(
        SELECT customer_id, password_hash
        FROM customer
        WHERE email = :email;
    )";

    QSqlQuery query(m_db);
    query.prepare(sql);
    query.bindValue(":email", email);

    qInfo() << "Executing SQL to get login info for email:" << email;
    if (!query.exec()) {
        qCritical() << "Помилка при отриманні даних для входу для email '" << email << "':";
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

    const QString sql = R"(
        SELECT
            customer_id, first_name, last_name, email, phone, address,
            join_date, loyalty_program, loyalty_points
        FROM customer
        WHERE customer_id = :customerId;
    )";

    QSqlQuery query(m_db);
    query.prepare(sql);
    query.bindValue(":customerId", customerId);

    qInfo() << "Executing SQL to get profile info for customer ID:" << customerId;
    if (!query.exec()) {
        qCritical() << "Помилка при отриманні профілю для customer ID '" << customerId << "':";
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
        // profileInfo.found залишається false
    }

    return profileInfo;
}


// Реалізація нового методу для реєстрації користувача
bool DatabaseManager::registerCustomer(const CustomerRegistrationInfo &regInfo, int &newCustomerId)
{
    newCustomerId = -1; // Ініціалізуємо ID помилковим значенням
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

    const QString sql = R"(
        INSERT INTO customer (first_name, last_name, email, password_hash, join_date, loyalty_program, loyalty_points)
        VALUES (:first_name, :last_name, :email, :password_hash, CURRENT_DATE, FALSE, 0)
        RETURNING customer_id;
    )";

    QSqlQuery query(m_db);
    query.prepare(sql);
    query.bindValue(":first_name", regInfo.firstName);
    query.bindValue(":last_name", regInfo.lastName);
    query.bindValue(":email", regInfo.email);
    query.bindValue(":password_hash", passwordHashHex);

    qInfo() << "Executing SQL to register new customer with email:" << regInfo.email;

    QVariant insertedId;
    // Використовуємо executeInsertQuery, який вже обробляє помилки та логування
    if (executeInsertQuery(query, QString("Register Customer %1").arg(regInfo.email), insertedId)) {
        newCustomerId = insertedId.toInt();
        qInfo() << "Customer registered successfully. Email:" << regInfo.email << "New ID:" << newCustomerId;
        return true;
    } else {
        // Перевіряємо, чи помилка пов'язана з унікальністю email
        // Зверніть увагу: lastError() тут може бути не тим, що очікується,
        // бо executeInsertQuery вже виконав запит. Краще перевіряти помилку всередині executeInsertQuery
        // або передавати її назовні. Поки що залишимо перевірку тексту помилки.
        QSqlError err = query.lastError(); // Отримуємо помилку з останнього запиту
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
        return false; // Ім'я та прізвище не можуть бути порожніми
    }

    const QString sql = R"(
        UPDATE customer
        SET first_name = :firstName, last_name = :lastName
        WHERE customer_id = :customerId;
    )";

    QSqlQuery query(m_db);
    query.prepare(sql);
    query.bindValue(":firstName", firstName);
    query.bindValue(":lastName", lastName);
    query.bindValue(":customerId", customerId);

    qInfo() << "Executing SQL to update name for customer ID:" << customerId;
    if (!query.exec()) {
        qCritical() << "Помилка при оновленні імені/прізвища для customer ID '" << customerId << "':";
        qCritical() << query.lastError().text();
        qCritical() << "SQL запит:" << query.lastQuery();
        qCritical() << "Bound values:" << query.boundValues();
        return false;
    }

    if (query.numRowsAffected() > 0) {
        qInfo() << "Name updated successfully for customer ID:" << customerId;
        return true;
    } else {
        // Перевіряємо, чи існує користувач, щоб відрізнити "не знайдено" від "дані не змінилися"
        QSqlQuery checkQuery(m_db);
        checkQuery.prepare("SELECT 1 FROM customer WHERE customer_id = :customerId");
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

    const QString sql = R"(
        UPDATE customer
        SET address = :address
        WHERE customer_id = :customerId;
    )";

    QSqlQuery query(m_db);
    query.prepare(sql);
    // Дозволяємо встановлювати NULL, якщо рядок порожній
    query.bindValue(":address", newAddress.isEmpty() ? QVariant(QVariant::String) : newAddress);
    query.bindValue(":customerId", customerId);

    qInfo() << "Executing SQL to update address for customer ID:" << customerId;
    if (!query.exec()) {
        qCritical() << "Помилка при оновленні адреси для customer ID '" << customerId << "':";
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
        QSqlQuery checkQuery(m_db);
        checkQuery.prepare("SELECT 1 FROM customer WHERE customer_id = :customerId");
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
    const QString sql = R"(
        UPDATE customer
        SET loyalty_points = loyalty_points + :pointsToAdd,
            loyalty_program = TRUE -- Вмикаємо програму лояльності при нарахуванні балів
        WHERE customer_id = :customerId;
    )";

    QSqlQuery query(m_db);
    query.prepare(sql);
    query.bindValue(":pointsToAdd", pointsToAdd);
    query.bindValue(":customerId", customerId);

    qInfo() << "Executing SQL to add" << pointsToAdd << "loyalty points for customer ID:" << customerId;
    if (!query.exec()) {
        qCritical() << "Помилка при додаванні бонусних балів для customer ID '" << customerId << "':";
        qCritical() << query.lastError().text();
        qCritical() << "SQL запит:" << query.lastQuery();
        qCritical() << "Bound values:" << query.boundValues();
        return false;
    }

    if (query.numRowsAffected() > 0) {
        qInfo() << "Loyalty points added successfully for customer ID:" << customerId;
        return true;
    } else {
        // Перевіряємо, чи існує користувач
        QSqlQuery checkQuery(m_db);
        checkQuery.prepare("SELECT 1 FROM customer WHERE customer_id = :customerId");
        checkQuery.bindValue(":customerId", customerId);
        if (checkQuery.exec() && checkQuery.next()) {
            qWarning() << "Loyalty points update query executed, but no rows were affected for customer ID:" << customerId << "(Should not happen unless pointsToAdd was 0)";
            // Можливо, варто повернути true, якщо користувач існує, але це дивно
            return false;
        } else {
            qWarning() << "Loyalty points update query executed, but no rows were affected for customer ID:" << customerId << "(Customer might not exist)";
            return false; // Користувача не знайдено
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

    const QString sql = R"(
        UPDATE customer
        SET phone = :phone
        WHERE customer_id = :customerId;
    )";

    QSqlQuery query(m_db);
    query.prepare(sql);
    query.bindValue(":phone", newPhone.isEmpty() ? QVariant(QVariant::String) : newPhone); // Дозволяємо встановлювати NULL, якщо рядок порожній
    query.bindValue(":customerId", customerId);

    qInfo() << "Executing SQL to update phone for customer ID:" << customerId;
    if (!query.exec()) {
        qCritical() << "Помилка при оновленні телефону для customer ID '" << customerId << "':";
        qCritical() << query.lastError().text();
        qCritical() << "SQL запит:" << query.lastQuery();
        qCritical() << "Bound values:" << query.boundValues();
        return false;
    }

    // Перевіряємо, чи був оновлений хоча б один рядок
    if (query.numRowsAffected() > 0) {
        qInfo() << "Phone number updated successfully for customer ID:" << customerId;
        return true;
    } else {
         // Перевіряємо, чи існує користувач
        QSqlQuery checkQuery(m_db);
        checkQuery.prepare("SELECT 1 FROM customer WHERE customer_id = :customerId");
        checkQuery.bindValue(":customerId", customerId);
         if (checkQuery.exec() && checkQuery.next()) {
            qInfo() << "Phone update query executed, but no rows were affected for customer ID:" << customerId << "(Phone likely unchanged)";
            return true; // Успіх, якщо користувач існує
        } else {
            qWarning() << "Phone update query executed, but no rows were affected for customer ID:" << customerId << "(Customer might not exist)";
            return false; // Користувача не знайдено
        }
    }
}
