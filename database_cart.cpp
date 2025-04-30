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

    QSqlQuery query(m_db);
    query.prepare("SELECT book_id, quantity FROM cart_item WHERE customer_id = :customerId");
    query.bindValue(":customerId", customerId);

    if (!query.exec()) {
        qCritical() << "Помилка при отриманні товарів з корзини для customerId" << customerId << ":" << query.lastError().text();
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
    // Використовуємо INSERT ... ON CONFLICT ... UPDATE для атомарного додавання/оновлення
    // (Потребує PostgreSQL 9.5+ або аналогічної функціональності в інших СУБД)
    // Для SQLite: INSERT OR REPLACE INTO cart_item (...) VALUES (...);
    // Або: INSERT INTO cart_item (...) VALUES (...) ON CONFLICT(customer_id, book_id) DO UPDATE SET quantity = excluded.quantity;
    // Перевірте синтаксис для вашої СУБД! Нижче приклад для PostgreSQL/SQLite.
    query.prepare(R"(
        INSERT INTO cart_item (customer_id, book_id, quantity, added_date)
        VALUES (:customerId, :bookId, :quantity, CURRENT_TIMESTAMP)
        ON CONFLICT (customer_id, book_id) DO UPDATE SET
            quantity = EXCLUDED.quantity,
            added_date = CURRENT_TIMESTAMP;
    )");
    query.bindValue(":customerId", customerId);
    query.bindValue(":bookId", bookId);
    query.bindValue(":quantity", quantity);

    if (!query.exec()) {
        qCritical() << "Помилка при додаванні/оновленні товару (bookId" << bookId << ", quantity" << quantity
                   << ") в корзину для customerId" << customerId << ":" << query.lastError().text();
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

    QSqlQuery query(m_db);
    query.prepare("DELETE FROM cart_item WHERE customer_id = :customerId AND book_id = :bookId");
    query.bindValue(":customerId", customerId);
    query.bindValue(":bookId", bookId);

    if (!query.exec()) {
        qCritical() << "Помилка при видаленні товару (bookId" << bookId << ") з корзини БД для customerId" << customerId << ":" << query.lastError().text();
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

    QSqlQuery query(m_db);
    query.prepare("DELETE FROM cart_item WHERE customer_id = :customerId");
    query.bindValue(":customerId", customerId);

    if (!query.exec()) {
        qCritical() << "Помилка при очищенні корзини БД для customerId" << customerId << ":" << query.lastError().text();
        return false;
    }

    qInfo() << "Корзину БД успішно очищено для customerId" << customerId << "(видалено рядків:" << query.numRowsAffected() << ")";
    return true;
}
