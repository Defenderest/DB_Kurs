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

    // 1. Отримуємо основну інформацію про замовлення (з явним кастом дати до тексту)
    const QString orderSql = R"(
        SELECT order_id, order_date::text, total_amount, shipping_address, payment_method
        FROM "order"
        WHERE order_id = :orderId;
    )";

    QSqlQuery orderQuery(m_db);
    orderQuery.prepare(orderSql);
    orderQuery.bindValue(":orderId", orderId);

    qInfo() << "Executing SQL to get details for order ID:" << orderId;
    if (!orderQuery.exec()) {
        qCritical() << "Помилка при отриманні деталей замовлення для order ID '" << orderId << "':";
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
    QSqlQuery itemQuery(m_db);
    const QString itemsSql = R"(
        SELECT oi.quantity, oi.price_per_unit, b.title
        FROM order_item oi
        JOIN book b ON oi.book_id = b.book_id
        WHERE oi.order_id = :orderId;
    )";
    if (!itemQuery.prepare(itemsSql)) {
         qCritical() << "Помилка підготовки запиту для order_item (details):" << itemQuery.lastError().text();
         return orderInfo; // Повертаємо те, що є
    }
    itemQuery.bindValue(":orderId", orderId);
    if (!itemQuery.exec()) {
        qCritical() << "Помилка при отриманні позицій для order ID '" << orderId << "':";
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
    QSqlQuery statusQuery(m_db);
    const QString statusesSql = R"(
        SELECT status, status_date, tracking_number
        FROM order_status
        WHERE order_id = :orderId
        ORDER BY status_date ASC;
    )";
     if (!statusQuery.prepare(statusesSql)) {
         qCritical() << "Помилка підготовки запиту для order_status (details):" << statusQuery.lastError().text();
         return orderInfo; // Повертаємо те, що є
     }
    statusQuery.bindValue(":orderId", orderId);
    if (!statusQuery.exec()) {
        qCritical() << "Помилка при отриманні статусів для order ID '" << orderId << "':";
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
    const QString insertOrderSQL = R"(
        INSERT INTO "order" (customer_id, order_date, total_amount, shipping_address, payment_method)
        VALUES (:customer_id, CURRENT_TIMESTAMP, 0.0, :shipping_address, :payment_method)
        RETURNING order_id;
    )";
    if (!query.prepare(insertOrderSQL)) {
        qCritical() << "Помилка підготовки запиту для вставки замовлення:" << query.lastError().text();
        success = false;
    } else {
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
        const QString insertItemSQL = R"(
            INSERT INTO order_item (order_id, book_id, quantity, price_per_unit)
            VALUES (:order_id, :book_id, :quantity, :price_per_unit);
        )";
        const QString getBookPriceSQL = "SELECT price, stock_quantity FROM book WHERE book_id = :book_id FOR UPDATE;"; // FOR UPDATE для блокування рядка
        const QString updateStockSQL = "UPDATE book SET stock_quantity = stock_quantity - :quantity WHERE book_id = :book_id AND stock_quantity >= :quantity;"; // Оновлення кількості

        QSqlQuery itemQuery(m_db); // Окремий запит для вставки позицій
        QSqlQuery priceQuery(m_db); // Окремий запит для отримання ціни та кількості
        QSqlQuery updateStockQuery(m_db); // Окремий запит для оновлення кількості

        if (!itemQuery.prepare(insertItemSQL) || !priceQuery.prepare(getBookPriceSQL) || !updateStockQuery.prepare(updateStockSQL)) {
            qCritical() << "Помилка підготовки запиту для позицій замовлення, ціни або оновлення кількості:"
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
                priceQuery.bindValue(":book_id", bookId);
                if (!priceQuery.exec()) {
                    qCritical() << "Помилка отримання ціни/кількості для книги ID" << bookId << ":" << priceQuery.lastError().text();
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
                    updateStockQuery.bindValue(":quantity", quantity);
                    updateStockQuery.bindValue(":book_id", bookId);
                    if (!updateStockQuery.exec()) {
                        qCritical() << "Помилка оновлення кількості на складі для книги ID" << bookId << ":" << updateStockQuery.lastError().text();
                        success = false;
                        break;
                    }
                    // Перевіряємо, чи вдалося оновити кількість (чи не змінилася вона між SELECT і UPDATE)
                    if (updateStockQuery.numRowsAffected() == 0) {
                        qWarning() << "Не вдалося оновити кількість на складі для книги ID" << bookId << "(можливо, кількість змінилася або недостатньо). Замовлення скасовано.";
                        success = false;
                        break;
                    }


                    // Додаємо позицію
                    itemQuery.bindValue(":order_id", newOrderId);
                    itemQuery.bindValue(":book_id", bookId);
                    itemQuery.bindValue(":quantity", quantity);
                    itemQuery.bindValue(":price_per_unit", currentPrice);

                    if (!itemQuery.exec()) {
                        qCritical() << "Помилка вставки позиції замовлення для книги ID" << bookId << ":" << itemQuery.lastError().text();
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
        const QString updateTotalSQL = R"(UPDATE "order" SET total_amount = :total WHERE order_id = :order_id;)";
        if (!query.prepare(updateTotalSQL)) {
            qCritical() << "Помилка підготовки запиту для оновлення суми замовлення:" << query.lastError().text();
            success = false;
        } else {
            query.bindValue(":total", calculatedTotalAmount);
            query.bindValue(":order_id", newOrderId);
            if (!query.exec()) {
                qCritical() << "Помилка оновлення суми замовлення ID" << newOrderId << ":" << query.lastError().text();
                success = false;
            } else {
                qInfo() << "Оновлено загальну суму замовлення ID" << newOrderId << "до" << calculatedTotalAmount;
            }
        }
    }

    // 4. Додаємо початковий статус замовлення
    if (success) {
        const QString insertStatusSQL = R"(
            INSERT INTO order_status (order_id, status, status_date)
            VALUES (:order_id, :status, CURRENT_TIMESTAMP);
        )";
        if (!query.prepare(insertStatusSQL)) {
            qCritical() << "Помилка підготовки запиту для вставки статусу замовлення:" << query.lastError().text();
            success = false;
        } else {
            query.bindValue(":order_id", newOrderId);
            query.bindValue(":status", tr("Нове")); // Початковий статус
            if (!query.exec()) {
                qCritical() << "Помилка вставки статусу для замовлення ID" << newOrderId << ":" << query.lastError().text();
                success = false;
            } else {
                qInfo() << "Додано початковий статус 'Нове' для замовлення ID" << newOrderId;
            }
        }
    }


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

    // 1. Отримуємо основну інформацію про замовлення користувача (з явним кастом дати до тексту)
    const QString ordersSql = R"(
        SELECT order_id, order_date::text, total_amount, shipping_address, payment_method
        FROM "order"
        WHERE customer_id = :customerId
        ORDER BY order_date DESC; -- Показуємо новіші замовлення першими
    )";

    QSqlQuery orderQuery(m_db);
    orderQuery.prepare(ordersSql);
    orderQuery.bindValue(":customerId", customerId);

    qInfo() << "Executing SQL to get orders for customer ID:" << customerId;
    if (!orderQuery.exec()) {
        qCritical() << "Помилка при отриманні замовлень для customer ID '" << customerId << "':";
        qCritical() << orderQuery.lastError().text();
        qCritical() << "SQL запит:" << orderQuery.lastQuery();
        return orders;
    }

    // Підготовлені запити для отримання позицій та статусів (для ефективності)
    QSqlQuery itemQuery(m_db);
    const QString itemsSql = R"(
        SELECT oi.quantity, oi.price_per_unit, b.title
        FROM order_item oi
        JOIN book b ON oi.book_id = b.book_id
        WHERE oi.order_id = :orderId;
    )";
    if (!itemQuery.prepare(itemsSql)) {
         qCritical() << "Помилка підготовки запиту для order_item:" << itemQuery.lastError().text();
         return orders; // Повертаємо те, що встигли зібрати, або порожній список
    }

    QSqlQuery statusQuery(m_db);
    const QString statusesSql = R"(
        SELECT status, status_date, tracking_number
        FROM order_status
        WHERE order_id = :orderId
        ORDER BY status_date ASC; -- Показуємо статуси в хронологічному порядку
    )";
     if (!statusQuery.prepare(statusesSql)) {
         qCritical() << "Помилка підготовки запиту для order_status:" << statusQuery.lastError().text();
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
        if (!itemQuery.exec()) {
            qCritical() << "Помилка при отриманні позицій для order ID '" << orderInfo.orderId << "':";
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
         if (!statusQuery.exec()) {
            qCritical() << "Помилка при отриманні статусів для order ID '" << orderInfo.orderId << "':";
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
