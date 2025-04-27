#include "database.h"
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QVector>
#include <QDate>
#include <QDateTime>
#include <QSqlRecord> // Needed for query.record()
// #include <QRandomGenerator> // Більше не потрібен тут
// #include <QSet>             // Більше не потрібен тут
#include <QMap> // Потрібен для createOrder

// Допоміжні функції randomDate, randomDateTime та структури PublisherData, AuthorData, BookData
// тепер знаходяться в testdata.h/cpp

DatabaseManager::DatabaseManager(QObject *parent) : QObject(parent)
{
    if (!QSqlDatabase::isDriverAvailable("QPSQL")) {
        qCritical() << "Error: QPSQL driver for PostgreSQL is not available!";
        qCritical() << "Available drivers:" << QSqlDatabase::drivers();
    }
    // Initialization of the random number generator (optional, QRandomGenerator::global() self-initializes)
    // qsrand(QTime::currentTime().msec());
}

DatabaseManager::~DatabaseManager()
{
    closeConnection();
}

// ... connectToDatabase и createSchemaTables остаются как были ...
bool DatabaseManager::connectToDatabase(const QString &host,
                                        int port,
                                        const QString &dbName,
                                        const QString &user,
                                        const QString &password)
{
    closeConnection();
    m_db = QSqlDatabase::addDatabase("QPSQL", "my_postgres_connection");
    m_db.setHostName(host);
    m_db.setPort(port);
    m_db.setDatabaseName(dbName);
    m_db.setUserName(user);
    m_db.setPassword(password);

    if (!m_db.open()) {
        qCritical() << "Не удалось подключиться к базе данных:";
        qCritical() << m_db.lastError().text();
        m_isConnected = false;
        QSqlDatabase::removeDatabase("my_postgres_connection");
        return false;
    }

    qDebug() << "Успешно подключено к базе данных" << dbName << "на" << host << ":" << port;
    m_isConnected = true;
    return true;
}

