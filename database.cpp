#include "database.h"
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QVector>
#include <QDate>
#include <QDateTime>
#include <QRandomGenerator> // Более современный генератор случайных чисел
#include <QSet> // Для генерации уникальных пар в связующих таблицах
#include <QSqlRecord>

// Вспомогательная функция для генерации случайной даты в диапазоне
QDate randomDate(const QDate &minDate, const QDate &maxDate) {
    qint64 minJulian = minDate.toJulianDay();
    qint64 maxJulian = maxDate.toJulianDay();
    qint64 randomJulian = QRandomGenerator::global()->bounded(minJulian, maxJulian + 1);
    return QDate::fromJulianDay(randomJulian);
}

// Вспомогательная функция для генерации случайного времени
QDateTime randomDateTime(const QDateTime &minDateTime, const QDateTime &maxDateTime) {
    qint64 minSecs = minDateTime.toSecsSinceEpoch();
    qint64 maxSecs = maxDateTime.toSecsSinceEpoch();
    qint64 randomSecs = QRandomGenerator::global()->bounded(minSecs, maxSecs + 1);
    return QDateTime::fromSecsSinceEpoch(randomSecs);
}


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

    success &= executeQuery(query, dropOrderStatusSQL, "Удаление order_status");
    if(success) success &= executeQuery(query, dropOrderItemSQL,   "Удаление order_item");
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
            loyalty_program BOOLEAN DEFAULT FALSE, join_date DATE NOT NULL DEFAULT CURRENT_DATE,
            loyalty_points INTEGER DEFAULT 0 CHECK (loyalty_points >= 0) ); )";
    if(success) success &= executeQuery(query, createCustomerSQL, "Создание customer");

    const QString createPublisherSQL = R"(
        CREATE TABLE publisher ( publisher_id SERIAL PRIMARY KEY, name VARCHAR(255) NOT NULL UNIQUE, contact_info TEXT ); )";
    if(success) success &= executeQuery(query, createPublisherSQL, "Создание publisher");

    const QString createAuthorSQL = R"(
        CREATE TABLE author ( author_id SERIAL PRIMARY KEY, first_name VARCHAR(100) NOT NULL, last_name VARCHAR(100) NOT NULL,
            birth_date DATE, nationality VARCHAR(100) ); )";
    if(success) success &= executeQuery(query, createAuthorSQL, "Создание author");

    const QString createBookSQL = R"(
        CREATE TABLE book ( book_id SERIAL PRIMARY KEY, title VARCHAR(255) NOT NULL, isbn VARCHAR(20) UNIQUE,
            publication_date DATE, publisher_id INTEGER, price NUMERIC(10, 2) CHECK (price >= 0),
            stock_quantity INTEGER DEFAULT 0 CHECK (stock_quantity >= 0), description TEXT, language VARCHAR(50),
            page_count INTEGER CHECK (page_count > 0),
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
}


