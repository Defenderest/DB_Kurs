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
#include <QMap> // Потрібен для createOrder - *Можливо, це включення тут більше не потрібне, якщо createOrder переїде*

// Конструктор і деструктор
DatabaseManager::DatabaseManager(QObject *parent) : QObject(parent), m_isConnected(false) // Ініціалізуємо m_isConnected
{
    if (!QSqlDatabase::isDriverAvailable("QPSQL")) {
        qCritical() << "Error: QPSQL driver for PostgreSQL is not available!";
        qCritical() << "Available drivers:" << QSqlDatabase::drivers();
    }
    // qsrand(QTime::currentTime().msec()); // Закоментовано, бо не використовується
}

DatabaseManager::~DatabaseManager()
{
    closeConnection();
}

// Метод підключення
bool DatabaseManager::connectToDatabase(const QString &host,
                                        int port,
                                        const QString &dbName,
                                        const QString &user,
                                        const QString &password)
{
    closeConnection(); // Закриваємо попереднє з'єднання, якщо воно було
    // Використовуємо унікальне ім'я з'єднання, щоб уникнути конфліктів
    const QString connectionName = QString("db_connection_%1").arg(QDateTime::currentMSecsSinceEpoch());
    m_db = QSqlDatabase::addDatabase("QPSQL", connectionName);
    m_db.setHostName(host);
    m_db.setPort(port);
    m_db.setDatabaseName(dbName);
    m_db.setUserName(user);
    m_db.setPassword(password);

    if (!m_db.open()) {
        qCritical() << "Не вдалося підключитися до бази даних:";
        qCritical() << m_db.lastError().text();
        m_isConnected = false;
        QSqlDatabase::removeDatabase(connectionName); // Видаляємо невдале з'єднання
        return false;
    }

    qDebug() << "Успішно підключено до бази даних" << dbName << "на" << host << ":" << port << "З'єднання:" << connectionName;
    m_isConnected = true;
    return true;
}

// Метод створення схеми
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
    // 1. Удаление существующих таблиц (если нужно начать с чистого листа)
    const QString dropOrderStatusSQL = R"(DROP TABLE IF EXISTS order_status CASCADE;)";
    const QString dropOrderItemSQL = R"(DROP TABLE IF EXISTS order_item CASCADE;)";
    const QString dropCommentSQL = R"(DROP TABLE IF EXISTS comment CASCADE;)"; // Додано видалення comment
    const QString dropBookAuthorSQL = R"(DROP TABLE IF EXISTS book_author CASCADE;)";
    const QString dropOrderSQL = R"(DROP TABLE IF EXISTS "order" CASCADE;)"; // Имя в кавычках
    const QString dropBookSQL = R"(DROP TABLE IF EXISTS book CASCADE;)";
    const QString dropAuthorSQL = R"(DROP TABLE IF EXISTS author CASCADE;)";
    const QString dropPublisherSQL = R"(DROP TABLE IF EXISTS publisher CASCADE;)";
    const QString dropCartItemSQL = R"(DROP TABLE IF EXISTS cart_item CASCADE;)"; // Додано видалення cart_item
    const QString dropCustomerSQL = R"(DROP TABLE IF EXISTS customer CASCADE;)";


    success &= executeQuery(query, dropOrderStatusSQL, "Удаление order_status");
    if(success) success &= executeQuery(query, dropOrderItemSQL,   "Удаление order_item");
    if(success) success &= executeQuery(query, dropCommentSQL,     "Удаление comment");
    if(success) success &= executeQuery(query, dropBookAuthorSQL,  "Удаление book_author");
    if(success) success &= executeQuery(query, dropOrderSQL,       "Удаление \"order\"");
    if(success) success &= executeQuery(query, dropCartItemSQL,    "Удаление cart_item"); // Додано видалення cart_item
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
            birth_date DATE, nationality VARCHAR(100), image_path VARCHAR(512), biography TEXT ); )"; // Додано image_path та biography
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

    const QString createCartItemSQL = R"(
        CREATE TABLE cart_item (
            customer_id INTEGER NOT NULL,
            book_id INTEGER NOT NULL,
            quantity INTEGER NOT NULL CHECK (quantity > 0),
            added_date TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
            PRIMARY KEY (customer_id, book_id), -- Комбінований первинний ключ
            CONSTRAINT fk_customer_cart FOREIGN KEY (customer_id) REFERENCES customer(customer_id) ON DELETE CASCADE,
            CONSTRAINT fk_book_cart FOREIGN KEY (book_id) REFERENCES book(book_id) ON DELETE CASCADE
        );
    )";
    if(success) success &= executeQuery(query, createCartItemSQL, "Создание cart_item");


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

// Допоміжні функції виконання запитів
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
}

// Методи доступу до стану та об'єкту БД
bool DatabaseManager::isConnected() const
{
    return m_isConnected;
}

QSqlDatabase& DatabaseManager::database()
{
    // Перевірка, чи з'єднання все ще дійсне
    if (!m_db.isValid()) {
         qWarning() << "DatabaseManager::database(): Спроба доступу до недійсного об'єкту QSqlDatabase.";
         // Можливо, варто кинути виняток або повернути статичний недійсний об'єкт
    } else if (m_isConnected && !m_db.isOpen()) {
         qWarning() << "DatabaseManager::database(): З'єднання позначено як активне, але об'єкт QSqlDatabase закритий.";
         // Спроба відновити? Або просто повернути як є?
    }
    return m_db;
}

QSqlError DatabaseManager::lastError() const
{
    // Перевіряємо, чи об'єкт m_db взагалі існує перед викликом методу
    if (m_db.isValid()) {
        return m_db.lastError();
    } else {
        qWarning() << "DatabaseManager::lastError(): Спроба отримати помилку для недійсного об'єкту QSqlDatabase.";
        return QSqlError(); // Повертаємо порожню помилку
    }
}

void DatabaseManager::closeConnection()
{
    if (m_db.isOpen()) { // Перевіряємо, чи з'єднання відкрите перед закриттям
        QString connectionName = m_db.connectionName();
        m_db.close();
        qInfo() << "З'єднання з базою даних" << connectionName << "закрито.";
    }
    // Видаляємо з'єднання з пулу Qt, якщо воно існує
    if (QSqlDatabase::contains(m_db.connectionName())) {
         QSqlDatabase::removeDatabase(m_db.connectionName());
         qInfo() << "З'єднання" << m_db.connectionName() << "видалено з пулу.";
    }
    m_isConnected = false;
    // Не потрібно робити m_db = QSqlDatabase(); це може призвести до проблем,
    // якщо хтось зберігає посилання на m_db. isValid() перевірить стан.
}

// Метод для виведення даних (можна залишити тут або перенести в окремий debug файл)
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
                                "book_author", "order_item", "order_status", "comment", "cart_item"}; // Додано cart_item

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