bool DatabaseManager::createSchemaTables()
{
    if (!m_isConnected || !m_db.isOpen()) {
        qWarning() << "Невозможно создать таблицы: нет активного соединения с БД.";
        return false;
    }

    // Начинаем транзакцию. Если что-то пойдет не так, все изменения откатятся.
    if (!m_db.transaction()) {
        qCritical() << "Не удалось начать транзакцию:" << m_db.lastError().text();
        return false;
    }
    qInfo() << "Транзакция начата для создания схемы...";

    QSqlQuery query(m_db);
    bool success = true;

    // --- SQL Запросы для создания таблиц (порядок важен!) ---
    // (Код для DROP и CREATE таблиц как в предыдущем ответе)
    // 1. Удаление существующих таблиц (если нужно начать с чистого листа)
    const QString dropOrderStatusSQL = R"(DROP TABLE IF EXISTS order_status CASCADE;)";
    const QString dropOrderItemSQL = R"(DROP TABLE IF EXISTS order_item CASCADE;)";
    const QString dropBookAuthorSQL = R"(DROP TABLE IF EXISTS book_author CASCADE;)";
    const QString dropOrderSQL = R"(DROP TABLE IF EXISTS "order" CASCADE;)"; // Имя в кавычках
    const QString dropBookSQL = R"(DROP TABLE IF EXISTS book CASCADE;)";
    const QString dropAuthorSQL = R"(DROP TABLE IF EXISTS author CASCADE;)";
    const QString dropPublisherSQL = R"(DROP TABLE IF EXISTS publisher CASCADE;)";
    const QString dropCustomerSQL = R"(DROP TABLE IF EXISTS customer CASCADE;)";
    const QString dropCommentSQL = R"(DROP TABLE IF EXISTS comment CASCADE;)"; // Додано видалення comment


    success &= executeQuery(query, dropOrderStatusSQL, "Удаление order_status");
    if(success) success &= executeQuery(query, dropOrderItemSQL,   "Удаление order_item");
    if(success) success &= executeQuery(query, dropCommentSQL,     "Удаление comment"); // Додано видалення comment
    if(success) success &= executeQuery(query, dropBookAuthorSQL,  "Удаление book_author");
    if(success) success &= executeQuery(query, dropOrderSQL,       "Удаление \"order\"");
    if(success) success &= executeQuery(query, dropBookSQL,        "Удаление book");
    if(success) success &= executeQuery(query, dropAuthorSQL,      "Удаление author");
    if(success) success &= executeQuery(query, dropPublisherSQL,   "Удаление publisher");
    if(success) success &= executeQuery(query, dropCustomerSQL,    "Удаление customer");


    // 2. Создание таблиц
    const QString createCustomerSQL = R"(
        CREATE TABLE customer (
            customer_id SERIAL PRIMARY KEY, first_name VARCHAR(100) NOT NULL, last_name VARCHAR(100) NOT NULL,
            email VARCHAR(255) UNIQUE NOT NULL, phone VARCHAR(30), address TEXT,
            password_hash VARCHAR(64) NOT NULL, -- Додано поле для хешу пароля (SHA-256 = 64 hex chars)
            loyalty_program BOOLEAN DEFAULT FALSE, join_date DATE NOT NULL DEFAULT CURRENT_DATE,
            loyalty_points INTEGER DEFAULT 0 CHECK (loyalty_points >= 0) ); )";
    if(success) success &= executeQuery(query, createCustomerSQL, "Создание customer");

    const QString createPublisherSQL = R"(
        CREATE TABLE publisher ( publisher_id SERIAL PRIMARY KEY, name VARCHAR(255) NOT NULL UNIQUE, contact_info TEXT ); )";
    if(success) success &= executeQuery(query, createPublisherSQL, "Создание publisher");

    const QString createAuthorSQL = R"(
        CREATE TABLE author ( author_id SERIAL PRIMARY KEY, first_name VARCHAR(100) NOT NULL, last_name VARCHAR(100) NOT NULL,
            birth_date DATE, nationality VARCHAR(100), image_path VARCHAR(512) ); )"; // Додано image_path
    if(success) success &= executeQuery(query, createAuthorSQL, "Создание author");

    const QString createBookSQL = R"(
        CREATE TABLE book ( book_id SERIAL PRIMARY KEY, title VARCHAR(255) NOT NULL, isbn VARCHAR(20) UNIQUE,
            publication_date DATE, publisher_id INTEGER, price NUMERIC(10, 2) CHECK (price >= 0),
            stock_quantity INTEGER DEFAULT 0 CHECK (stock_quantity >= 0), description TEXT, language VARCHAR(50), -- Description already exists here, no change needed in CREATE TABLE
            page_count INTEGER CHECK (page_count > 0),
            cover_image_path VARCHAR(512),
            genre VARCHAR(100), -- Додано поле для жанру
            CONSTRAINT fk_publisher FOREIGN KEY (publisher_id) REFERENCES publisher(publisher_id) ON DELETE SET NULL ); )";
    if(success) success &= executeQuery(query, createBookSQL, "Создание book");

    const QString createOrderSQL = R"(
        CREATE TABLE "order" ( order_id SERIAL PRIMARY KEY, customer_id INTEGER,
            order_date TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP, total_amount NUMERIC(12, 2) CHECK (total_amount >= 0),
            shipping_address TEXT NOT NULL, payment_method VARCHAR(50),
            CONSTRAINT fk_customer FOREIGN KEY (customer_id) REFERENCES customer(customer_id) ON DELETE SET NULL ); )";
    if(success) success &= executeQuery(query, createOrderSQL, "Создание \"order\""); // Имя в кавычках

    const QString createBookAuthorSQL = R"(
        CREATE TABLE book_author ( book_id INTEGER NOT NULL, author_id INTEGER NOT NULL, role VARCHAR(100),
            PRIMARY KEY (book_id, author_id),
            CONSTRAINT fk_book FOREIGN KEY (book_id) REFERENCES book(book_id) ON DELETE CASCADE,
            CONSTRAINT fk_author FOREIGN KEY (author_id) REFERENCES author(author_id) ON DELETE CASCADE ); )";
    if(success) success &= executeQuery(query, createBookAuthorSQL, "Создание book_author");

    const QString createOrderItemSQL = R"(
        CREATE TABLE order_item ( order_item_id SERIAL PRIMARY KEY, order_id INTEGER NOT NULL, book_id INTEGER NOT NULL,
            quantity INTEGER NOT NULL CHECK (quantity > 0), price_per_unit NUMERIC(10, 2) NOT NULL CHECK (price_per_unit >= 0),
            CONSTRAINT fk_order FOREIGN KEY (order_id) REFERENCES "order"(order_id) ON DELETE CASCADE,
            CONSTRAINT fk_book FOREIGN KEY (book_id) REFERENCES book(book_id) ON DELETE RESTRICT ); )";
    if(success) success &= executeQuery(query, createOrderItemSQL, "Создание order_item");

    const QString createOrderStatusSQL = R"(
        CREATE TABLE order_status ( order_status_id SERIAL PRIMARY KEY, order_id INTEGER NOT NULL, status VARCHAR(50) NOT NULL,
            status_date TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP, tracking_number VARCHAR(100),
            CONSTRAINT fk_order FOREIGN KEY (order_id) REFERENCES "order"(order_id) ON DELETE CASCADE ); )";
    if(success) success &= executeQuery(query, createOrderStatusSQL, "Создание order_status");

    const QString createCommentSQL = R"(
        CREATE TABLE comment (
            comment_id SERIAL PRIMARY KEY,
            book_id INTEGER NOT NULL,
            customer_id INTEGER NOT NULL,
            comment_text TEXT NOT NULL,
            comment_date TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
            rating INTEGER CHECK (rating >= 0 AND rating <= 5), -- 0 = no rating, 1-5 stars
            CONSTRAINT fk_book_comment FOREIGN KEY (book_id) REFERENCES book(book_id) ON DELETE CASCADE,
            CONSTRAINT fk_customer_comment FOREIGN KEY (customer_id) REFERENCES customer(customer_id) ON DELETE CASCADE
        );
    )";
    if(success) success &= executeQuery(query, createCommentSQL, "Создание comment");


    // 3. Добавление комментариев и индексов (опционально)
    // ... (код для комментариев и индексов) ...


    // Завершаем транзакцию
    if (success) {
        if (m_db.commit()) {
            qInfo() << "Транзакция создания схемы успешно завершена.";
            return true;
        } else {
            qCritical() << "Ошибка при коммите транзакции создания схемы:" << m_db.lastError().text();
            m_db.rollback();
            return false;
        }
    } else {
        qWarning() << "Произошла ошибка при создании схемы. Откат транзакции...";
        if (!m_db.rollback()) {
            qCritical() << "Ошибка при откате транзакции создания схемы:" << m_db.lastError().text();
        } else {
            qInfo() << "Транзакция создания схемы успешно отменена.";
        }
        return false;
    }
}


// Вспомогательная функция для выполнения запроса с логированием и проверкой
bool DatabaseManager::executeQuery(QSqlQuery &query, const QString &sql, const QString &description)
{
    qInfo().noquote() << QString("Виконання SQL (%1): %2").arg(description, sql.left(100).replace("\n", " ").simplified().append("..."));
    if (!query.exec(sql)) {
        qCritical().noquote() << QString("Помилка при виконанні SQL (%1):").arg(description);
        qCritical() << query.lastError().text();
        qCritical() << "SQL запит:" << sql;
        return false;
    }
    //qInfo().noquote() << QString("Успішно виконано: %1").arg(description); // Можно раскомментировать для детального лога
    return true;
}

// Вспомогательная функция для выполнения ПОДГОТОВЛЕННОГО INSERT с возвратом ID
bool DatabaseManager::executeInsertQuery(QSqlQuery &query, const QString &description, QVariant &insertedId)
{
    qInfo().noquote() << QString("Executing prepared INSERT (%1)...").arg(description); // Логируем только описание

    if (!query.exec()) { // Выполняем подготовленный запрос
        qCritical().noquote() << QString("Error executing prepared INSERT (%1):").arg(description);
        qCritical() << query.lastError().text();
        qCritical() << "Prepared query:" << query.lastQuery(); // Показываем подготовленный запрос
        qCritical() << "Bound values:" << query.boundValues(); // Показываем значения, которые пытались вставить
        return false;
    }

    // Получаем ID из результата RETURNING
    if (query.next()) {
        insertedId = query.value(0); // Предполагаем, что ID - первый столбец (SERIAL PRIMARY KEY)
        if (!insertedId.isValid() || insertedId.isNull()) {
            qWarning() << "Попередження: Не вдалося отримати ID після INSERT для" << description;
            // return false; // Можно сделать это фатальной ошибкой
        }
        // qInfo() << description << "-> Вставлено ID:" << insertedId.toString();
        return true;
    } else {
        qWarning() << "Попередження: Запит INSERT виконано, але RETURNING не повернув рядка для" << description;
        // return false; // Можно сделать это фатальной ошибкой
        return true; // Или продолжить, если ID не критичен для следующих шагов
    }
} // <-- Додано відсутню закриваючу дужку

