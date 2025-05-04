#include "database.h"
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QMap> // Для createOrder
#include <QDateTime> // Для дат

// Реалізація нового методу для отримання деталей одного замовлення за ID
OrderDisplayInfo DatabaseManager::getOrderDetailsById(int orderId) const
{
    OrderDisplayInfo orderInfo; // Повернемо порожню, якщо не знайдено
    orderInfo.orderId = -1; // Позначка, що не знайдено

    if (!m_isConnected || !m_db.isOpen() || orderId <= 0) {
        qWarning() << "Неможливо отримати деталі замовлення: немає з'єднання або невірний orderId.";
        return orderInfo;
    }

    // 1. Отримуємо основну інформацію про замовлення
    const QString orderSql = getSqlQuery("GetOrderHeaderById");
    if (orderSql.isEmpty()) return orderInfo; // Помилка завантаження запиту

    QSqlQuery orderQuery(m_db);
    if (!orderQuery.prepare(orderSql)) {
        qCritical() << "Помилка підготовки запиту 'GetOrderHeaderById':" << orderQuery.lastError().text();
        return orderInfo;
    }
    orderQuery.bindValue(":orderId", orderId);

    qInfo() << "Executing SQL 'GetOrderHeaderById' for order ID:" << orderId;
    if (!orderQuery.exec()) {
        qCritical() << "Помилка при виконанні 'GetOrderHeaderById' для order ID '" << orderId << "':";
        qCritical() << orderQuery.lastError().text();
        qCritical() << "SQL запит:" << orderQuery.lastQuery();
        return orderInfo;
    }

    if (orderQuery.next()) {
        // --- Зміна: Отримуємо дату одразу як рядок ---
        QString dateString = orderQuery.value("order_date").toString();
        orderInfo.orderId = orderQuery.value("order_id").toInt();
        orderInfo.orderDate = QDateTime(); // Ініціалізуємо невалідною датою

        if (!dateString.isEmpty()) {
            // Спробуємо розпарсити рядок у QDateTime, використовуючи ISO формат (з мілісекундами або без)
            orderInfo.orderDate = QDateTime::fromString(dateString, Qt::ISODateWithMs);
            if (!orderInfo.orderDate.isValid()) { // Якщо з мілісекундами не вийшло, спробуємо без них
                orderInfo.orderDate = QDateTime::fromString(dateString, Qt::ISODate);
            }
            // Можна додати інші формати, якщо ISO не спрацює, наприклад:
            // if (!orderInfo.orderDate.isValid()) {
            //     orderInfo.orderDate = QDateTime::fromString(dateString, "yyyy-MM-dd HH:mm:ss");
            // }
        }
        // --- Кінець зміни ---
        orderInfo.totalAmount = orderQuery.value("total_amount").toDouble();
        orderInfo.shippingAddress = orderQuery.value("shipping_address").toString();
        orderInfo.paymentMethod = orderQuery.value("payment_method").toString();
     // --- Оновлена Відладка дати ---
     qDebug() << "[DEBUG] Order ID:" << orderInfo.orderId
              << "Raw order_date value (CAST to TEXT in SQL):" << dateString; // Показуємо рядок, отриманий з запиту
     qDebug() << "[DEBUG] Parsed QDateTime (after string parse attempts):" << orderInfo.orderDate
              << "Is Valid:" << orderInfo.orderDate.isValid();
     if (!orderInfo.orderDate.isValid() && !dateString.isEmpty()) {
          qWarning() << "[DEBUG] Failed to parse date string:" << dateString << "using ISODate/ISODateWithMs formats.";
       }
       // --- Кінець відладки ---
       orderInfo.found = true; // <<< Set the found flag here!
        qInfo() << "Order header found for ID:" << orderId;
    } else {
        qWarning() << "Order not found for ID:" << orderId;
        return orderInfo; // Повертаємо порожню структуру, якщо замовлення не знайдено
    }

    // 2. Отримуємо позиції для цього замовлення
    const QString itemsSql = getSqlQuery("GetOrderItemsByOrderId");
    if (itemsSql.isEmpty()) return orderInfo; // Помилка завантаження запиту, повертаємо що є

    QSqlQuery itemQuery(m_db);
    if (!itemQuery.prepare(itemsSql)) {
         qCritical() << "Помилка підготовки запиту 'GetOrderItemsByOrderId':" << itemQuery.lastError().text();
         return orderInfo; // Повертаємо те, що є
    }
    itemQuery.bindValue(":orderId", orderId);
    qInfo() << "Executing SQL 'GetOrderItemsByOrderId' for order ID:" << orderId;
    if (!itemQuery.exec()) {
        qCritical() << "Помилка при виконанні 'GetOrderItemsByOrderId' для order ID '" << orderId << "':";
        qCritical() << itemQuery.lastError().text();
        // Продовжуємо, щоб отримати статуси
    } else {
        while (itemQuery.next()) {
            OrderItemDisplayInfo itemInfo;
            itemInfo.quantity = itemQuery.value("quantity").toInt();
            itemInfo.pricePerUnit = itemQuery.value("price_per_unit").toDouble();
            itemInfo.bookTitle = itemQuery.value("title").toString();
            orderInfo.items.append(itemInfo);
        }
        qInfo() << "Fetched" << orderInfo.items.size() << "items for order ID:" << orderId;
    }

    // 3. Отримуємо статуси для цього замовлення
    const QString statusesSql = getSqlQuery("GetOrderStatusesByOrderId");
    if (statusesSql.isEmpty()) return orderInfo; // Помилка завантаження запиту, повертаємо що є

    QSqlQuery statusQuery(m_db);
     if (!statusQuery.prepare(statusesSql)) {
         qCritical() << "Помилка підготовки запиту 'GetOrderStatusesByOrderId':" << statusQuery.lastError().text();
         return orderInfo; // Повертаємо те, що є
     }
    statusQuery.bindValue(":orderId", orderId);
    qInfo() << "Executing SQL 'GetOrderStatusesByOrderId' for order ID:" << orderId;
    if (!statusQuery.exec()) {
        qCritical() << "Помилка при виконанні 'GetOrderStatusesByOrderId' для order ID '" << orderId << "':";
        qCritical() << statusQuery.lastError().text();
    } else {
        while (statusQuery.next()) {
            OrderStatusDisplayInfo statusInfo;
            statusInfo.status = statusQuery.value("status").toString();
            statusInfo.statusDate = statusQuery.value("status_date").toDateTime();
            statusInfo.trackingNumber = statusQuery.value("tracking_number").toString();
            orderInfo.statuses.append(statusInfo);
        }
        qInfo() << "Fetched" << orderInfo.statuses.size() << "statuses for order ID:" << orderId;
    }

    return orderInfo;
}

