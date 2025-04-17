#include "database.h"
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QVector>
#include <QDate>
#include <QDateTime>
#include <QRandomGenerator> // Для генерації клієнтів/замовлень
#include <QSet>
#include <QSqlRecord>
#include <QMap> // Для зберігання ID

// Вспомогательная функция для генерации случайной даты в диапазоне (залишаємо для клієнтів/замовлень)
QDate randomDate(const QDate &minDate, const QDate &maxDate) {
    qint64 minJulian = minDate.toJulianDay();
    qint64 maxJulian = maxDate.toJulianDay();
    qint64 randomJulian = QRandomGenerator::global()->bounded(minJulian, maxJulian + 1);
    return QDate::fromJulianDay(randomJulian);
}

// Вспомогательная функция для генерации случайного времени (залишаємо для клієнтів/замовлень)
QDateTime randomDateTime(const QDateTime &minDateTime, const QDateTime &maxDateTime) {
    qint64 minSecs = minDateTime.toSecsSinceEpoch();
    qint64 maxSecs = maxDateTime.toSecsSinceEpoch();
    qint64 randomSecs = QRandomGenerator::global()->bounded(minSecs, maxSecs + 1);
    return QDateTime::fromSecsSinceEpoch(randomSecs);
}

// Структури для зберігання реальних даних
struct PublisherData {
    QString name;
    QString contactInfo;
    int dbId = -1; // Для збереження ID після вставки
};

struct AuthorData {
    QString firstName;
    QString lastName;
    QDate birthDate;
    QString nationality;
    QString imagePath; // Додано поле для шляху до зображення
    int dbId = -1; // Для збереження ID після вставки
};