// Метод populateTestData тепер знаходиться в testdata.cpp

// Реалізація публічних методів доступу
bool DatabaseManager::isConnected() const
{
    return m_isConnected;
}

QSqlDatabase& DatabaseManager::database()
{
    return m_db;
}


QSqlError DatabaseManager::lastError() const
{
    return m_db.lastError();
}

void DatabaseManager::closeConnection()
{
    if (m_isConnected && m_db.isOpen()) {
        QString connectionName = m_db.connectionName();
        m_db.close();
        qInfo() << "З'єднання з базою даних" << connectionName << "закрито.";
        m_isConnected = false;
    }
    if (QSqlDatabase::contains("my_postgres_connection")) {
        QSqlDatabase::removeDatabase("my_postgres_connection");
    }
}

bool DatabaseManager::printAllData() const
{

        if (!m_isConnected || !m_db.isOpen()) {
            qWarning() << "Неможливо вивести дані: немає активного з'єднання з БД.";
            return false;
        }

        qInfo() << "\n===============================================";
        qInfo() << "       ВИВЕДЕННЯ ДАНИХ З УСІХ ТАБЛИЦЬ        ";
        qInfo() << "===============================================";

        // Список таблиц в порядке, удобном для просмотра (или любом другом)
        // Важно: не забываем кавычки для "order"
        const QStringList tables = {"customer", "publisher", "author", "book", "\"order\"",
                                    "book_author", "order_item", "order_status", "comment"}; // Додано comment

        bool overallSuccess = true;

        for (const QString &tableName : tables) {
            qInfo().noquote() << "\n--- Таблиця:" << tableName << "---"; // noquote() убирает лишние кавычки

            QSqlQuery query(m_db);
            QString sql = QString("SELECT * FROM %1;").arg(tableName);

            if (!query.exec(sql)) {
                qCritical().noquote() << QString("Помилка при отриманні даних з таблиці '%1':").arg(tableName);
                qCritical() << query.lastError().text();
                overallSuccess = false; // Помечаем общую неудачу, но продолжаем к следующей таблице
                continue; // Переходим к следующей таблице
            }

            // Получаем информацию о колонках
            QSqlRecord record = query.record();
            if (record.isEmpty() && query.size() == 0) { // Проверяем, есть ли вообще колонки и строки
                qInfo().noquote() << "(Таблиця порожня або не містить колонок)";
                continue;
            }

            // Выводим заголовок таблицы (имена колонок)
            QString headerLine;
            for (int i = 0; i < record.count(); ++i) {
                headerLine += record.fieldName(i) + "\t"; // Используем табуляцию для разделения
            }
            qInfo().noquote() << headerLine.trimmed(); // trimmed() убирает последний таб

            // Выводим разделитель
            QString separatorLine;
            for (int i = 0; i < record.count(); ++i) {
                separatorLine += QString(record.fieldName(i).length(), '-') + "\t";
            }
            qInfo().noquote() << separatorLine.trimmed();


            // Выводим строки данных
            int rowCount = 0;
            while (query.next()) {
                QString dataLine;
                for (int i = 0; i < record.count(); ++i) {
                    // Получаем значение и преобразуем в строку
                    // Обрабатываем NULL значения как "(NULL)"
                    QVariant value = query.value(i);
                    dataLine += (value.isNull() ? "(NULL)" : value.toString()) + "\t";
                }
                qInfo().noquote() << dataLine.trimmed();
                rowCount++;
            }

            if (rowCount == 0 && !record.isEmpty()) {
                qInfo().noquote() << "(Немає даних)";
            } else {
                qInfo().noquote() << QString("-> Всього рядків: %1").arg(rowCount);
            }
        } // Конец цикла по таблицам

        qInfo() << "\n===============================================";
        qInfo() << "       Завершення виведення даних           ";
        qInfo() << "===============================================";


        return overallSuccess; // Возвращаем true, если не было критических ошибок при SELECT
}


// Реалізація нового методу для отримання книг для UI
QList<BookDisplayInfo> DatabaseManager::getAllBooksForDisplay() const
{
    QList<BookDisplayInfo> books;
    if (!m_isConnected || !m_db.isOpen()) {
        qWarning() << "Неможливо отримати книги: немає активного з'єднання з БД.";
        return books; // Повертаємо порожній список
    }

    // Використовуємо LEFT JOIN, щоб отримати книги навіть без авторів або обкладинок
    // Використовуємо STRING_AGG для об'єднання авторів в один рядок
    // Додаємо publisher.name
    const QString sql = R"(
        SELECT
            b.book_id,
            b.title,
            b.price,
            b.cover_image_path,
            b.stock_quantity,
            b.genre, -- Додано отримання жанру
            COALESCE(p.name, 'Невідомий видавець') AS publisher_name,
            STRING_AGG(DISTINCT a.first_name || ' ' || a.last_name, ', ') AS authors
        FROM book b
        LEFT JOIN publisher p ON b.publisher_id = p.publisher_id
        LEFT JOIN book_author ba ON b.book_id = ba.book_id
        LEFT JOIN author a ON ba.author_id = a.author_id
        GROUP BY b.book_id, b.title, b.price, b.cover_image_path, b.stock_quantity, b.genre, p.name -- Додано b.genre в GROUP BY
        ORDER BY b.title;
    )";

    QSqlQuery query(m_db);
    qInfo() << "Executing SQL to get books for display...";
    if (!query.exec(sql)) {
        qCritical() << "Помилка при отриманні списку книг для відображення:";
        qCritical() << query.lastError().text();
        qCritical() << "SQL запит:" << sql;
        return books; // Повертаємо порожній список у разі помилки
    }

    qInfo() << "Successfully fetched books. Processing results...";
    int count = 0;
    while (query.next()) {
        BookDisplayInfo bookInfo;
        bookInfo.bookId = query.value("book_id").toInt();
        bookInfo.title = query.value("title").toString();
        bookInfo.price = query.value("price").toDouble();
        bookInfo.coverImagePath = query.value("cover_image_path").toString();
        bookInfo.stockQuantity = query.value("stock_quantity").toInt();
        bookInfo.authors = query.value("authors").toString();
        bookInfo.genre = query.value("genre").toString(); // Отримуємо жанр

        // Якщо автори відсутні (був LEFT JOIN), перевіряємо на NULL
        if (query.value("authors").isNull()) {
             bookInfo.authors = ""; // Повертаємо порожній рядок, UI вирішить, що показати
        }


        books.append(bookInfo);
        count++;
    }
    qInfo() << "Processed" << count << "books for display.";

    return books;
}


