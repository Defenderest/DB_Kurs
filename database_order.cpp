#include "database.h"
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QMap>
#include <QDateTime>

OrderDisplayInfo DatabaseManager::getOrderDetailsById(int orderId) const
{
    OrderDisplayInfo orderInfo;
    orderInfo.orderId = -1;

    if (!m_isConnected || !m_db.isOpen() || orderId <= 0) {
        qWarning() << "Неможливо отримати деталі замовлення: немає з'єднання або невірний orderId.";
        return orderInfo;
    }

    const QString orderSql = getSqlQuery("GetOrderHeaderById");
    if (orderSql.isEmpty()) return orderInfo;

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
        QString dateString = orderQuery.value("order_date").toString();
        orderInfo.orderId = orderQuery.value("order_id").toInt();
        orderInfo.orderDate = QDateTime();

        if (!dateString.isEmpty()) {
            orderInfo.orderDate = QDateTime::fromString(dateString, Qt::ISODateWithMs);
            if (!orderInfo.orderDate.isValid()) {
                orderInfo.orderDate = QDateTime::fromString(dateString, Qt::ISODate);
            }
        }
        orderInfo.totalAmount = orderQuery.value("total_amount").toDouble();
        orderInfo.shippingAddress = orderQuery.value("shipping_address").toString();
        orderInfo.paymentMethod = orderQuery.value("payment_method").toString();
     qDebug() << "[DEBUG] Order ID:" << orderInfo.orderId
              << "Raw order_date value (CAST to TEXT in SQL):" << dateString;
     qDebug() << "[DEBUG] Parsed QDateTime (after string parse attempts):" << orderInfo.orderDate
              << "Is Valid:" << orderInfo.orderDate.isValid();
     if (!orderInfo.orderDate.isValid() && !dateString.isEmpty()) {
          qWarning() << "[DEBUG] Failed to parse date string:" << dateString << "using ISODate/ISODateWithMs formats.";
       }
       orderInfo.found = true;
        qInfo() << "Order header found for ID:" << orderId;
    } else {
        qWarning() << "Order not found for ID:" << orderId;
        return orderInfo;
    }

    const QString itemsSql = getSqlQuery("GetOrderItemsByOrderId");
    if (itemsSql.isEmpty()) return orderInfo;

    QSqlQuery itemQuery(m_db);
    if (!itemQuery.prepare(itemsSql)) {
         qCritical() << "Помилка підготовки запиту 'GetOrderItemsByOrderId':" << itemQuery.lastError().text();
         return orderInfo;
    }
    itemQuery.bindValue(":orderId", orderId);
    qInfo() << "Executing SQL 'GetOrderItemsByOrderId' for order ID:" << orderId;
    if (!itemQuery.exec()) {
        qCritical() << "Помилка при виконанні 'GetOrderItemsByOrderId' для order ID '" << orderId << "':";
        qCritical() << itemQuery.lastError().text();
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

    const QString statusesSql = getSqlQuery("GetOrderStatusesByOrderId");
    if (statusesSql.isEmpty()) return orderInfo;

    QSqlQuery statusQuery(m_db);
     if (!statusQuery.prepare(statusesSql)) {
         qCritical() << "Помилка підготовки запиту 'GetOrderStatusesByOrderId':" << statusQuery.lastError().text();
         return orderInfo;
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

double DatabaseManager::createOrder(int customerId, const QMap<int, int> &items, const QString &shippingAddress, const QString &paymentMethod, int &newOrderId)
{
    newOrderId = -1;
    double calculatedTotalAmount = 0.0;
    const double errorReturnValue = -1.0;

    if (!m_isConnected || !m_db.isOpen()) {
        qWarning() << "Неможливо створити замовлення: немає з'єднання з БД.";
        return errorReturnValue;
    }
    if (customerId <= 0 || items.isEmpty() || shippingAddress.isEmpty()) {
        qWarning() << "Неможливо створити замовлення: невірний ID користувача, порожній кошик або не вказано адресу.";
        return errorReturnValue;
    }

    if (!m_db.transaction()) {
        qCritical() << "Не вдалося почати транзакцію для створення замовлення:" << m_db.lastError().text();
        return errorReturnValue;
    }
    qInfo() << "Транзакція для створення замовлення розпочата...";

    QSqlQuery query(m_db);
    bool success = true;
    QVariant lastId;

    const QString insertOrderSQL = getSqlQuery("InsertOrderHeader");
    if (insertOrderSQL.isEmpty()) {
        success = false;
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

    if (success) {
        const QString insertItemSQL = getSqlQuery("InsertOrderItem");
        const QString getBookPriceSQL = getSqlQuery("GetBookPriceAndStockForUpdate");
        const QString updateStockSQL = getSqlQuery("UpdateBookStock");

        if (insertItemSQL.isEmpty() || getBookPriceSQL.isEmpty() || updateStockSQL.isEmpty()) {
             qCritical() << "Помилка завантаження SQL запитів для створення позицій замовлення.";
             success = false;
        } else {
            QSqlQuery itemQuery(m_db);
            QSqlQuery priceQuery(m_db);
            QSqlQuery updateStockQuery(m_db);

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

                    if (quantity > currentStock) {
                         qWarning() << "Недостатньо товару на складі для книги ID" << bookId << "(замовлено:" << quantity << ", на складі:" << currentStock << "). Замовлення скасовано.";
                         success = false;
                         break;
                    }

                    qInfo() << "Executing SQL 'UpdateBookStock' for book ID:" << bookId << "Quantity:" << quantity;
                    updateStockQuery.bindValue(":quantity", quantity);
                    updateStockQuery.bindValue(":book_id", bookId);
                    if (!updateStockQuery.exec()) {
                        qCritical() << "Помилка виконання 'UpdateBookStock' для книги ID" << bookId << ":" << updateStockQuery.lastError().text();
                        success = false;
                        break;
                    }
                    if (updateStockQuery.numRowsAffected() == 0) {
                        qWarning() << "Не вдалося виконати 'UpdateBookStock' для книги ID" << bookId << "(можливо, кількість змінилася або недостатньо). Замовлення скасовано.";
                        success = false;
                        break;
                    }


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

    if (success) {
        const QString updateTotalSQL = getSqlQuery("UpdateOrderTotalAmount");
        if (updateTotalSQL.isEmpty()) {
            success = false;
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

    if (success) {
        const QString insertStatusSQL = getSqlQuery("InsertOrderStatus");
         if (insertStatusSQL.isEmpty()) {
            success = false;
        } else if (!query.prepare(insertStatusSQL)) {
            qCritical() << "Помилка підготовки запиту 'InsertOrderStatus':" << query.lastError().text();
            success = false;
        } else {
            qInfo() << "Executing SQL 'InsertOrderStatus' for order ID:" << newOrderId;
            query.bindValue(":order_id", newOrderId);
            query.bindValue(":status", tr("Нове"));
            if (!query.exec()) {
                qCritical() << "Помилка виконання 'InsertOrderStatus' для замовлення ID" << newOrderId << ":" << query.lastError().text();
                success = false;
            } else {
                qInfo() << "Додано початковий статус 'Нове' для замовлення ID" << newOrderId;
            }
        }
    }
    }


    if (success) {
        if (m_db.commit()) {
            qInfo() << "Транзакція створення замовлення ID" << newOrderId << "успішно завершена. Total:" << calculatedTotalAmount;
            return calculatedTotalAmount;
        } else {
            qCritical() << "Помилка при коміті транзакції створення замовлення:" << m_db.lastError().text();
            m_db.rollback();
            return errorReturnValue;
        }
    } else {
        qWarning() << "Виникла помилка під час створення замовлення. Відкат транзакції...";
        if (!m_db.rollback()) {
            qCritical() << "Помилка при відкаті транзакції створення замовлення:" << m_db.lastError().text();
        } else {
            qInfo() << "Транзакція створення замовлення успішно скасована.";
        }
        newOrderId = -1;
        return errorReturnValue;
    }
}

QList<OrderDisplayInfo> DatabaseManager::getCustomerOrdersForDisplay(int customerId) const
{
    QList<OrderDisplayInfo> orders;
    if (!m_isConnected || !m_db.isOpen() || customerId <= 0) {
        qWarning() << "Неможливо отримати замовлення: немає з'єднання або невірний customerId.";
        return orders;
    }

    const QString ordersSql = getSqlQuery("GetCustomerOrderHeadersByCustomerId");
    if (ordersSql.isEmpty()) return orders;

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

    const QString itemsSql = getSqlQuery("GetOrderItemsByOrderId");
    const QString statusesSql = getSqlQuery("GetOrderStatusesByOrderId");

    if (itemsSql.isEmpty() || statusesSql.isEmpty()) {
        qCritical() << "Помилка завантаження SQL запитів для позицій або статусів замовлень.";
        return orders;
    }

    QSqlQuery itemQuery(m_db);
    if (!itemQuery.prepare(itemsSql)) {
         qCritical() << "Помилка підготовки запиту 'GetOrderItemsByOrderId' (для списку замовлень):" << itemQuery.lastError().text();
         return orders;
    }

    QSqlQuery statusQuery(m_db);
     if (!statusQuery.prepare(statusesSql)) {
         qCritical() << "Помилка підготовки запиту 'GetOrderStatusesByOrderId' (для списку замовлень):" << statusQuery.lastError().text();
         return orders;
     }


    qInfo() << "Processing orders for customer ID:" << customerId;
    int orderCount = 0;
    while (orderQuery.next()) {
        OrderDisplayInfo orderInfo;
        QString dateString = orderQuery.value("order_date").toString();
        orderInfo.orderId = orderQuery.value("order_id").toInt();
        orderInfo.orderDate = QDateTime();

        if (!dateString.isEmpty()) {
            orderInfo.orderDate = QDateTime::fromString(dateString, Qt::ISODateWithMs);
            if (!orderInfo.orderDate.isValid()) {
                orderInfo.orderDate = QDateTime::fromString(dateString, Qt::ISODate);
            }
        }
        orderInfo.totalAmount = orderQuery.value("total_amount").toDouble();
        orderInfo.shippingAddress = orderQuery.value("shipping_address").toString();
        orderInfo.paymentMethod = orderQuery.value("payment_method").toString();
       qDebug() << "[DEBUG] Customer Order ID:" << orderInfo.orderId
                << "Raw order_date value (CAST to TEXT in SQL):" << dateString;
       qDebug() << "[DEBUG] Parsed QDateTime (after string parse attempts):" << orderInfo.orderDate
                << "Is Valid:" << orderInfo.orderDate.isValid();
       if (!orderInfo.orderDate.isValid() && !dateString.isEmpty()) {
            qWarning() << "[DEBUG] Failed to parse date string:" << dateString << "using ISODate/ISODateWithMs formats.";
       }

        itemQuery.bindValue(":orderId", orderInfo.orderId);
        qInfo() << "Executing SQL 'GetOrderItemsByOrderId' for order ID:" << orderInfo.orderId << "(in list)";
        if (!itemQuery.exec()) {
            qCritical() << "Помилка при виконанні 'GetOrderItemsByOrderId' для order ID '" << orderInfo.orderId << "':";
            qCritical() << itemQuery.lastError().text();
            continue;
        }
        while (itemQuery.next()) {
            OrderItemDisplayInfo itemInfo;
            itemInfo.quantity = itemQuery.value("quantity").toInt();
            itemInfo.pricePerUnit = itemQuery.value("price_per_unit").toDouble();
            itemInfo.bookTitle = itemQuery.value("title").toString();
            orderInfo.items.append(itemInfo);
        }

        statusQuery.bindValue(":orderId", orderInfo.orderId);
        qInfo() << "Executing SQL 'GetOrderStatusesByOrderId' for order ID:" << orderInfo.orderId << "(in list)";
         if (!statusQuery.exec()) {
            qCritical() << "Помилка при виконанні 'GetOrderStatusesByOrderId' для order ID '" << orderInfo.orderId << "':";
            qCritical() << statusQuery.lastError().text();
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