// Реалізація оновленого методу для створення замовлення (повертає double)
double DatabaseManager::createOrder(int customerId, const QMap<int, int> &items, const QString &shippingAddress, const QString &paymentMethod, int &newOrderId)
{
    newOrderId = -1;
    double calculatedTotalAmount = 0.0; // Ініціалізуємо тут
    const double errorReturnValue = -1.0; // Значення, що повертається при помилці

    if (!m_isConnected || !m_db.isOpen()) {
        qWarning() << "Неможливо створити замовлення: немає з'єднання з БД.";
        return errorReturnValue;
    }
    if (customerId <= 0 || items.isEmpty() || shippingAddress.isEmpty()) {
        qWarning() << "Неможливо створити замовлення: невірний ID користувача, порожній кошик або не вказано адресу.";
        return errorReturnValue;
    }

    // Починаємо транзакцію
    if (!m_db.transaction()) {
        qCritical() << "Не вдалося почати транзакцію для створення замовлення:" << m_db.lastError().text();
        return errorReturnValue;
    }
    qInfo() << "Транзакція для створення замовлення розпочата...";

    QSqlQuery query(m_db); // Основний запит для заголовка, оновлення суми, статусу
    bool success = true;
    // double calculatedTotalAmount = 0.0; // Перенесено вище
    QVariant lastId;

    // 1. Створюємо запис в таблиці "order" (поки що з total_amount = 0)
    const QString insertOrderSQL = getSqlQuery("InsertOrderHeader");
    if (insertOrderSQL.isEmpty()) {
        success = false; // Помилка завантаження запиту
    } else if (!query.prepare(insertOrderSQL)) {
        qCritical() << "Помилка підготовки запиту 'InsertOrderHeader':" << query.lastError().text();
        success = false;
    } else {
        qInfo() << "Executing SQL 'InsertOrderHeader' for customer ID:" << customerId;
        query.bindValue(":customer_id", customerId);
        query.bindValue(":shipping_address", shippingAddress);
        query.bindValue(":payment_method", paymentMethod.isEmpty() ? QVariant(QVariant::String) : paymentMethod);

        if (executeInsertQuery(query, "Insert Order Header", lastId)) {
            newOrderId = lastId.toInt();
            qInfo() << "Створено заголовок замовлення з ID:" << newOrderId;
        } else {
            success = false;
        }
    }

    // 2. Додаємо позиції в order_item та розраховуємо суму
    if (success) {
        const QString insertItemSQL = getSqlQuery("InsertOrderItem");
        const QString getBookPriceSQL = getSqlQuery("GetBookPriceAndStockForUpdate");
        const QString updateStockSQL = getSqlQuery("UpdateBookStock");

        if (insertItemSQL.isEmpty() || getBookPriceSQL.isEmpty() || updateStockSQL.isEmpty()) {
             qCritical() << "Помилка завантаження SQL запитів для створення позицій замовлення.";
             success = false;
        } else {
            QSqlQuery itemQuery(m_db); // Окремий запит для вставки позицій
            QSqlQuery priceQuery(m_db); // Окремий запит для отримання ціни та кількості
            QSqlQuery updateStockQuery(m_db); // Окремий запит для оновлення кількості

            if (!itemQuery.prepare(insertItemSQL) || !priceQuery.prepare(getBookPriceSQL) || !updateStockQuery.prepare(updateStockSQL)) {
                qCritical() << "Помилка підготовки запитів для позицій замовлення, ціни або оновлення кількості:"
                            << itemQuery.lastError().text() << priceQuery.lastError().text() << updateStockQuery.lastError().text();
                success = false;
            } else {
                for (auto it = items.constBegin(); it != items.constEnd() && success; ++it) {
                int bookId = it.key();
                int quantity = it.value();

                if (quantity <= 0) {
                    qWarning() << "Пропущено позицію з невірною кількістю (" << quantity << ") для книги ID" << bookId;
                    continue;
                }

                // Отримуємо актуальну ціну та кількість на складі
                qInfo() << "Executing SQL 'GetBookPriceAndStockForUpdate' for book ID:" << bookId;
                priceQuery.bindValue(":book_id", bookId);
                if (!priceQuery.exec()) {
                    qCritical() << "Помилка виконання 'GetBookPriceAndStockForUpdate' для книги ID" << bookId << ":" << priceQuery.lastError().text();
                    success = false;
                    break;
                }

                if (priceQuery.next()) {
                    double currentPrice = priceQuery.value(0).toDouble();
                    int currentStock = priceQuery.value(1).toInt();

                    // Перевірка наявності
                    if (quantity > currentStock) {
                         qWarning() << "Недостатньо товару на складі для книги ID" << bookId << "(замовлено:" << quantity << ", на складі:" << currentStock << "). Замовлення скасовано.";
                         success = false;
                         break;
                    }

                    // Зменшуємо кількість на складі АТОМАРНО
                    qInfo() << "Executing SQL 'UpdateBookStock' for book ID:" << bookId << "Quantity:" << quantity;
                    updateStockQuery.bindValue(":quantity", quantity);
                    updateStockQuery.bindValue(":book_id", bookId);
                    if (!updateStockQuery.exec()) {
                        qCritical() << "Помилка виконання 'UpdateBookStock' для книги ID" << bookId << ":" << updateStockQuery.lastError().text();
                        success = false;
                        break;
                    }
                    // Перевіряємо, чи вдалося оновити кількість (чи не змінилася вона між SELECT і UPDATE)
                    if (updateStockQuery.numRowsAffected() == 0) {
                        qWarning() << "Не вдалося виконати 'UpdateBookStock' для книги ID" << bookId << "(можливо, кількість змінилася або недостатньо). Замовлення скасовано.";
                        success = false;
                        break;
                    }


                    // Додаємо позицію
                    itemQuery.bindValue(":order_id", newOrderId);
                    itemQuery.bindValue(":book_id", bookId);
                    itemQuery.bindValue(":quantity", quantity);
                    itemQuery.bindValue(":price_per_unit", currentPrice);

                    qInfo() << "Executing SQL 'InsertOrderItem' for order ID:" << newOrderId << "Book ID:" << bookId;
                    if (!itemQuery.exec()) {
                        qCritical() << "Помилка виконання 'InsertOrderItem' для книги ID" << bookId << ":" << itemQuery.lastError().text();
                        success = false;
                        break;
                    }
                    calculatedTotalAmount += currentPrice * quantity;
                    qInfo() << "Додано позицію: Книга ID" << bookId << ", Кількість:" << quantity << ", Ціна:" << currentPrice << ". Оновлено кількість на складі.";

                } else {
                    qWarning() << "Книгу з ID" << bookId << "не знайдено при створенні замовлення. Замовлення скасовано.";
                    success = false;
                    break;
                }
            }
        }
    }

    // 3. Оновлюємо total_amount в замовленні
    if (success) {
        const QString updateTotalSQL = getSqlQuery("UpdateOrderTotalAmount");
        if (updateTotalSQL.isEmpty()) {
            success = false; // Помилка завантаження запиту
        } else if (!query.prepare(updateTotalSQL)) {
            qCritical() << "Помилка підготовки запиту 'UpdateOrderTotalAmount':" << query.lastError().text();
            success = false;
        } else {
            qInfo() << "Executing SQL 'UpdateOrderTotalAmount' for order ID:" << newOrderId;
            query.bindValue(":total", calculatedTotalAmount);
            query.bindValue(":order_id", newOrderId);
            if (!query.exec()) {
                qCritical() << "Помилка виконання 'UpdateOrderTotalAmount' для замовлення ID" << newOrderId << ":" << query.lastError().text();
                success = false;
            } else {
                qInfo() << "Оновлено загальну суму замовлення ID" << newOrderId << " до" << calculatedTotalAmount;
            }
        }
    }

    // 4. Додаємо початковий статус замовлення
    if (success) {
        const QString insertStatusSQL = getSqlQuery("InsertOrderStatus");
         if (insertStatusSQL.isEmpty()) {
            success = false; // Помилка завантаження запиту
        } else if (!query.prepare(insertStatusSQL)) {
            qCritical() << "Помилка підготовки запиту 'InsertOrderStatus':" << query.lastError().text();
            success = false;
        } else {
            qInfo() << "Executing SQL 'InsertOrderStatus' for order ID:" << newOrderId;
            query.bindValue(":order_id", newOrderId);
            query.bindValue(":status", tr("Нове")); // Початковий статус
            if (!query.exec()) {
                qCritical() << "Помилка виконання 'InsertOrderStatus' для замовлення ID" << newOrderId << ":" << query.lastError().text();
                success = false;
            } else {
                qInfo() << "Додано початковий статус 'Нове' для замовлення ID" << newOrderId;
            }
        }
    }
    } // <--- Closing brace for the main if(success) block started on line 183


    // Завершуємо транзакцію
    if (success) {
        if (m_db.commit()) {
            qInfo() << "Транзакція створення замовлення ID" << newOrderId << "успішно завершена. Total:" << calculatedTotalAmount;
            return calculatedTotalAmount; // Повертаємо розраховану суму
        } else {
            qCritical() << "Помилка при коміті транзакції створення замовлення:" << m_db.lastError().text();
            m_db.rollback(); // Спробувати відкат
            return errorReturnValue;
        }
    } else {
        qWarning() << "Виникла помилка під час створення замовлення. Відкат транзакції...";
        if (!m_db.rollback()) {
            qCritical() << "Помилка при відкаті транзакції створення замовлення:" << m_db.lastError().text();
        } else {
            qInfo() << "Транзакція створення замовлення успішно скасована.";
        }
        newOrderId = -1; // Скидаємо ID, оскільки замовлення не створено
        return errorReturnValue;
    }
}