// --- Реалізація методів для роботи з кошиком в БД ---

// Отримання товарів з кошика користувача
QMap<int, int> DatabaseManager::getCartItems(int customerId) const
{
    QMap<int, int> cartItems;
    if (!m_isConnected || !m_db.isOpen() || customerId <= 0) {
        qWarning() << "Неможливо отримати кошик: немає з'єднання або невірний customerId.";
        return cartItems;
    }

    const QString sql = R"(
        SELECT book_id, quantity
        FROM shopping_cart
        WHERE customer_id = :customerId;
    )";

    QSqlQuery query(m_db);
    query.prepare(sql);
    query.bindValue(":customerId", customerId);

    qInfo() << "Executing SQL to get cart items for customer ID:" << customerId;
    if (!query.exec()) {
        qCritical() << "Помилка при отриманні кошика для customer ID '" << customerId << "':";
        qCritical() << query.lastError().text();
        qCritical() << "SQL запит:" << query.lastQuery();
        return cartItems;
    }

    int count = 0;
    while (query.next()) {
        int bookId = query.value("book_id").toInt();
        int quantity = query.value("quantity").toInt();
        if (bookId > 0 && quantity > 0) {
            cartItems.insert(bookId, quantity);
            count++;
        } else {
            qWarning() << "Invalid bookId or quantity found in cart for customer" << customerId << ": bookId=" << bookId << ", quantity=" << quantity;
        }
    }
    qInfo() << "Fetched" << count << "cart items for customer ID:" << customerId;
    return cartItems;
}

// Додавання або оновлення товару в кошику
bool DatabaseManager::addOrUpdateCartItem(int customerId, int bookId, int quantity)
{
    if (!m_isConnected || !m_db.isOpen() || customerId <= 0 || bookId <= 0 || quantity <= 0) {
        qWarning() << "Неможливо додати/оновити товар в кошику: невірні параметри.";
        return false;
    }

    // Використовуємо INSERT ... ON CONFLICT (PostgreSQL specific)
    const QString sql = R"(
        INSERT INTO shopping_cart (customer_id, book_id, quantity, added_at)
        VALUES (:customer_id, :book_id, :quantity, CURRENT_TIMESTAMP)
        ON CONFLICT (customer_id, book_id) DO UPDATE SET
            quantity = EXCLUDED.quantity,
            added_at = CURRENT_TIMESTAMP;
    )";

    QSqlQuery query(m_db);
    query.prepare(sql);
    query.bindValue(":customer_id", customerId);
    query.bindValue(":book_id", bookId);
    query.bindValue(":quantity", quantity);

    qInfo() << "Executing SQL to add/update cart item for customer ID:" << customerId << ", book ID:" << bookId << ", quantity:" << quantity;
    if (!query.exec()) {
        qCritical() << "Помилка при додаванні/оновленні товару в кошику:";
        qCritical() << query.lastError().text();
        qCritical() << "SQL запит:" << query.lastQuery();
        qCritical() << "Bound values:" << query.boundValues();
        return false;
    }

    qInfo() << "Cart item added/updated successfully.";
    return true;
}

// Видалення товару з кошика
bool DatabaseManager::removeCartItemFromDb(int customerId, int bookId)
{
    if (!m_isConnected || !m_db.isOpen() || customerId <= 0 || bookId <= 0) {
        qWarning() << "Неможливо видалити товар з кошика: невірні параметри.";
        return false;
    }

    const QString sql = R"(
        DELETE FROM shopping_cart
        WHERE customer_id = :customer_id AND book_id = :book_id;
    )";

    QSqlQuery query(m_db);
    query.prepare(sql);
    query.bindValue(":customer_id", customerId);
    query.bindValue(":book_id", bookId);

    qInfo() << "Executing SQL to remove cart item for customer ID:" << customerId << ", book ID:" << bookId;
    if (!query.exec()) {
        qCritical() << "Помилка при видаленні товару з кошика:";
        qCritical() << query.lastError().text();
        qCritical() << "SQL запит:" << query.lastQuery();
        qCritical() << "Bound values:" << query.boundValues();
        return false;
    }

    if (query.numRowsAffected() > 0) {
        qInfo() << "Cart item removed successfully.";
    } else {
        qWarning() << "Cart item remove query executed, but no rows were affected (item might not have existed).";
    }
    return true; // Повертаємо true, навіть якщо нічого не було видалено (запит виконався)
}

// Очищення кошика користувача
bool DatabaseManager::clearCart(int customerId)
{
    if (!m_isConnected || !m_db.isOpen() || customerId <= 0) {
        qWarning() << "Неможливо очистити кошик: невірний customerId.";
        return false;
    }

    const QString sql = R"(
        DELETE FROM shopping_cart
        WHERE customer_id = :customer_id;
    )";

    QSqlQuery query(m_db);
    query.prepare(sql);
    query.bindValue(":customer_id", customerId);

    qInfo() << "Executing SQL to clear cart for customer ID:" << customerId;
    if (!query.exec()) {
        qCritical() << "Помилка при очищенні кошика для customer ID '" << customerId << "':";
        qCritical() << query.lastError().text();
        qCritical() << "SQL запит:" << query.lastQuery();
        return false;
    }

    qInfo() << "Cart cleared successfully for customer ID:" << customerId << ". Rows affected:" << query.numRowsAffected();
    return true;
}

