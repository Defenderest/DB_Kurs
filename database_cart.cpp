#include "database.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>
#include <QMap>

// --- Реалізація методів для роботи з корзиною ---

QMap<int, int> DatabaseManager::getCartItems(int customerId) const
{
    QMap<int, int> cartItems;
    if (!m_isConnected || !m_db.isOpen()) {
        qWarning() << "getCartItems: Немає активного з'єднання з БД.";
        return cartItems; // Повертаємо порожню мапу
    }

    const QString sql = getSqlQuery("GetCartItemsByCustomerId");
    if (sql.isEmpty()) return cartItems; // Помилка завантаження запиту

    QSqlQuery query(m_db);
    if (!query.prepare(sql)) {
        qCritical() << "Помилка підготовки запиту 'GetCartItemsByCustomerId':" << query.lastError().text();
        return cartItems;
    }
    query.bindValue(":customerId", customerId);

    qInfo() << "Executing SQL 'GetCartItemsByCustomerId' for customer ID:" << customerId;
    if (!query.exec()) {
        qCritical() << "Помилка при виконанні 'GetCartItemsByCustomerId' для customerId" << customerId << ":" << query.lastError().text();
        return cartItems; // Повертаємо порожню мапу
    }

    while (query.next()) {
        int bookId = query.value("book_id").toInt();
        int quantity = query.value("quantity").toInt();
        if (bookId > 0 && quantity > 0) {
            cartItems.insert(bookId, quantity);
        } else {
            qWarning() << "getCartItems: Отримано некоректні дані з БД (bookId або quantity <= 0) для customerId" << customerId;
        }
    }

    qInfo() << "Завантажено" << cartItems.size() << "товарів з корзини для customerId" << customerId;
    return cartItems;
}

bool DatabaseManager::addOrUpdateCartItem(int customerId, int bookId, int quantity)
{
    if (!m_isConnected || !m_db.isOpen()) {
        qWarning() << "addOrUpdateCartItem: Немає активного з'єднання з БД.";
        return false;
    }
    if (quantity <= 0) {
        qWarning() << "addOrUpdateCartItem: Спроба додати товар з кількістю <= 0. Видалення товару...";
        return removeCartItem(customerId, bookId); // Якщо кількість 0 або менше, видаляємо товар
    }

    QSqlQuery query(m_db);
    // Використовуємо завантажений SQL запит
    const QString sql = getSqlQuery("AddOrUpdateCartItem");
     if (sql.isEmpty()) return false; // Помилка завантаження запиту

    // QSqlQuery query(m_db); // Видаляємо повторне оголошення
    if (!query.prepare(sql)) {
        qCritical() << "Помилка підготовки запиту 'AddOrUpdateCartItem':" << query.lastError().text();
        return false;
    }
    query.bindValue(":customerId", customerId);
    query.bindValue(":bookId", bookId);
    query.bindValue(":quantity", quantity);

    qInfo() << "Executing SQL 'AddOrUpdateCartItem' for customer ID:" << customerId << "Book ID:" << bookId;
    if (!query.exec()) {
        qCritical() << "Помилка при виконанні 'AddOrUpdateCartItem' (bookId" << bookId << ", quantity" << quantity
                   << ") для customerId" << customerId << ":" << query.lastError().text();
        return false;
    }

    qInfo() << "Товар (bookId" << bookId << ", quantity" << quantity << ") успішно додано/оновлено в корзині БД для customerId" << customerId;
    return true;
}

bool DatabaseManager::removeCartItem(int customerId, int bookId)
{
    if (!m_isConnected || !m_db.isOpen()) {
        qWarning() << "removeCartItem: Немає активного з'єднання з БД.";
        return false;
    }

    const QString sql = getSqlQuery("RemoveCartItem");
    if (sql.isEmpty()) return false; // Помилка завантаження запиту

    QSqlQuery query(m_db);
    if (!query.prepare(sql)) {
        qCritical() << "Помилка підготовки запиту 'RemoveCartItem':" << query.lastError().text();
        return false;
    }
    query.bindValue(":customerId", customerId);
    query.bindValue(":bookId", bookId);

    qInfo() << "Executing SQL 'RemoveCartItem' for customer ID:" << customerId << "Book ID:" << bookId;
    if (!query.exec()) {
        qCritical() << "Помилка при виконанні 'RemoveCartItem' (bookId" << bookId << ") для customerId" << customerId << ":" << query.lastError().text();
        return false;
    }

    if (query.numRowsAffected() > 0) {
        qInfo() << "Товар (bookId" << bookId << ") успішно видалено з корзини БД для customerId" << customerId;
    } else {
        qWarning() << "removeCartItem: Товар (bookId" << bookId << ") не знайдено в корзині БД для customerId" << customerId << " (можливо, вже видалено).";
    }
    return true; // Повертаємо true, навіть якщо нічого не було видалено (операція пройшла без помилок БД)
}

bool DatabaseManager::clearCart(int customerId)
{
    if (!m_isConnected || !m_db.isOpen()) {
        qWarning() << "clearCart: Немає активного з'єднання з БД.";
        return false;
    }

    const QString sql = getSqlQuery("ClearCartByCustomerId");
    if (sql.isEmpty()) return false; // Помилка завантаження запиту

    QSqlQuery query(m_db);
    if (!query.prepare(sql)) {
        qCritical() << "Помилка підготовки запиту 'ClearCartByCustomerId':" << query.lastError().text();
        return false;
    }
    query.bindValue(":customerId", customerId);

    qInfo() << "Executing SQL 'ClearCartByCustomerId' for customer ID:" << customerId;
    if (!query.exec()) {
        qCritical() << "Помилка при виконанні 'ClearCartByCustomerId' для customerId" << customerId << ":" << query.lastError().text();
        return false;
    }

    qInfo() << "Корзину БД успішно очищено для customerId" << customerId << "(видалено рядків:" << query.numRowsAffected() << ")";
    return true;
}
