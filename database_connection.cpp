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
#include <QMap>       // Потрібен для QMap в інших файлах
#include <QFile>      // Для читання файлів SQL
#include <QTextStream>// Для читання файлів SQL
#include <QDir>       // Для роботи з директоріями SQL

// Конструктор і деструктор
DatabaseManager::DatabaseManager(QObject *parent) : QObject(parent), m_isConnected(false) // Ініціалізуємо m_isConnected
{
    // Завантажуємо SQL запити перед усім іншим
    if (!loadSqlQueries()) {
        qCritical() << "FATAL: Failed to load SQL queries. Database operations will likely fail.";
        // Можна тут викинути виняток або встановити прапорець помилки
    }

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

    // --- SQL Запити для створення таблиць (порядок важен!) ---
    // 1. Видалення існуючих таблиць (якщо потрібно почати з чистого аркуша)
    // Використовуємо getSqlQuery для отримання запитів з файлу
    success &= executeQuery(query, getSqlQuery("DropOrderStatusTable"), "Видалення order_status");
    if(success) success &= executeQuery(query, getSqlQuery("DropOrderItemTable"),   "Видалення order_item");
    if(success) success &= executeQuery(query, getSqlQuery("DropCommentTable"),     "Видалення comment");
    if(success) success &= executeQuery(query, getSqlQuery("DropBookAuthorTable"),  "Видалення book_author");
    if(success) success &= executeQuery(query, getSqlQuery("DropOrderTable"),       "Видалення \"order\"");
    if(success) success &= executeQuery(query, getSqlQuery("DropCartItemTable"),    "Видалення cart_item");
    if(success) success &= executeQuery(query, getSqlQuery("DropBookTable"),        "Видалення book");
    if(success) success &= executeQuery(query, getSqlQuery("DropAuthorTable"),      "Видалення author");
    if(success) success &= executeQuery(query, getSqlQuery("DropPublisherTable"),   "Видалення publisher");
    if(success) success &= executeQuery(query, getSqlQuery("DropCustomerTable"),    "Видалення customer");

    // 2. Створення таблиць
    if(success) success &= executeQuery(query, getSqlQuery("CreateCustomerTable"), "Створення customer");
    if(success) success &= executeQuery(query, getSqlQuery("CreatePublisherTable"), "Створення publisher");
    if(success) success &= executeQuery(query, getSqlQuery("CreateAuthorTable"), "Створення author");
    if(success) success &= executeQuery(query, getSqlQuery("CreateBookTable"), "Створення book");
    if(success) success &= executeQuery(query, getSqlQuery("CreateOrderTable"), "Створення \"order\"");
    if(success) success &= executeQuery(query, getSqlQuery("CreateBookAuthorTable"), "Створення book_author");
    if(success) success &= executeQuery(query, getSqlQuery("CreateOrderItemTable"), "Створення order_item");
    if(success) success &= executeQuery(query, getSqlQuery("CreateOrderStatusTable"), "Створення order_status");
    if(success) success &= executeQuery(query, getSqlQuery("CreateCommentTable"), "Створення comment");
    if(success) success &= executeQuery(query, getSqlQuery("CreateCartItemTable"), "Створення cart_item");


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


// --- Реалізація функцій завантаження SQL ---

// Завантажує всі .sql файли з вказаної директорії
bool DatabaseManager::loadSqlQueries(const QString& directory)
{
    m_sqlQueries.clear(); // Очищуємо попередні запити
    QDir sqlDir(directory);
    if (!sqlDir.exists()) {
        qCritical() << "SQL directory not found:" << sqlDir.absolutePath();
        return false;
    }

    qInfo() << "Loading SQL queries from directory:" << sqlDir.absolutePath();
    QStringList sqlFiles = sqlDir.entryList(QStringList() << "*.sql", QDir::Files);
    bool allParsed = true;

    for (const QString& fileName : sqlFiles) {
        QString filePath = sqlDir.absoluteFilePath(fileName);
        if (!parseSqlFile(filePath)) {
            qWarning() << "Failed to parse SQL file:" << filePath;
            allParsed = false; // Продовжуємо завантажувати інші файли
        }
    }

    qInfo() << "Loaded" << m_sqlQueries.count() << "SQL queries from" << sqlFiles.count() << "files.";
    if (!allParsed) {
         qWarning() << "Some SQL files failed to parse correctly.";
    }
    return allParsed; // Повертаємо true, тільки якщо ВСІ файли розпарсились успішно
}

// Парсить один .sql файл і додає запити до m_sqlQueries
bool DatabaseManager::parseSqlFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << "Cannot open SQL file for reading:" << filePath << file.errorString();
        return false;
    }

    QTextStream in(&file);
    QString currentQueryName;
    QString currentQuerySql;
    int queryCount = 0;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();

        if (line.startsWith("-- name:")) {
            // Зберігаємо попередній запит, якщо він був
            if (!currentQueryName.isEmpty() && !currentQuerySql.isEmpty()) {
                m_sqlQueries.insert(currentQueryName, currentQuerySql.trimmed());
                queryCount++;
                // qInfo() << "Parsed query:" << currentQueryName << "from file:" << filePath;
            }

            // Починаємо новий запит
            currentQueryName = line.mid(8).trimmed(); // Видаляємо "-- name:"
            currentQuerySql.clear();

            if (currentQueryName.isEmpty()) {
                qWarning() << "Found empty query name after '-- name:' in file:" << filePath << "Line:" << line;
                // Продовжуємо, але цей запит не буде збережено з порожнім іменем
            }
        } else if (!currentQueryName.isEmpty() && !line.startsWith("--") && !line.isEmpty()) {
            // Додаємо рядок до поточного SQL запиту, ігноруючи порожні рядки та коментарі SQL (--)
            currentQuerySql += line + "\n";
        }
    }

    // Зберігаємо останній запит у файлі
    if (!currentQueryName.isEmpty() && !currentQuerySql.isEmpty()) {
        m_sqlQueries.insert(currentQueryName, currentQuerySql.trimmed());
        queryCount++;
        // qInfo() << "Parsed query:" << currentQueryName << "from file:" << filePath;
    }

    file.close();
    qInfo() << "Parsed" << queryCount << "queries from file:" << filePath;
    return true;
}

// Отримує SQL запит за його іменем
QString DatabaseManager::getSqlQuery(const QString& queryName) const
{
    if (!m_sqlQueries.contains(queryName)) {
        qCritical() << "SQL query not found:" << queryName;
        // Можна повернути порожній рядок або кинути виняток
        return QString();
    }
    return m_sqlQueries.value(queryName);
}