// Новый метод для заполнения данными
bool DatabaseManager::populateTestData(int numberOfRecords)
{
    if (!m_isConnected || !m_db.isOpen()) {
        qWarning() << "Неможливо заповнити дані: немає активного з'єднання з БД.";
        return false;
    }
    if (numberOfRecords <= 0) {
        qWarning() << "Кількість записів для генерації повинна бути позитивною.";
        return false;
    }

    // Начинаем транзакцию для вставки данных
    if (!m_db.transaction()) {
        qCritical() << "Не вдалося почати транзакцію для заповнення даних:" << m_db.lastError().text();
        return false;
    }
    qInfo() << "Транзакція для заповнення даних розпочата...";

    QSqlQuery query(m_db);
    bool success = true;
    QVariant lastId; // Для хранения последнего вставленного ID

    // --- Списки тестовых данных на украинском ---
    QStringList firstNames = {"Олександр", "Андрій", "Сергій", "Володимир", "Дмитро", "Максим", "Іван", "Артем", "Денис", "Віктор", "Олена", "Наталія", "Тетяна", "Юлія", "Ірина", "Анна", "Оксана", "Марія", "Світлана", "Катерина"};
    QStringList lastNames = {"Мельник", "Шевченко", "Коваленко", "Бондаренко", "Бойко", "Ткаченко", "Кравченко", "Ковальчук", "Коваль", "Олійник", "Шевчук", "Поліщук", "Лисенко", "Бондар", "Мороз", "Марченко", "Ткачук", "Павленко", "Савченко", "Іванова"};
    QStringList cities = {"Київ", "Харків", "Одеса", "Дніпро", "Львів", "Запоріжжя", "Кривий Ріг", "Миколаїв", "Вінниця", "Херсон"};
    QStringList streets = {"вул. Хрещатик", "просп. Свободи", "вул. Сумська", "вул. Пушкінська", "бул. Шевченка", "вул. Городоцька", "вул. Дерибасівська", "просп. Науки", "вул. Соборна", "вул. Центральна"};
    QStringList bookNouns = {"Таємниця", "Спадщина", "Подорож", "Тінь", "Полум'я", "Доля", "Мрія", "Код", "Острів", "Ліс"};
    QStringList bookAdjectives = {"Забута", "Крижане", "Незвідана", "Темна", "Вічне", "Незламна", "Остання", "Прихований", "Чарівний", "Золотий"};
    QStringList bookGenres = {"Детектив", "Фентезі", "Пригоди", "Роман", "Трилер", "Наукова фантастика", "Історичний роман"};
    QStringList publisherNames = {"А-ба-ба-га-ла-ма-га", "Видавництво Старого Лева", "Клуб Сімейного Дозвілля", "Наш Формат", "Фабула", "Ранок", "Віват", "КМ-Букс", "Фоліо", "Нора-Друк"};
    QStringList roles = {"Автор", "Ілюстратор", "Перекладач"};
    QStringList orderStatuses = {"Очікує підтвердження", "В обробці", "Комплектується", "Передано до служби доставки", "Надіслано", "Доставлено", "Скасовано"};
    QStringList paymentMethods = {"Карткою онлайн", "Готівкою при отриманні", "Переказ на рахунок"};
    QStringList languages = {"українська", "англійська"};
    QStringList nationalities = {"українець/українка", "поляк/полька", "американець/американка", "британець/британка"};

    // Хранилища для сгенерированных ID
    QVector<int> customerIds;
    QVector<int> publisherIds;
    QVector<int> authorIds;
    QVector<int> bookIds;
    QVector<int> orderIds;

    // --- Заполнение таблиц ---

    // 1. Customer
    qInfo() << "Populating table customer..."; // Changed log to English
    QString insertCustomerSQL = R"(
        INSERT INTO customer (first_name, last_name, email, phone, address, loyalty_program, join_date, loyalty_points)
        VALUES (:first_name, :last_name, :email, :phone, :address, :loyalty_program, :join_date, :loyalty_points)
        RETURNING customer_id;
    )"; // Added RETURNING
    if (!query.prepare(insertCustomerSQL)) {
        qCritical() << "Error preparing query for customer:" << query.lastError().text(); // Changed log to English
        success = false;
    } else {
        for (int i = 0; i < numberOfRecords && success; ++i) {
            QString fname = firstNames.at(QRandomGenerator::global()->bounded(firstNames.size()));
            QString lname = lastNames.at(QRandomGenerator::global()->bounded(lastNames.size()));
            query.bindValue(":first_name", fname);
            query.bindValue(":last_name", lname);
            query.bindValue(":email", QString("%1.%2.%3@example.com").arg(fname.toLower()).arg(lname.toLower()).arg(i)); // Уникальный email
            query.bindValue(":phone", QString("+380%1%2%3").arg(QRandomGenerator::global()->bounded(10, 100))
                                          .arg(QRandomGenerator::global()->bounded(100, 1000))
                                          .arg(QRandomGenerator::global()->bounded(1000, 10000)));
            query.bindValue(":address", QString("%1, %2, буд. %3, кв. %4")
                                            .arg(cities.at(QRandomGenerator::global()->bounded(cities.size())))
                                            .arg(streets.at(QRandomGenerator::global()->bounded(streets.size())))
                                            .arg(QRandomGenerator::global()->bounded(1, 151))
                                            .arg(QRandomGenerator::global()->bounded(1, 301)));
            query.bindValue(":loyalty_program", QRandomGenerator::global()->bounded(10) < 3); // ~30% в программе
            query.bindValue(":join_date", randomDate(QDate::currentDate().addYears(-5), QDate::currentDate()));
            query.bindValue(":loyalty_points", QRandomGenerator::global()->bounded(0, 501));

            if (executeInsertQuery(query, QString("Customer %1").arg(i+1), lastId)) {
                customerIds.append(lastId.toInt());
            } else {
                success = false;
            }
        }
    }


    // 2. Publisher
    if (success) {
        qInfo() << "Populating table publisher..."; // Changed log to English
        QString insertPublisherSQL = R"(
             INSERT INTO publisher (name, contact_info) VALUES (:name, :contact_info)
             RETURNING publisher_id;
         )"; // Added RETURNING
        if (!query.prepare(insertPublisherSQL)) {
            qCritical() << "Error preparing query for publisher:" << query.lastError().text(); // Changed log to English
            success = false;
        } else {
            QSet<QString> usedPublisherNames; // Для уникальности
            for (int i = 0; i < qMin(numberOfRecords, publisherNames.size()) && success; ++i) { // Берем уникальные названия
                QString pubName = publisherNames.at(i);
                // if (usedPublisherNames.contains(pubName)) continue; // Пропускаем, если имя уже есть (хотя тут берем по индексу)
                // usedPublisherNames.insert(pubName);

                query.bindValue(":name", pubName);
                query.bindValue(":contact_info", QString("Контактна інформація для %1").arg(pubName));

                if (executeInsertQuery(query, QString("Publisher %1").arg(i+1), lastId)) {
                    publisherIds.append(lastId.toInt());
                } else {
                    success = false;
                }
            }
        }
    }

    // 3. Author
    if (success) {
        qInfo() << "Populating table author..."; // Changed log to English
        QString insertAuthorSQL = R"(
             INSERT INTO author (first_name, last_name, birth_date, nationality)
             VALUES (:first_name, :last_name, :birth_date, :nationality)
             RETURNING author_id;
         )"; // Added RETURNING
        if (!query.prepare(insertAuthorSQL)) {
            qCritical() << "Error preparing query for author:" << query.lastError().text(); // Changed log to English
            success = false;
        } else {
            for (int i = 0; i < numberOfRecords && success; ++i) {
                query.bindValue(":first_name", firstNames.at(QRandomGenerator::global()->bounded(firstNames.size())));
                query.bindValue(":last_name", lastNames.at(QRandomGenerator::global()->bounded(lastNames.size())));
                query.bindValue(":birth_date", randomDate(QDate(1950, 1, 1), QDate(2000, 1, 1)));
                query.bindValue(":nationality", nationalities.at(QRandomGenerator::global()->bounded(nationalities.size())));

                if (executeInsertQuery(query, QString("Author %1").arg(i+1), lastId)) {
                    authorIds.append(lastId.toInt());
                } else {
                    success = false;
                }
            }
        }
    }


    // 4. Book
    if (success && !publisherIds.isEmpty()) { // At least one publisher is needed
        qInfo() << "Populating table book..."; // Changed log to English
        QString insertBookSQL = R"(
             INSERT INTO book (title, isbn, publication_date, publisher_id, price, stock_quantity, description, language, page_count)
             VALUES (:title, :isbn, :publication_date, :publisher_id, :price, :stock_quantity, :description, :language, :page_count)
             RETURNING book_id;
         )"; // Added RETURNING
        if (!query.prepare(insertBookSQL)) {
            qCritical() << "Error preparing query for book:" << query.lastError().text(); // Changed log to English
            success = false;
        } else {
            for (int i = 0; i < numberOfRecords * 2 && success; ++i) { // Генерируем больше книг
                QString title = QString("%1 %2")
                                    .arg(bookAdjectives.at(QRandomGenerator::global()->bounded(bookAdjectives.size())))
                                    .arg(bookNouns.at(QRandomGenerator::global()->bounded(bookNouns.size())));
                query.bindValue(":title", title);
                query.bindValue(":isbn", QString("978-966-%1-%2-%3").arg(QRandomGenerator::global()->bounded(1000, 10000))
                                             .arg(QRandomGenerator::global()->bounded(100, 1000))
                                             .arg(i)); // Уникальный ISBN
                query.bindValue(":publication_date", randomDate(QDate(2000, 1, 1), QDate::currentDate()));
                query.bindValue(":publisher_id", publisherIds.at(QRandomGenerator::global()->bounded(publisherIds.size())));
                query.bindValue(":price", 600);
                query.bindValue(":stock_quantity", QRandomGenerator::global()->bounded(0, 101));
                query.bindValue(":description", QString("Чудовий опис для книги '%1'").arg(title));
                query.bindValue(":language", languages.at(QRandomGenerator::global()->bounded(languages.size())));
                query.bindValue(":page_count", QRandomGenerator::global()->bounded(100, 801));

                if (executeInsertQuery(query, QString("Book %1").arg(i+1), lastId)) {
                    bookIds.append(lastId.toInt());
                } else {
                    // ISBN может быть не уникальным при перезапуске, игнорируем ошибку UNIQUE constraint
                    if (!query.lastError().text().contains("duplicate key value violates unique constraint")) {
                        success = false; // Провалить транзакцию при других ошибках
                    } else {
                        qWarning() << "Пропуск дубліката ISBN для книги" << (i+1);
                    }
                }
            }
        }
    } else if (publisherIds.isEmpty()) {
        qWarning() << "Пропуск заповнення 'book', оскільки немає видавців (publisher).";
    }

    // 5. "order"
    if (success && !customerIds.isEmpty()) {
        qInfo() << "Populating table \"order\"..."; // Changed log to English
        QString insertOrderSQL = R"(
             INSERT INTO "order" (customer_id, order_date, total_amount, shipping_address, payment_method)
             VALUES (:customer_id, :order_date, :total_amount, :shipping_address, :payment_method)
             RETURNING order_id;
         )"; // Added RETURNING
        // Note the quotes around "order" in the SQL query!
        if (!query.prepare(insertOrderSQL)) {
            qCritical() << "Error preparing query for \"order\":" << query.lastError().text(); // Changed log to English
            success = false;
        } else {
            for (int i = 0; i < numberOfRecords && success; ++i) {
                query.bindValue(":customer_id", customerIds.at(QRandomGenerator::global()->bounded(customerIds.size())));
                query.bindValue(":order_date", randomDateTime(QDateTime::currentDateTime().addDays(-90), QDateTime::currentDateTime()));
                query.bindValue(":total_amount", 500); // Сумма будет пересчитана позже, это плейсхолдер
                query.bindValue(":shipping_address", QString("%1, %2, буд. %3, Нова Пошта %4")
                                                         .arg(cities.at(QRandomGenerator::global()->bounded(cities.size())))
                                                         .arg(streets.at(QRandomGenerator::global()->bounded(streets.size())))
                                                         .arg(QRandomGenerator::global()->bounded(1, 151))
                                                         .arg(QRandomGenerator::global()->bounded(1, 51)));
                query.bindValue(":payment_method", paymentMethods.at(QRandomGenerator::global()->bounded(paymentMethods.size())));

                if (executeInsertQuery(query, QString("Order %1").arg(i+1), lastId)) {
                    orderIds.append(lastId.toInt());
                } else {
                    success = false;
                }
            }
        }
    } else if (customerIds.isEmpty()) {
        qWarning() << "Пропуск заповнення 'order', оскільки немає клієнтів (customer).";
    }


    // 6. book_author (Связующая таблица)
    if (success && !bookIds.isEmpty() && !authorIds.isEmpty()) {
        qInfo() << "Заповнення таблиці book_author...";
        QString insertBookAuthorSQL = R"(
             INSERT INTO book_author (book_id, author_id, role) VALUES (:book_id, :author_id, :role);
         )";
        if (!query.prepare(insertBookAuthorSQL)) {
            qCritical() << "Помилка підготовки запиту для book_author:" << query.lastError().text();
            success = false;
        } else {
            QSet<QPair<int, int>> usedPairs; // Для уникальности пар (книга, автор)
            int linksCreated = 0;
            for (int bookId : bookIds) { // Для каждой книги добавим 1-2 автора
                int authorsCount = QRandomGenerator::global()->bounded(1, 3); // 1 или 2 автора
                for (int j = 0; j < authorsCount && success; ++j) {
                    int authorId = authorIds.at(QRandomGenerator::global()->bounded(authorIds.size()));
                    QPair<int, int> currentPair = qMakePair(bookId, authorId);

                    if (usedPairs.contains(currentPair)) continue; // Пропускаем, если такая связь уже есть

                    query.bindValue(":book_id", bookId);
                    query.bindValue(":author_id", authorId);
                    query.bindValue(":role", roles.at(QRandomGenerator::global()->bounded(roles.size()))); // Random role

                    // Execute the prepared query directly
                    if (query.exec()) {
                        usedPairs.insert(currentPair);
                        linksCreated++;
                    } else {
                        // Ignore PRIMARY KEY violation error if the pair already exists
                        if (!query.lastError().text().contains("duplicate key value violates unique constraint")) {
                            success = false;
                        } else {
                            qWarning() << "Пропуск дубліката зв'язку book_author для" << currentPair;
                        }
                    }
                }
                if (linksCreated >= numberOfRecords * 1.5) break; // Ограничим общее кол-во связей
            }
            qInfo() << "Створено" << linksCreated << "зв'язків книга-автор.";
        }
    }

    // 7. order_item
    if (success && !orderIds.isEmpty() && !bookIds.isEmpty()) {
        qInfo() << "Заповнення таблиці order_item...";
        QString insertOrderItemSQL = R"(
             INSERT INTO order_item (order_id, book_id, quantity, price_per_unit)
             VALUES (:order_id, :book_id, :quantity, :price_per_unit);
         )";
        if (!query.prepare(insertOrderItemSQL)) {
            qCritical() << "Помилка підготовки запиту для order_item:" << query.lastError().text();
            success = false;
        } else {
            QMap<int, double> orderTotals; // Для пересчета total_amount в заказе
            int itemsCreated = 0;
            for (int orderId : orderIds) { // Для каждого заказа добавим 1-4 позиции
                int itemsCount = QRandomGenerator::global()->bounded(1, 5); // 1-4 книги в заказе
                QSet<int> booksInOrder; // Книги в текущем заказе
                for (int j = 0; j < itemsCount && success; ++j) {
                    int bookId = bookIds.at(QRandomGenerator::global()->bounded(bookIds.size()));
                    if (booksInOrder.contains(bookId)) continue; // Не добавляем одну и ту же книгу дважды

                    int quantity = QRandomGenerator::global()->bounded(1, 4); // 1-3 шт.
                    // Получим цену книги (для примера возьмем случайную, в реальности надо SELECT цену из book)
                    double price = 200;

                    query.bindValue(":order_id", orderId);
                    query.bindValue(":book_id", bookId);
                    query.bindValue(":quantity", quantity);
                    query.bindValue(":price_per_unit", price);

                    // Execute the prepared query directly
                    if (query.exec()) {
                        orderTotals[orderId] += quantity * price; // Sum up the cost
                        booksInOrder.insert(bookId);
                        itemsCreated++;
                    } else {
                        qCritical().noquote() << QString("Error executing prepared INSERT (OrderItem %1):").arg(itemsCreated + 1);
                        qCritical() << query.lastError().text();
                        qCritical() << "Bound values:" << query.boundValues();
                        success = false; // Any error here is critical
                    }
                }
                if (itemsCreated >= numberOfRecords * 2.5) break; // Ограничение
            }
            qInfo() << "Створено" << itemsCreated << "позицій замовлень.";

            // Обновляем total_amount в таблице "order"
            qInfo() << "Оновлення total_amount в таблиці \"order\"...";
            QString updateOrderTotalSQL = R"(UPDATE "order" SET total_amount = :total WHERE order_id = :id;)";
            if (!query.prepare(updateOrderTotalSQL)) {
                qCritical() << "Помилка підготовки запиту для оновлення total_amount:" << query.lastError().text();
                success = false;
            } else {
                for (auto it = orderTotals.constBegin(); it != orderTotals.constEnd() && success; ++it) {
                    query.bindValue(":total", it.value());
                    query.bindValue(":id", it.key());
                    // Execute the prepared query directly
                    if (!query.exec()) {
                         qCritical().noquote() << QString("Error executing prepared UPDATE (Update Order Total %1):").arg(it.key());
                         qCritical() << query.lastError().text();
                         qCritical() << "Bound values:" << query.boundValues();
                         success = false;
                    }
                }
            }
        }
    }

    // 8. order_status
    if (success && !orderIds.isEmpty()) {
        qInfo() << "Заповнення таблиці order_status...";
        QString insertOrderStatusSQL = R"(
            INSERT INTO order_status (order_id, status, status_date, tracking_number)
            VALUES (:order_id, :status, :status_date, :tracking_number);
        )";
        if (!query.prepare(insertOrderStatusSQL)) {
            qCritical() << "Помилка підготовки запиту для order_status:" << query.lastError().text();
            success = false;
        } else {
            int statusesCreated = 0;
            for (int orderId : orderIds) { // Для каждого заказа добавим 1-3 статуса
                int statusCount = QRandomGenerator::global()->bounded(1, 4);
                QDateTime lastStatusDate = QDateTime::currentDateTime().addDays(-91); // Начальная дата для статуса

                // Получим дату создания заказа, чтобы статусы были после нее
                QSqlQuery dateQuery(m_db);
                dateQuery.prepare(R"(SELECT order_date FROM "order" WHERE order_id = :id)");
                dateQuery.bindValue(":id", orderId);
                if (dateQuery.exec() && dateQuery.next()) {
                    lastStatusDate = dateQuery.value(0).toDateTime();
                }


                for (int j = 0; j < statusCount && success; ++j) {
                    QString status = orderStatuses.at(QRandomGenerator::global()->bounded(orderStatuses.size()));
                    QDateTime statusDate = randomDateTime(lastStatusDate.addSecs(3600), // Минимум через час после предыдущего
                                                          lastStatusDate.addDays(5).addSecs(86400)); // Максимум через 5 дней
                    QString tracking = (status == "Надіслано" || status == "Доставлено")
                                           ? QString("59000%1").arg(QRandomGenerator::global()->bounded(10000000, 99999999))
                                           : QVariant(QVariant::String).toString(); // Пустой трекинг для других статусов

                    query.bindValue(":order_id", orderId);
                    query.bindValue(":status", status);
                    query.bindValue(":status_date", statusDate);
                    query.bindValue(":tracking_number", tracking.isEmpty() ? QVariant(QVariant::String) : tracking); // Pass NULL if empty

                    // Execute the prepared query directly
                    if (query.exec()) {
                        lastStatusDate = statusDate; // Update the last status date
                        statusesCreated++;
                    } else {
                        qCritical().noquote() << QString("Error executing prepared INSERT (OrderStatus %1):").arg(statusesCreated + 1);
                        qCritical() << query.lastError().text();
                        qCritical() << "Bound values:" << query.boundValues();
                        success = false;
                    }
                }
                if (statusesCreated >= numberOfRecords * 2) break; // Ограничение
            }
            qInfo() << "Створено" << statusesCreated << "статусів замовлень.";
        }
    }


    // Завершаем транзакцию
    if (success) {
        if (m_db.commit()) {
            qInfo() << "Транзакція заповнення даних успішно завершена. Додано приблизно по" << numberOfRecords << "записів.";
            return true;
        } else {
            qCritical() << "Помилка при коміті транзакції заповнення даних:" << m_db.lastError().text();
            m_db.rollback();
            return false;
        }
    } else {
        qWarning() << "Произошла ошибка при заполнении данных. Откат транзакции...";
        if (!m_db.rollback()) {
            qCritical() << "Помилка при відкаті транзакції заповнення даних:" << m_db.lastError().text();
        } else {
            qInfo() << "Транзакція заповнення даних успішно скасована.";
        }
        return false;
    }
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
                                    "book_author", "order_item", "order_status"};

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