// --- Кінець реалізації методів для кошика ---


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
    const QString orderSql = R"(
        SELECT order_id, order_date, total_amount, shipping_address, payment_method
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
        orderInfo.orderId = orderQuery.value("order_id").toInt();
        orderInfo.orderDate = orderQuery.value("order_date").toDateTime();
        orderInfo.totalAmount = orderQuery.value("total_amount").toDouble();
        orderInfo.shippingAddress = orderQuery.value("shipping_address").toString();
        orderInfo.paymentMethod = orderQuery.value("payment_method").toString();
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


// Реалізація нового методу для перевірки, чи користувач вже коментував книгу
bool DatabaseManager::hasUserCommentedOnBook(int bookId, int customerId) const
{
    if (!m_isConnected || !m_db.isOpen() || bookId <= 0 || customerId <= 0) {
        qWarning() << "Неможливо перевірити коментар: немає з'єднання або невірний ID книги/користувача.";
        return false; // Повертаємо false, щоб уникнути блокування, якщо є проблема з перевіркою
    }

    const QString sql = R"(
        SELECT COUNT(*)
        FROM comment
        WHERE book_id = :bookId AND customer_id = :customerId;
    )";

    QSqlQuery query(m_db);
    query.prepare(sql);
    query.bindValue(":bookId", bookId);
    query.bindValue(":customerId", customerId);

    qInfo() << "Executing SQL to check if customer" << customerId << "commented on book" << bookId;
    if (!query.exec()) {
        qCritical() << "Помилка при перевірці наявності коментаря для book ID '" << bookId << "' та customer ID '" << customerId << "':";
        qCritical() << query.lastError().text();
        qCritical() << "SQL запит:" << query.lastQuery();
        return false; // Повертаємо false при помилці запиту
    }

    if (query.next()) {
        int count = query.value(0).toInt();
        qInfo() << "Comment check result: count =" << count;
        return count > 0; // Повертає true, якщо знайдено хоча б один коментар
    }

    qWarning() << "Comment check query did not return a result.";
    return false; // Повертаємо false, якщо запит не повернув результат
}


// Реалізація нового методу для додавання коментаря
bool DatabaseManager::addComment(int bookId, int customerId, const QString &commentText, int rating)
{
    if (!m_isConnected || !m_db.isOpen()) {
        qWarning() << "Неможливо додати коментар: немає з'єднання з БД.";
        return false;
    }
    if (bookId <= 0 || customerId <= 0 || commentText.trimmed().isEmpty()) {
        qWarning() << "Неможливо додати коментар: невірний ID книги/користувача або порожній текст.";
        return false;
    }
    // Перевірка рейтингу (0 - без оцінки, 1-5 - з оцінкою)
    if (rating < 0 || rating > 5) {
        qWarning() << "Неможливо додати коментар: невірний рейтинг (" << rating << "). Допустимі значення: 0-5.";
        return false;
    }

    const QString sql = R"(
        INSERT INTO comment (book_id, customer_id, comment_text, comment_date, rating)
        VALUES (:book_id, :customer_id, :comment_text, CURRENT_TIMESTAMP, :rating);
    )";

    QSqlQuery query(m_db);
    query.prepare(sql);
    query.bindValue(":book_id", bookId);
    query.bindValue(":customer_id", customerId);
    query.bindValue(":comment_text", commentText.trimmed());
    // Якщо рейтинг 0, вставляємо NULL, інакше - значення рейтингу
    query.bindValue(":rating", (rating == 0) ? QVariant(QVariant::Int) : rating);

    qInfo() << "Executing SQL to add comment for book ID:" << bookId << "by customer ID:" << customerId;
    if (!query.exec()) {
        qCritical() << "Помилка при додаванні коментаря для book ID '" << bookId << "':";
        qCritical() << query.lastError().text();
        qCritical() << "SQL запит:" << query.lastQuery();
        qCritical() << "Bound values:" << query.boundValues();
        return false;
    }

    qInfo() << "Comment added successfully for book ID:" << bookId;
    return true;
}