// Реалізація нового методу для отримання замовлень користувача
QList<OrderDisplayInfo> DatabaseManager::getCustomerOrdersForDisplay(int customerId) const
{
    QList<OrderDisplayInfo> orders;
    if (!m_isConnected || !m_db.isOpen() || customerId <= 0) {
        qWarning() << "Неможливо отримати замовлення: немає з'єднання або невірний customerId.";
        return orders;
    }

    // 1. Отримуємо основну інформацію про замовлення користувача
    const QString ordersSql = getSqlQuery("GetCustomerOrderHeadersByCustomerId");
    if (ordersSql.isEmpty()) return orders; // Помилка завантаження запиту

    QSqlQuery orderQuery(m_db);
    if (!orderQuery.prepare(ordersSql)) {
        qCritical() << "Помилка підготовки запиту 'GetCustomerOrderHeadersByCustomerId':" << orderQuery.lastError().text();
        return orders;
    }
    orderQuery.bindValue(":customerId", customerId);

    qInfo() << "Executing SQL 'GetCustomerOrderHeadersByCustomerId' for customer ID:" << customerId;
    if (!orderQuery.exec()) {
        qCritical() << "Помилка при виконанні 'GetCustomerOrderHeadersByCustomerId' для customer ID '" << customerId << "':";
        qCritical() << orderQuery.lastError().text();
        qCritical() << "SQL запит:" << orderQuery.lastQuery();
        return orders;
    }

    // Підготовлені запити для отримання позицій та статусів (для ефективності)
    // Використовуємо ті ж самі запити, що й для getOrderDetailsById
    const QString itemsSql = getSqlQuery("GetOrderItemsByOrderId");
    const QString statusesSql = getSqlQuery("GetOrderStatusesByOrderId");

    if (itemsSql.isEmpty() || statusesSql.isEmpty()) {
        qCritical() << "Помилка завантаження SQL запитів для позицій або статусів замовлень.";
        return orders; // Повертаємо те, що встигли зібрати
    }

    QSqlQuery itemQuery(m_db);
    if (!itemQuery.prepare(itemsSql)) {
         qCritical() << "Помилка підготовки запиту 'GetOrderItemsByOrderId' (для списку замовлень):" << itemQuery.lastError().text();
         return orders; // Повертаємо те, що встигли зібрати
    }

    QSqlQuery statusQuery(m_db);
     if (!statusQuery.prepare(statusesSql)) {
         qCritical() << "Помилка підготовки запиту 'GetOrderStatusesByOrderId' (для списку замовлень):" << statusQuery.lastError().text();
         return orders;
     }


    // 2. Обробляємо кожне замовлення
    qInfo() << "Processing orders for customer ID:" << customerId;
    int orderCount = 0;
    while (orderQuery.next()) {
        OrderDisplayInfo orderInfo;
        // --- Зміна: Отримуємо дату одразу як рядок ---
        QString dateString = orderQuery.value("order_date").toString();
        orderInfo.orderId = orderQuery.value("order_id").toInt();
        orderInfo.orderDate = QDateTime(); // Ініціалізуємо невалідною датою

        if (!dateString.isEmpty()) {
            // Спробуємо розпарсити рядок у QDateTime, використовуючи ISO формат (з мілісекундами або без)
            orderInfo.orderDate = QDateTime::fromString(dateString, Qt::ISODateWithMs);
            if (!orderInfo.orderDate.isValid()) { // Якщо з мілісекундами не вийшло, спробуємо без них
                orderInfo.orderDate = QDateTime::fromString(dateString, Qt::ISODate);
            }
            // Можна додати інші формати, якщо ISO не спрацює
        }
        // --- Кінець зміни ---
        orderInfo.totalAmount = orderQuery.value("total_amount").toDouble();
        orderInfo.shippingAddress = orderQuery.value("shipping_address").toString();
        orderInfo.paymentMethod = orderQuery.value("payment_method").toString();
       // --- Оновлена Відладка дати ---
       qDebug() << "[DEBUG] Customer Order ID:" << orderInfo.orderId
                << "Raw order_date value (CAST to TEXT in SQL):" << dateString; // Показуємо рядок, отриманий з запиту
       qDebug() << "[DEBUG] Parsed QDateTime (after string parse attempts):" << orderInfo.orderDate
                << "Is Valid:" << orderInfo.orderDate.isValid();
       if (!orderInfo.orderDate.isValid() && !dateString.isEmpty()) {
            qWarning() << "[DEBUG] Failed to parse date string:" << dateString << "using ISODate/ISODateWithMs formats.";
       }
       // --- Кінець відладки ---

        // 3. Отримуємо позиції для поточного замовлення
        itemQuery.bindValue(":orderId", orderInfo.orderId);
        qInfo() << "Executing SQL 'GetOrderItemsByOrderId' for order ID:" << orderInfo.orderId << "(in list)";
        if (!itemQuery.exec()) {
            qCritical() << "Помилка при виконанні 'GetOrderItemsByOrderId' для order ID '" << orderInfo.orderId << "':";
            qCritical() << itemQuery.lastError().text();
            // Продовжуємо до наступного замовлення або повертаємо помилку
            continue;
        }
        while (itemQuery.next()) {
            OrderItemDisplayInfo itemInfo;
            itemInfo.quantity = itemQuery.value("quantity").toInt();
            itemInfo.pricePerUnit = itemQuery.value("price_per_unit").toDouble();
            itemInfo.bookTitle = itemQuery.value("title").toString();
            orderInfo.items.append(itemInfo);
        }

        // 4. Отримуємо статуси для поточного замовлення
        statusQuery.bindValue(":orderId", orderInfo.orderId);
        qInfo() << "Executing SQL 'GetOrderStatusesByOrderId' for order ID:" << orderInfo.orderId << "(in list)";
         if (!statusQuery.exec()) {
            qCritical() << "Помилка при виконанні 'GetOrderStatusesByOrderId' для order ID '" << orderInfo.orderId << "':";
            qCritical() << statusQuery.lastError().text();
            // Продовжуємо
            continue;
        }
        while (statusQuery.next()) {
            OrderStatusDisplayInfo statusInfo;
            statusInfo.status = statusQuery.value("status").toString();
            statusInfo.statusDate = statusQuery.value("status_date").toDateTime();
            statusInfo.trackingNumber = statusQuery.value("tracking_number").toString();
            orderInfo.statuses.append(statusInfo);
        }

        orders.append(orderInfo);
        orderCount++;
    }

    qInfo() << "Processed" << orderCount << "orders for customer ID:" << customerId;
    return orders;
}