struct BookData {
    QString title;
    QString isbn;
    QDate publicationDate;
    QString publisherName; // Посилання на видавця за іменем
    double price;
    int stockQuantity;
    QString description;
    QString language;
    int pageCount;
    QString coverImagePath;
    QStringList authorLastNames; // Посилання на авторів за прізвищем (спрощено)
    QString genre; // Додано поле жанру
    int dbId = -1; // Для збереження ID книги
    int publisherDbId = -1; // ID видавця з БД
    QList<int> authorDbIds; // ID авторів з БД
};


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
            birth_date DATE, nationality VARCHAR(100), image_path VARCHAR(512) ); )"; // Додано image_path
    if(success) success &= executeQuery(query, createAuthorSQL, "Создание author");

    const QString createBookSQL = R"(
        CREATE TABLE book ( book_id SERIAL PRIMARY KEY, title VARCHAR(255) NOT NULL, isbn VARCHAR(20) UNIQUE,
            publication_date DATE, publisher_id INTEGER, price NUMERIC(10, 2) CHECK (price >= 0),
            stock_quantity INTEGER DEFAULT 0 CHECK (stock_quantity >= 0), description TEXT, language VARCHAR(50),
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
    QVariant lastId; // Для зберігання останнього вставленого ID

    // --- Списки для генерації КЛІЄНТІВ та ЗАМОВЛЕНЬ (залишаємо) ---
    QStringList firstNames = {"Олександр", "Андрій", "Сергій", "Володимир", "Дмитро", "Максим", "Іван", "Артем", "Денис", "Віктор", "Олена", "Наталія", "Тетяна", "Юлія", "Ірина", "Анна", "Оксана", "Марія", "Світлана", "Катерина"};
    QStringList lastNames = {"Мельник", "Шевченко", "Коваленко", "Бондаренко", "Бойко", "Ткаченко", "Кравченко", "Ковальчук", "Коваль", "Олійник", "Шевчук", "Поліщук", "Лисенко", "Бондар", "Мороз", "Марченко", "Ткачук", "Павленко", "Савченко", "Іванова"};
    QStringList cities = {"Київ", "Харків", "Одеса", "Дніпро", "Львів", "Запоріжжя", "Кривий Ріг", "Миколаїв", "Вінниця", "Херсон"};
    QStringList streets = {"вул. Хрещатик", "просп. Свободи", "вул. Сумська", "вул. Пушкінська", "бул. Шевченка", "вул. Городоцька", "вул. Дерибасівська", "просп. Науки", "вул. Соборна", "вул. Центральна"};
    QStringList orderStatuses = {"Очікує підтвердження", "В обробці", "Комплектується", "Передано до служби доставки", "Надіслано", "Доставлено", "Скасовано"};
    QStringList paymentMethods = {"Карткою онлайн", "Готівкою при отриманні", "Переказ на рахунок"};

    // --- Реальні дані ---
    QList<PublisherData> publishers = {
        {"А-ба-ба-га-ла-ма-га", "Київ, вул. Басейна, 1/2А", -1},
        {"Видавництво Старого Лева", "Львів, вул. Старознесенська, 24-26", -1},
        {"Клуб Сімейного Дозвілля", "Харків, вул. Плеханівська, 117", -1},
        {"Наш Формат", "Київ, пров. Алли Горської, 5", -1},
        {"Фабула", "Харків, вул. Сумська, 1", -1},
        {"Vivat", "Харків, вул. Гоголя, 2А", -1},
        {"Penguin Books", "London, UK", -1},
        {"HarperCollins", "New York, USA", -1}
    };

    QList<AuthorData> authors = {
        // Українські класики (3)
        {"Тарас", "Шевченко", QDate(1814, 3, 9), "українець", "", -1}, // Додано порожній imagePath
        {"Леся", "Українка", QDate(1871, 2, 25), "українка", "", -1}, // Додано порожній imagePath
        {"Іван", "Франко", QDate(1856, 8, 27), "українець", "", -1}, // Додано порожній imagePath
        // Зарубіжні (9)
        {"George", "Orwell", QDate(1903, 6, 25), "британець", "", -1}, // Додано порожній imagePath
        {"J.K.", "Rowling", QDate(1965, 7, 31), "британка", "", -1}, // Додано порожній imagePath
        {"Stephen", "King", QDate(1947, 9, 21), "американець", "", -1}, // Додано порожній imagePath
        {"Haruki", "Murakami", QDate(1949, 1, 12), "японець", "", -1}, // Додано порожній imagePath
        {"Neil", "Gaiman", QDate(1960, 11, 10), "британець", "", -1}, // Додано порожній imagePath
        {"Margaret", "Atwood", QDate(1939, 11, 18), "канадка", "", -1}, // Додано порожній imagePath
        {"Yuval Noah", "Harari", QDate(1976, 2, 24), "ізраїльтянин", "", -1}, // Додано порожній imagePath
        {"Andrzej", "Sapkowski", QDate(1948, 6, 21), "поляк", "", -1}, // Додано порожній imagePath
        {"J.R.R.", "Tolkien", QDate(1892, 1, 3), "британець", "", -1} // Додано порожній imagePath
    };

    QList<BookData> books = {
        // Українські класики (6 книг)
        {"Кобзар", "978-966-7047-36-8", QDate(1840, 1, 1), "А-ба-ба-га-ла-ма-га", 300.00, 150, "Збірка поетичних творів", "українська", 704, "D:\\projects\\DB_Kurs\\QtAPP\\cover_img\\test_img.jpg", {"Шевченко"}, "Класика"},
        {"Гайдамаки", "978-966-03-4689-0", QDate(1841, 1, 1), "Видавництво Старого Лева", 180.00, 80, "Історико-героїчна поема", "українська", 160, "D:\\projects\\DB_Kurs\\QtAPP\\cover_img\\test_img.jpg", {"Шевченко"}, "Класика"},
        {"Лісова пісня", "978-617-679-191-9", QDate(1911, 1, 1), "Видавництво Старого Лева", 220.00, 100, "Драма-феєрія", "українська", 256, "D:\\projects\\DB_Kurs\\QtAPP\\cover_img\\test_img.jpg", {"Українка"}, "Класика"},
        {"Камінний господар", "978-966-10-5500-7", QDate(1912, 1, 1), "А-ба-ба-га-ла-ма-га", 190.00, 70, "Драматична поема", "українська", 128, "D:\\projects\\DB_Kurs\\QtAPP\\cover_img\\test_img.jpg", {"Українка"}, "Класика"},
        {"Захар Беркут", "978-966-03-5112-2", QDate(1883, 1, 1), "А-ба-ба-га-ла-ма-га", 250.00, 120, "Історична повість", "українська", 320, "D:\\projects\\DB_Kurs\\QtAPP\\cover_img\\test_img.jpg", {"Франко"}, "Класика"},
        {"Украдене щастя", "978-617-569-098-1", QDate(1893, 1, 1), "Видавництво Старого Лева", 170.00, 90, "Соціально-психологічна драма", "українська", 192, "D:\\projects\\DB_Kurs\\QtAPP\\cover_img\\test_img.jpg", {"Франко"}, "Класика"},
        // Зарубіжні автори (12 книг)
        {"1984", "978-0-452-28423-4", QDate(1949, 6, 8), "Penguin Books", 150.00, 100, "Dystopian classic", "англійська", 328, "D:\\projects\\DB_Kurs\\QtAPP\\cover_img\\test_img.jpg", {"Orwell"}, "Наукова фантастика"},
        {"Animal Farm", "978-0-451-52634-2", QDate(1945, 8, 17), "Penguin Books", 120.00, 120, "Political allegory", "англійська", 144, "D:\\projects\\DB_Kurs\\QtAPP\\cover_img\\test_img.jpg", {"Orwell"}, "Сучасна проза"},
        {"Harry Potter and the Philosopher's Stone", "978-0-7475-3269-9", QDate(1997, 6, 26), "Bloomsbury", 350.00, 80, "Fantasy novel", "англійська", 223, "D:\\projects\\DB_Kurs\\QtAPP\\cover_img\\test_img.jpg", {"Rowling"}, "Фентезі"}, // Note: Publisher not in list
        {"The Shining", "978-0-385-12167-5", QDate(1977, 1, 28), "Doubleday", 280.00, 65, "Horror novel", "англійська", 447, "D:\\projects\\DB_Kurs\\QtAPP\\cover_img\\test_img.jpg", {"King"}, "Жахи"}, // Note: Publisher not in list
        {"It", "978-0-670-81302-5", QDate(1986, 9, 15), "Viking", 450.00, 45, "Horror novel", "англійська", 1138, "D:\\projects\\DB_Kurs\\QtAPP\\cover_img\\test_img.jpg", {"King"}, "Жахи"}, // Note: Publisher not in list
        {"Norwegian Wood", "978-0-375-70402-4", QDate(1987, 1, 1), "Vintage International", 240.00, 70, "Coming-of-age novel", "англійська", 296, "D:\\projects\\DB_Kurs\\QtAPP\\cover_img\\test_img.jpg", {"Murakami"}, "Сучасна проза"}, // Note: Publisher not in list
        {"American Gods", "978-0-380-97365-1", QDate(2001, 6, 19), "HarperCollins", 320.00, 50, "Fantasy novel", "англійська", 465, "D:\\projects\\DB_Kurs\\QtAPP\\cover_img\\test_img.jpg", {"Gaiman"}, "Фентезі"},
        {"The Handmaid's Tale", "978-0-385-49081-8", QDate(1985, 1, 1), "McClelland and Stewart", 270.00, 75, "Dystopian novel", "англійська", 311, "D:\\projects\\DB_Kurs\\QtAPP\\cover_img\\test_img.jpg", {"Atwood"}, "Наукова фантастика"}, // Note: Publisher not in list
        {"Sapiens: A Brief History of Humankind", "978-0-06-231609-7", QDate(2011, 1, 1), "HarperCollins", 400.00, 90, "Non-fiction", "англійська", 464, "D:\\projects\\DB_Kurs\\QtAPP\\cover_img\\test_img.jpg", {"Harari"}, "Науково-популярне"},
        {"The Last Wish (The Witcher)", "978-0-316-43896-9", QDate(1993, 1, 1), "Orbit", 290.00, 60, "Fantasy short stories", "англійська", 400, "D:\\projects\\DB_Kurs\\QtAPP\\cover_img\\test_img.jpg", {"Sapkowski"}, "Фентезі"}, // Note: Publisher not in list
        {"The Hobbit", "978-0-547-92822-7", QDate(1937, 9, 21), "Houghton Mifflin Harcourt", 220.00, 110, "Fantasy novel", "англійська", 310, "D:\\projects\\DB_Kurs\\QtAPP\\cover_img\\test_img.jpg", {"Tolkien"}, "Фентезі"}, // Note: Publisher not in list
        {"The Lord of the Rings", "978-0-618-26027-9", QDate(1954, 7, 29), "Allen & Unwin", 600.00, 40, "High-fantasy novel", "англійська", 1178, "D:\\projects\\DB_Kurs\\QtAPP\\cover_img\\test_img.jpg", {"Tolkien"}, "Фентезі"} // Note: Publisher not in list
    };


    // Хранилища для ID (використовуємо Map для зручного пошуку)
    QMap<QString, int> publisherNameToId;
    QMap<QString, int> authorLastNameToId; // Увага: спрощення, може бути не унікальним!
    QVector<int> customerIds; // Залишаємо для клієнтів
    QVector<int> bookIds;     // Буде заповнено реальними ID книг
    QVector<double> bookPrices; // Зберігаємо ціни для order_item
    QVector<int> orderIds;    // Залишаємо для замовлень


    // --- Заполнение таблиц ---

    // 1. Publishers (Вставка реальних видавців)
    qInfo() << "Populating table publisher with real data...";
    QString insertPublisherSQL = R"(
         INSERT INTO publisher (name, contact_info) VALUES (:name, :contact_info)
         RETURNING publisher_id;
     )";
    if (!query.prepare(insertPublisherSQL)) {
        qCritical() << "Error preparing query for publisher:" << query.lastError().text();
        success = false;
    } else {
        for (PublisherData &pub : publishers) {
            query.bindValue(":name", pub.name);
            query.bindValue(":contact_info", pub.contactInfo);
            if (executeInsertQuery(query, QString("Publisher %1").arg(pub.name), lastId)) {
                pub.dbId = lastId.toInt();
                publisherNameToId[pub.name] = pub.dbId;
            } else {
                // Перевірка на дублікат (якщо запускаємо повторно без очищення)
                 if (query.lastError().text().contains("duplicate key value violates unique constraint")) {
                    qWarning() << "Publisher" << pub.name << "already exists. Fetching ID...";
                    QSqlQuery fetchQuery(m_db);
                    fetchQuery.prepare("SELECT publisher_id FROM publisher WHERE name = :name");
                    fetchQuery.bindValue(":name", pub.name);
                    if (fetchQuery.exec() && fetchQuery.next()) {
                        pub.dbId = fetchQuery.value(0).toInt();
                        publisherNameToId[pub.name] = pub.dbId;
                        qInfo() << "Found existing Publisher ID:" << pub.dbId;
                    } else {
                         qCritical() << "Failed to fetch existing publisher ID for" << pub.name << ":" << fetchQuery.lastError().text();
                         success = false;
                         break;
                    }
                } else {
                    success = false; // Інша помилка є критичною
                    break;
                }
            }
        }
    }

    // 2. Authors (Вставка реальних авторів)
    if (success) {
        qInfo() << "Populating table author with real data...";
        QString insertAuthorSQL = R"(
             INSERT INTO author (first_name, last_name, birth_date, nationality, image_path)
             VALUES (:first_name, :last_name, :birth_date, :nationality, :image_path)
             RETURNING author_id;
         )"; // Додано image_path
        if (!query.prepare(insertAuthorSQL)) {
            qCritical() << "Error preparing query for author:" << query.lastError().text();
            success = false;
        } else {
            for (AuthorData &auth : authors) {
                query.bindValue(":first_name", auth.firstName);
                query.bindValue(":last_name", auth.lastName);
                query.bindValue(":birth_date", auth.birthDate.isValid() ? QVariant(auth.birthDate) : QVariant(QVariant::Date));
                query.bindValue(":nationality", auth.nationality);
                query.bindValue(":image_path", auth.imagePath.isEmpty() ? QVariant(QVariant::String) : auth.imagePath); // Додано прив'язку image_path

                if (executeInsertQuery(query, QString("Author %1 %2").arg(auth.firstName, auth.lastName), lastId)) {
                    auth.dbId = lastId.toInt();
                    // Увага: ключ - прізвище. Якщо є однакові, буде перезаписано!
                    authorLastNameToId[auth.lastName] = auth.dbId;
                } else {
                    // Припускаємо, що автори унікальні і помилка вставки є критичною
                    success = false;
                    break;
                }
            }
        }
    }

    // 3. Books and Book_Author (Вставка реальних книг та зв'язків)
    if (success) {
        qInfo() << "Populating table book and book_author with real data...";
        QString insertBookSQL = R"(
             INSERT INTO book (title, isbn, publication_date, publisher_id, price, stock_quantity, description, language, page_count, cover_image_path, genre)
             VALUES (:title, :isbn, :publication_date, :publisher_id, :price, :stock_quantity, :description, :language, :page_count, :cover_image_path, :genre)
             RETURNING book_id;
         )";
         QString insertBookAuthorSQL = R"(
             INSERT INTO book_author (book_id, author_id) VALUES (:book_id, :author_id);
         )"; // Спрощено, без ролі

        QSqlQuery bookQuery(m_db); // Окремий запит для книг
        QSqlQuery bookAuthorQuery(m_db); // Окремий запит для зв'язків

        if (!bookQuery.prepare(insertBookSQL)) {
             qCritical() << "Error preparing query for book:" << bookQuery.lastError().text();
             success = false;
        }
        if (!bookAuthorQuery.prepare(insertBookAuthorSQL)) {
             qCritical() << "Error preparing query for book_author:" << bookAuthorQuery.lastError().text();
             success = false;
        }

        if(success) {
            for (BookData &book : books) {
                // Знайти ID видавця
                if (publisherNameToId.contains(book.publisherName)) {
                    book.publisherDbId = publisherNameToId.value(book.publisherName);
                } else {
                    qWarning() << "Publisher '" << book.publisherName << "' for book '" << book.title << "' not found in the database. Setting publisher_id to NULL.";
                    book.publisherDbId = -1; // Або інше значення для NULL
                }

                // Знайти ID авторів
                book.authorDbIds.clear();
                for (const QString &lastName : book.authorLastNames) {
                    if (authorLastNameToId.contains(lastName)) {
                        book.authorDbIds.append(authorLastNameToId.value(lastName));
                    } else {
                        qWarning() << "Author with last name '" << lastName << "' for book '" << book.title << "' not found. Skipping author link.";
                    }
                }

                // Вставити книгу
                bookQuery.bindValue(":title", book.title);
                bookQuery.bindValue(":isbn", book.isbn);
                bookQuery.bindValue(":publication_date", book.publicationDate.isValid() ? QVariant(book.publicationDate) : QVariant(QVariant::Date));
                // Обробка NULL для publisher_id
                if (book.publisherDbId != -1) {
                     bookQuery.bindValue(":publisher_id", book.publisherDbId);
                } else {
                     bookQuery.bindValue(":publisher_id", QVariant(QVariant::Int)); // Вставка NULL
                }
                bookQuery.bindValue(":price", book.price);
                bookQuery.bindValue(":stock_quantity", book.stockQuantity);
                bookQuery.bindValue(":description", book.description);
                bookQuery.bindValue(":language", book.language);
                bookQuery.bindValue(":page_count", book.pageCount);
                bookQuery.bindValue(":cover_image_path", book.coverImagePath.isEmpty() ? QVariant(QVariant::String) : book.coverImagePath);
                bookQuery.bindValue(":genre", book.genre.isEmpty() ? QVariant(QVariant::String) : book.genre); // Додано прив'язку жанру

                if (executeInsertQuery(bookQuery, QString("Book %1").arg(book.title), lastId)) {
                    book.dbId = lastId.toInt();
                    bookIds.append(book.dbId); // Зберігаємо ID для замовлень
                    bookPrices.append(book.price); // Зберігаємо ціну

                    // Вставити зв'язки книга-автор
                    if (!book.authorDbIds.isEmpty()) {
                        for (int authorId : book.authorDbIds) {
                            bookAuthorQuery.bindValue(":book_id", book.dbId);
                            bookAuthorQuery.bindValue(":author_id", authorId);
                            if (!bookAuthorQuery.exec()) {
                                // Перевірка на дублікат зв'язку
                                if (bookAuthorQuery.lastError().text().contains("duplicate key value violates unique constraint")) {
                                     qWarning() << "Book-author link (" << book.dbId << "," << authorId << ") already exists. Skipping.";
                                } else {
                                     qCritical() << "Error inserting book_author link:" << bookAuthorQuery.lastError().text();
                                     qCritical() << "Values:" << book.dbId << authorId;
                                     success = false;
                                     break; // Зупиняємо вставку зв'язків для цієї книги
                                }
                            }
                        }
                    }
                } else {
                     // Перевірка на дублікат ISBN
                    if (bookQuery.lastError().text().contains("duplicate key value violates unique constraint")) {
                         qWarning() << "Book with ISBN" << book.isbn << "already exists. Skipping insertion.";
                         // Можна спробувати знайти існуючу книгу і її ID, якщо потрібно
                    } else {
                        success = false; // Інша помилка критична
                    }
                }
                if (!success) break; // Зупиняємо весь процес, якщо була помилка
            }
        }
    }


    // 4. Customer (Генерація тестових клієнтів, як раніше)
    if (success) {
        qInfo() << "Populating table customer (generating test data)...";
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
    } // <- Добавлена недостающая скобка для закрытия 'if (success)' для Customer


    // 5. "order" (Генерація тестових замовлень)
    if (success && !customerIds.isEmpty()) {
        qInfo() << "Populating table \"order\" (generating test data)...";
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

    // 6. order_item (Генерація позицій замовлення з РЕАЛЬНИМИ книгами)
    if (success && !orderIds.isEmpty() && !bookIds.isEmpty()) {
        qInfo() << "Populating table order_item (using real books)...";
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
                    if (booksInOrder.contains(bookId)) continue; // Не додаємо одну й ту саму книгу двічі

                    int quantity = QRandomGenerator::global()->bounded(1, 4); // 1-3 шт.
                    // Беремо реальну ціну книги зі збереженого списку
                    double price = 0.0;
                    int bookIndex = bookIds.indexOf(bookId); // Знаходимо індекс книги
                    if (bookIndex != -1 && bookIndex < bookPrices.size()) {
                        price = bookPrices.at(bookIndex);
                    } else {
                        qWarning() << "Could not find price for book ID" << bookId << "in order" << orderId << ". Using 0.0.";
                    }


                    query.bindValue(":order_id", orderId);
                    query.bindValue(":book_id", bookId); // Використовуємо ID реальної книги
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
            qInfo() << "Created" << itemsCreated << "order items."; // Changed log to English

            // Обновляем total_amount в таблице "order"
            qInfo() << "Updating total_amount in table \"order\"..."; // Changed log to English
            QString updateOrderTotalSQL = R"(UPDATE "order" SET total_amount = :total WHERE order_id = :id;)";
            if (!query.prepare(updateOrderTotalSQL)) {
                qCritical() << "Error preparing query for updating total_amount:" << query.lastError().text(); // Changed log to English
                success = false;
            } else {
                for (auto it = orderTotals.constBegin(); it != orderTotals.constEnd() && success; ++it) {
                    query.bindValue(":total", it.value());
                    query.bindValue(":id", it.key());
                    // Execute the prepared query directly
                    if (!query.exec()) {
                         qCritical().noquote() << QString("Error executing prepared UPDATE (Update Order Total %1):").arg(it.key()); // Changed log to English
                         qCritical() << query.lastError().text();
                         qCritical() << "Bound values:" << query.boundValues();
                         success = false; // Changed log to English
                    }
                }
            }
        }
    } else if (bookIds.isEmpty()) {
         qWarning() << "Skipping order_item population because no books were added.";
    }


    // 7. order_status (Генерація статусів замовлень, як раніше)
    if (success && !orderIds.isEmpty()) {
        qInfo() << "Populating table order_status (generating test data)...";
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
            qInfo() << "Created" << statusesCreated << "order statuses."; // Changed log to English
        }
    }


    // Завершуємо транзакцію
    if (success) {
        if (m_db.commit()) {
            qInfo() << "Data population transaction committed successfully. Added real books/authors and ~" << numberOfRecords << "customers/orders."; // Changed log to English
            return true;
        } else {
            qCritical() << "Error committing data population transaction:" << m_db.lastError().text(); // Changed log to English
            m_db.rollback();
            return false;
        }
    } else {
        qWarning() << "An error occurred during data population. Rolling back transaction..."; // Changed log to English
        if (!m_db.rollback()) {
            qCritical() << "Error rolling back data population transaction:" << m_db.lastError().text(); // Changed log to English
        } else {
            qInfo() << "Data population transaction successfully rolled back."; // Changed log to English
        }
        return false;
    } // Closes else block for 'if (success)'
} // Closes populateTestData function body


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