// Реалізація нового методу для створення замовлення
bool DatabaseManager::createOrder(int customerId, const QMap<int, int> &items, const QString &shippingAddress, const QString &paymentMethod, int &newOrderId)
{
    newOrderId = -1;
    if (!m_isConnected || !m_db.isOpen()) {
        qWarning() << "Неможливо створити замовлення: немає з'єднання з БД.";
        return false;
    }
    if (customerId <= 0 || items.isEmpty() || shippingAddress.isEmpty()) {
        qWarning() << "Неможливо створити замовлення: невірний ID користувача, порожній кошик або не вказано адресу.";
        return false;
    }

    // Починаємо транзакцію
    if (!m_db.transaction()) {
        qCritical() << "Не вдалося почати транзакцію для створення замовлення:" << m_db.lastError().text();
        return false;
    }
    qInfo() << "Транзакція для створення замовлення розпочата...";

    QSqlQuery query(m_db);
    bool success = true;
    double calculatedTotalAmount = 0.0;
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

        QSqlQuery itemQuery(m_db); // Окремий запит для позицій
        QSqlQuery priceQuery(m_db); // Окремий запит для отримання ціни та кількості

        if (!itemQuery.prepare(insertItemSQL) || !priceQuery.prepare(getBookPriceSQL)) {
            qCritical() << "Помилка підготовки запиту для позицій замовлення або ціни:" << itemQuery.lastError().text() << priceQuery.lastError().text();
            success = false;
        } else {
            for (auto it = items.constBegin(); it != items.constEnd() && success; ++it) {
                int bookId = it.key();
                int quantity = it.value();

                if (quantity <= 0) continue; // Пропускаємо невірну кількість

                // Отримуємо актуальну ціну та кількість на складі
                priceQuery.bindValue(":book_id", bookId);
                if (!priceQuery.exec()) {
                    qCritical() << "Помилка отримання ціни для книги ID" << bookId << ":" << priceQuery.lastError().text();
                    success = false;
                    break;
                }

                if (priceQuery.next()) {
                    double currentPrice = priceQuery.value(0).toDouble();
                    int currentStock = priceQuery.value(1).toInt();

                    // Перевірка наявності
                    if (quantity > currentStock) {
                         qWarning() << "Недостатньо товару на складі для книги ID" << bookId << "(замовлено:" << quantity << ", на складі:" << currentStock << "). Замовлення скасовано.";
                         // Можна повернути спеціальну помилку або просто false
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
                    qInfo() << "Додано позицію: Книга ID" << bookId << ", Кількість:" << quantity << ", Ціна:" << currentPrice;

                    // TODO: Зменшити stock_quantity в таблиці book (опціонально, залежить від бізнес-логіки)
                    // QSqlQuery updateStockQuery(m_db);
                    // updateStockQuery.prepare("UPDATE book SET stock_quantity = stock_quantity - :quantity WHERE book_id = :book_id");
                    // updateStockQuery.bindValue(":quantity", quantity);
                    // updateStockQuery.bindValue(":book_id", bookId);
                    // if (!updateStockQuery.exec()) { ... обробка помилки ... }

                } else {
                    qWarning() << "Книгу з ID" << bookId << "не знайдено при створенні замовлення. Позицію пропущено.";
                    // Можливо, варто скасувати все замовлення?
                    // success = false;
                    // break;
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
            qInfo() << "Транзакція створення замовлення ID" << newOrderId << "успішно завершена.";
            return true;
        } else {
            qCritical() << "Помилка при коміті транзакції створення замовлення:" << m_db.lastError().text();
            m_db.rollback(); // Спробувати відкат
            return false;
        }
    } else {
        qWarning() << "Виникла помилка під час створення замовлення. Відкат транзакції...";
        if (!m_db.rollback()) {
            qCritical() << "Помилка при відкаті транзакції створення замовлення:" << m_db.lastError().text();
        } else {
            qInfo() << "Транзакція створення замовлення успішно скасована.";
        }
        newOrderId = -1; // Скидаємо ID, оскільки замовлення не створено
        return false;
    }
}


// Реалізація нового методу для отримання детальної інформації про книгу
BookDetailsInfo DatabaseManager::getBookDetails(int bookId) const
{
    BookDetailsInfo details;
    if (!m_isConnected || !m_db.isOpen() || bookId <= 0) {
        qWarning() << "Неможливо отримати деталі книги: немає з'єднання або невірний bookId.";
        return details; // Повертаємо порожню структуру
    }

    const QString sql = R"(
        SELECT
            b.book_id, b.title, b.price, b.cover_image_path, b.stock_quantity,
            b.genre, b.description, b.publication_date, b.isbn, b.page_count, b.language,
            COALESCE(p.name, 'Невідомий видавець') AS publisher_name,
            STRING_AGG(DISTINCT a.first_name || ' ' || a.last_name, ', ') AS authors
        FROM book b
        LEFT JOIN publisher p ON b.publisher_id = p.publisher_id
        LEFT JOIN book_author ba ON b.book_id = ba.book_id
        LEFT JOIN author a ON ba.author_id = a.author_id
        WHERE b.book_id = :bookId
        GROUP BY b.book_id, p.name -- Групуємо за всіма полями книги та видавця
        LIMIT 1; -- Очікуємо тільки один результат
    )";

    QSqlQuery query(m_db);
    query.prepare(sql);
    query.bindValue(":bookId", bookId);

    qInfo() << "Executing SQL to get book details for book ID:" << bookId;
    if (!query.exec()) {
        qCritical() << "Помилка при отриманні деталей книги для book ID '" << bookId << "':";
        qCritical() << query.lastError().text();
        qCritical() << "SQL запит:" << query.lastQuery();
        return details;
    }

    if (query.next()) {
        details.bookId = query.value("book_id").toInt();
        details.title = query.value("title").toString();
        details.price = query.value("price").toDouble();
        details.coverImagePath = query.value("cover_image_path").toString();
        details.stockQuantity = query.value("stock_quantity").toInt();
        details.genre = query.value("genre").toString();
        details.description = query.value("description").toString();
        details.publicationDate = query.value("publication_date").toDate();
        details.isbn = query.value("isbn").toString();
        details.pageCount = query.value("page_count").toInt();
        details.language = query.value("language").toString();
        details.publisherName = query.value("publisher_name").toString();
        details.authors = query.value("authors").toString();
        if (query.value("authors").isNull()) {
             details.authors = ""; // Порожній рядок, якщо немає авторів
        }
        details.found = true;
        qInfo() << "Book details found for book ID:" << bookId;
    } else {
        qInfo() << "Book details not found for book ID:" << bookId;
        // details.found залишається false
    }

    // Отримуємо коментарі до книги
    details.comments = getBookComments(bookId);
    qInfo() << "Fetched" << details.comments.size() << "comments for book ID:" << bookId;


    return details;
}


// Реалізація нового методу для отримання коментарів до книги
QList<CommentDisplayInfo> DatabaseManager::getBookComments(int bookId) const
{
    QList<CommentDisplayInfo> comments;
    if (!m_isConnected || !m_db.isOpen() || bookId <= 0) {
        qWarning() << "Неможливо отримати коментарі: немає з'єднання або невірний bookId.";
        return comments;
    }

    const QString sql = R"(
        SELECT
            c.comment_text,
            c.comment_date,
            c.rating,
            cust.first_name || ' ' || cust.last_name AS author_name
        FROM comment c
        JOIN customer cust ON c.customer_id = cust.customer_id
        WHERE c.book_id = :bookId
        ORDER BY c.comment_date DESC; -- Показуємо новіші коментарі першими
    )";

    QSqlQuery query(m_db);
    query.prepare(sql);
    query.bindValue(":bookId", bookId);

    qInfo() << "Executing SQL to get comments for book ID:" << bookId;
    if (!query.exec()) {
        qCritical() << "Помилка при отриманні коментарів для book ID '" << bookId << "':";
        qCritical() << query.lastError().text();
        qCritical() << "SQL запит:" << query.lastQuery();
        return comments;
    }

    qInfo() << "Successfully fetched comments. Processing results...";
    int count = 0;
    while (query.next()) {
        CommentDisplayInfo commentInfo;
        commentInfo.authorName = query.value("author_name").toString();
        commentInfo.commentDate = query.value("comment_date").toDateTime();
        // Обробка NULL для рейтингу
        QVariant ratingValue = query.value("rating");
        commentInfo.rating = ratingValue.isNull() ? 0 : ratingValue.toInt();
        commentInfo.commentText = query.value("comment_text").toString();

        comments.append(commentInfo);
        count++;
    }
    qInfo() << "Processed" << count << "comments for book ID" << bookId;

    return comments;
}


// Реалізація нового методу для отримання BookDisplayInfo за ID
BookDisplayInfo DatabaseManager::getBookDisplayInfoById(int bookId) const
{
    BookDisplayInfo bookInfo; // found = false за замовчуванням
    if (!m_isConnected || !m_db.isOpen() || bookId <= 0) {
        qWarning() << "Неможливо отримати BookDisplayInfo: немає з'єднання або невірний bookId.";
        return bookInfo;
    }

    // Запит схожий на getAllBooksForDisplay, але з WHERE b.book_id = :bookId
    const QString sql = R"(
        SELECT
            b.book_id,
            b.title,
            b.price,
            b.cover_image_path,
            b.stock_quantity,
            b.genre,
            STRING_AGG(DISTINCT a.first_name || ' ' || a.last_name, ', ') AS authors
        FROM book b
        LEFT JOIN book_author ba ON b.book_id = ba.book_id
        LEFT JOIN author a ON ba.author_id = a.author_id
        WHERE b.book_id = :bookId
        GROUP BY b.book_id -- Групуємо за ID книги, інші поля агрегуються або є унікальними
        LIMIT 1;
    )";

    QSqlQuery query(m_db);
    query.prepare(sql);
    query.bindValue(":bookId", bookId);

    qInfo() << "Executing SQL to get BookDisplayInfo for book ID:" << bookId;
    if (!query.exec()) {
        qCritical() << "Помилка при отриманні BookDisplayInfo для book ID '" << bookId << "':";
        qCritical() << query.lastError().text();
        qCritical() << "SQL запит:" << query.lastQuery();
        return bookInfo;
    }

    if (query.next()) {
        bookInfo.bookId = query.value("book_id").toInt();
        bookInfo.title = query.value("title").toString();
        bookInfo.price = query.value("price").toDouble();
        bookInfo.coverImagePath = query.value("cover_image_path").toString();
        bookInfo.stockQuantity = query.value("stock_quantity").toInt();
        bookInfo.authors = query.value("authors").toString();
        bookInfo.genre = query.value("genre").toString();
        if (query.value("authors").isNull()) {
             bookInfo.authors = "";
        }
        bookInfo.found = true; // Позначаємо, що книгу знайдено
        qInfo() << "BookDisplayInfo found for book ID:" << bookId;
    } else {
        qInfo() << "BookDisplayInfo not found for book ID:" << bookId;
    }

    return bookInfo;
}


// Реалізація нового методу для отримання пропозицій пошуку
QStringList DatabaseManager::getSearchSuggestions(const QString &prefix, int limit) const
{
    QStringList suggestions;
    if (!m_isConnected || !m_db.isOpen() || prefix.length() < 2) { // Не шукаємо занадто короткі префікси
        // qWarning() << "Неможливо отримати пропозиції: немає з'єднання або префікс занадто короткий.";
        return suggestions; // Повертаємо порожній список
    }

    // Використовуємо UNION для об'єднання результатів з книг та авторів
    // Додаємо '%' до префіксу для пошуку за допомогою LIKE
    // Використовуємо LOWER() для пошуку без урахування регістру
    const QString sql = R"(
        (SELECT title AS suggestion
         FROM book
         WHERE LOWER(title) LIKE LOWER(:prefix) || '%'
         ORDER BY title
         LIMIT :limit_per_source)
        UNION
        (SELECT first_name || ' ' || last_name AS suggestion
         FROM author
         WHERE LOWER(first_name || ' ' || last_name) LIKE LOWER(:prefix) || '%'
         ORDER BY suggestion
         LIMIT :limit_per_source)
        ORDER BY suggestion
        LIMIT :total_limit;
    )";

    QSqlQuery query(m_db);
    query.prepare(sql);
    query.bindValue(":prefix", prefix);
    // Обмежуємо кількість результатів з кожного джерела та загальну кількість
    query.bindValue(":limit_per_source", limit); // Обмеження для книг та авторів окремо
    query.bindValue(":total_limit", limit);      // Загальне обмеження

    qInfo() << "Executing SQL to get search suggestions for prefix:" << prefix << "with limit:" << limit;
    if (!query.exec()) {
        qCritical() << "Помилка при отриманні пропозицій пошуку для префікса '" << prefix << "':";
        qCritical() << query.lastError().text();
        qCritical() << "SQL запит:" << query.lastQuery();
        qCritical() << "Bound values:" << query.boundValues();
        return suggestions; // Повертаємо порожній список у разі помилки
    }

    qInfo() << "Successfully fetched suggestions. Processing results...";
    int count = 0;
    while (query.next()) {
        suggestions.append(query.value("suggestion").toString());
        count++;
    }
    qInfo() << "Processed" << count << "suggestions for prefix" << prefix;

    return suggestions;
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
        qWarning() << "Name update query executed, but no rows were affected for customer ID:" << customerId << "(Customer might not exist)";
        return false;
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
        // Якщо адреса не змінилася, numRowsAffected може бути 0, але це не помилка
        qWarning() << "Address update query executed, but no rows were affected for customer ID:" << customerId << "(Customer might not exist or address unchanged)";
        // Повертаємо true, якщо запит виконався без помилок SQL
        return true;
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

    // Перевіряємо, чи був оновлений хоча б один рядок (опціонально, але корисно)
    if (query.numRowsAffected() > 0) {
        qInfo() << "Phone number updated successfully for customer ID:" << customerId;
        return true;
    } else {
        qWarning() << "Phone number update query executed, but no rows were affected for customer ID:" << customerId << "(Customer might not exist)";
        // Можна повернути true, якщо запит виконався без помилок, або false, якщо рядок не знайдено
        return false; // Повертаємо false, якщо користувача не знайдено або телефон вже був таким самим
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
    const QString ordersSql = R"(
        SELECT order_id, order_date, total_amount, shipping_address, payment_method
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
        orderInfo.orderId = orderQuery.value("order_id").toInt();
        orderInfo.orderDate = orderQuery.value("order_date").toDateTime();
        orderInfo.totalAmount = orderQuery.value("total_amount").toDouble();
        orderInfo.shippingAddress = orderQuery.value("shipping_address").toString();
        orderInfo.paymentMethod = orderQuery.value("payment_method").toString();

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


// Реалізація методу для отримання даних для входу
CustomerLoginInfo DatabaseManager::getCustomerLoginInfo(const QString &email) const
{
    CustomerLoginInfo loginInfo;
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


// Реалізація нового методу для отримання авторів для UI
QList<AuthorDisplayInfo> DatabaseManager::getAllAuthorsForDisplay() const
{
    QList<AuthorDisplayInfo> authors;
    if (!m_isConnected || !m_db.isOpen()) {
        qWarning() << "Неможливо отримати авторів: немає активного з'єднання з БД.";
        return authors; // Повертаємо порожній список
    }

    const QString sql = R"(
        SELECT
            author_id,
            first_name,
            last_name,
            nationality,
            image_path
        FROM author
        ORDER BY last_name, first_name; -- Сортуємо за прізвищем та ім'ям
    )";

    QSqlQuery query(m_db);
    qInfo() << "Executing SQL to get authors for display...";
    if (!query.exec(sql)) {
        qCritical() << "Помилка при отриманні списку авторів для відображення:";
        qCritical() << query.lastError().text();
        qCritical() << "SQL запит:" << sql;
        return authors; // Повертаємо порожній список у разі помилки
    }

    qInfo() << "Successfully fetched authors. Processing results...";
    int count = 0;
    while (query.next()) {
        AuthorDisplayInfo authorInfo;
        authorInfo.authorId = query.value("author_id").toInt();
        authorInfo.firstName = query.value("first_name").toString();
        authorInfo.lastName = query.value("last_name").toString();
        authorInfo.nationality = query.value("nationality").toString();
        authorInfo.imagePath = query.value("image_path").toString();

        authors.append(authorInfo);
        count++;
    }
    qInfo() << "Processed" << count << "authors for display.";

    return authors;
}


// Реалізація нового методу для отримання повної інформації профілю користувача
CustomerProfileInfo DatabaseManager::getCustomerProfileInfo(int customerId) const
{
    CustomerProfileInfo profileInfo;
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
    if (executeInsertQuery(query, QString("Register Customer %1").arg(regInfo.email), insertedId)) {
        newCustomerId = insertedId.toInt();
        qInfo() << "Customer registered successfully. Email:" << regInfo.email << "New ID:" << newCustomerId;
        return true;
    } else {
        // Перевіряємо, чи помилка пов'язана з унікальністю email
        if (lastError().text().contains("customer_email_key") || lastError().text().contains("duplicate key value violates unique constraint")) {
            qWarning() << "Registration failed: Email already exists -" << regInfo.email;
        } else {
            qCritical() << "Registration failed for email '" << regInfo.email << "':" << lastError().text();
        }
        return false;
    }
}



// Реалізація нового методу для отримання книг за жанром
QList<BookDisplayInfo> DatabaseManager::getBooksByGenre(const QString &genre, int limit) const
{
    QList<BookDisplayInfo> books;
    if (!m_isConnected || !m_db.isOpen()) {
        qWarning() << "Неможливо отримати книги за жанром: немає активного з'єднання з БД.";
        return books;
    }
    if (genre.isEmpty()) {
        qWarning() << "Неможливо отримати книги: не вказано жанр.";
        return books;
    }

    // Запит схожий на getAllBooksForDisplay, але з WHERE та LIMIT
    const QString sql = R"(
        SELECT
            b.book_id,
            b.title,
            b.price,
            b.cover_image_path,
            b.stock_quantity,
            b.genre,
            COALESCE(p.name, 'Невідомий видавець') AS publisher_name,
            STRING_AGG(DISTINCT a.first_name || ' ' || a.last_name, ', ') AS authors
        FROM book b
        LEFT JOIN publisher p ON b.publisher_id = p.publisher_id
        LEFT JOIN book_author ba ON b.book_id = ba.book_id
        LEFT JOIN author a ON ba.author_id = a.author_id
        WHERE b.genre = :genre -- Фільтрація за жанром
        GROUP BY b.book_id, b.title, b.price, b.cover_image_path, b.stock_quantity, b.genre, p.name
        ORDER BY b.publication_date DESC, b.title -- Сортування (наприклад, новіші спочатку)
        LIMIT :limit; -- Обмеження кількості
    )";

    QSqlQuery query(m_db);
    query.prepare(sql);
    query.bindValue(":genre", genre);
    query.bindValue(":limit", limit > 0 ? limit : 10); // За замовчуванням 10, якщо ліміт недійсний

    qInfo() << "Executing SQL to get books for genre:" << genre << "with limit:" << query.boundValue(":limit").toInt();
    if (!query.exec()) {
        qCritical() << "Помилка при отриманні списку книг для жанру '" << genre << "':";
        qCritical() << query.lastError().text();
        qCritical() << "SQL запит:" << query.lastQuery(); // Показуємо запит з підставленими значеннями
        qCritical() << "Bound values:" << query.boundValues();
        return books;
    }

    qInfo() << "Successfully fetched books for genre" << genre << ". Processing results...";
    int count = 0;
    while (query.next()) {
        BookDisplayInfo bookInfo;
        bookInfo.bookId = query.value("book_id").toInt();
        bookInfo.title = query.value("title").toString();
        bookInfo.price = query.value("price").toDouble();
        bookInfo.coverImagePath = query.value("cover_image_path").toString();
        bookInfo.stockQuantity = query.value("stock_quantity").toInt();
        bookInfo.authors = query.value("authors").toString();
        bookInfo.genre = query.value("genre").toString();

        // Якщо автори відсутні (був LEFT JOIN), перевіряємо на NULL
        if (query.value("authors").isNull()) {
             bookInfo.authors = ""; // Повертаємо порожній рядок
        }

        books.append(bookInfo);
        count++;
    }
    qInfo() << "Processed" << count << "books for genre" << genre;

    return books;
}

