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
    // QString description; // Видалено дублююче поле опису, воно вже є вище
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
        {"Тарас", "Шевченко", QDate(1814, 3, 9), "українець", "D:\\projects\\DB_Kurs\\QtAPP\\author_img\\Taras_Shevchenko.jpg", -1}, // Додано порожній imagePath
        {"Леся", "Українка", QDate(1871, 2, 25), "українка", "D:\\projects\\DB_Kurs\\QtAPP\\author_img\\Lesia_Ukrainka.jpg", -1}, // Додано порожній imagePath
        {"Іван", "Франко", QDate(1856, 8, 27), "українець", "D:\\projects\\DB_Kurs\\QtAPP\\author_img\\Ivan_Franko.jpg", -1}, // Додано порожній imagePath
        // Зарубіжні (9)
        {"George", "Orwell", QDate(1903, 6, 25), "британець", "D:\\projects\\DB_Kurs\\QtAPP\\author_img\\George_Orwell.jpg", -1}, // Додано порожній imagePath
        {"J.K.", "Rowling", QDate(1965, 7, 31), "британка", "D:\\projects\\DB_Kurs\\QtAPP\\author_img\\J.K.-Rowling-2021.jpg", -1}, // Додано порожній imagePath
        {"Stephen", "King", QDate(1947, 9, 21), "американець", "D:\\projects\\DB_Kurs\\QtAPP\\author_img\\Stephen_King.jpg", -1}, // Додано порожній imagePath
        {"Haruki", "Murakami", QDate(1949, 1, 12), "японець", "D:\\projects\\DB_Kurs\\QtAPP\\author_img\\haruki-murakami.jpg", -1}, // Додано порожній imagePath
        {"Neil", "Gaiman", QDate(1960, 11, 10), "британець", "D:\\projects\\DB_Kurs\\QtAPP\\author_img\\Neil_Gaiman.jpg", -1}, // Додано порожній imagePath
        {"Margaret", "Atwood", QDate(1939, 11, 18), "канадка", "D:\\projects\\DB_Kurs\\QtAPP\\author_img\\Margaret_Atwood.jpg", -1}, // Додано порожній imagePath
        {"Yuval Noah", "Harari", QDate(1976, 2, 24), "ізраїльтянин", "D:\\projects\\DB_Kurs\\QtAPP\\author_img\\Yuval Noah_Harari.jpg", -1}, // Додано порожній imagePath
        {"Andrzej", "Sapkowski", QDate(1948, 6, 21), "поляк", "D:\\projects\\DB_Kurs\\QtAPP\\author_img\\Andrzej_Sapkowski.jpg", -1},
        {"George R.R.", "Martin", QDate(1948, 9, 20), "американець", "D:\\projects\\DB_Kurs\\QtAPP\\author_img\\George_RR_Martin.jpg", -1},        // Додано порожній imagePath
        {"J.R.R.", "Tolkien", QDate(1892, 1, 3), "британець", "D:\\projects\\DB_Kurs\\QtAPP\\author_img\\Tolkien.jpg", -1} // Додано порожній imagePath

    };

    QList<BookData> books = {
        // Українські класики (6 книг)
        {"Кобзар", "978-966-7047-36-8", QDate(1840, 1, 1), "А-ба-ба-га-ла-ма-га", 300.00, 150,
         "«Кобзар» Тараса Шевченка – це не просто збірка поетичних творів, це серце української літератури та душа нації. Вперше виданий у 1840 році, він став символом боротьби за свободу, національну ідентичність та людську гідність. Книга містить найвідоміші поеми та вірші Шевченка, такі як «Катерина», «Гайдамаки», «Сон», «Заповіт» та багато інших. Кожен рядок пронизаний глибоким патріотизмом, болем за долю України та її народу, вірою у краще майбутнє. Шевченко майстерно змальовує історичні події, життя простих людей, їхні радощі та страждання. Його мова – багата, образна, мелодійна – торкається найглибших струн душі. «Кобзар» – це книга, яка надихала покоління українців на боротьбу, формувала національну свідомість і залишається актуальною й донині. Вона є обов'язковою для прочитання кожному, хто хоче зрозуміти історію, культуру та дух України. Це вічний пам'ятник генію українського народу.",
         "українська", 704, "D:\\projects\\DB_Kurs\\QtAPP\\cover_img\\kobzar.jpg", {"Шевченко"}, "Класика"},
        {"Гайдамаки", "978-966-03-4689-0", QDate(1841, 1, 1), "Видавництво Старого Лева", 180.00, 80,
         "«Гайдамаки» – одна з найвизначніших поем Тараса Шевченка, що змальовує трагічні та героїчні події Коліївщини – великого народного повстання 1768 року проти польсько-шляхетського панування на Правобережній Україні. Поема вражає своєю епічною масштабністю, динамізмом та глибоким психологізмом. Шевченко не лише відтворює історичний контекст, але й проникає у внутрішній світ героїв – відважних гайдамаків, які борються за волю та справедливість, та їхніх супротивників. Центральною фігурою є Ярема Галайда, чия особиста драма переплітається з долею всього народу. Поет майстерно використовує фольклорні мотиви, народні пісні та перекази, що надає твору особливої автентичності та емоційної сили. «Гайдамаки» – це не просто історична хроніка, а й глибоке філософське осмислення тем свободи, помсти, жертовності та національної єдності. Поема є важливим твором української літератури, що розкриває складні сторінки історії та незламний дух народу.",
         "українська", 160, "D:\\projects\\DB_Kurs\\QtAPP\\cover_img\\gaidamaki.jpg", {"Шевченко"}, "Класика"},
        {"Лісова пісня", "978-617-679-191-9", QDate(1911, 1, 1), "Видавництво Старого Лева", 220.00, 100,
         "«Лісова пісня» – шедевр української драматургії, драма-феєрія Лесі Українки, що зачаровує своєю поетичністю, глибиною символізму та філософським змістом. Твір розповідає про трагічне кохання лісової мавки Мавки та людського хлопця Лукаша. Їхні стосунки розгортаються на тлі мальовничої волинської природи, яка постає не просто декорацією, а повноцінним дійовим персонажем, сповненим таємниць та магії. Леся Українка майстерно протиставляє два світи: гармонійний, одухотворений світ природи, втілений в образах лісових істот, та прагматичний, часто жорстокий світ людей. Конфлікт між цими світами, між вічним і тимчасовим, духовним і матеріальним, є центральним у творі. «Лісова пісня» – це гімн красі природи, чистому коханню та свободі духу. П'єса сповнена глибоких роздумів про сенс життя, мистецтво, вірність і зраду. Вона залишається одним із найулюбленіших творів української літератури, що не втрачає своєї актуальності та чарівності.",
         "українська", 256, "D:\\projects\\DB_Kurs\\QtAPP\\cover_img\\lisova.jpg", {"Українка"}, "Класика"},
        {"Камінний господар", "978-966-10-5500-7", QDate(1912, 1, 1), "А-ба-ба-га-ла-ма-га", 190.00, 70,
         "«Камінний господар» – одна з найвідоміших драматичних поем Лесі Українки, що є оригінальною інтерпретацією вічного образу Дон Жуана. На відміну від традиційних трактувань, де Дон Жуан постає лише як спокусник і розпусник, у Лесі Українки він – Командор, сильна, вольова особистість, що прагне влади та утвердження власного «я». Його головною метою є не стільки кохання, скільки підкорення, руйнація усталених норм і традицій. Центральним є конфлікт між Командором та Донною Анною, яка втілює консервативні цінності та моральні принципи. Їхнє протистояння – це боротьба не лише двох характерів, а й двох світоглядів. Поема насичена глибокими філософськими роздумами про свободу волі, відповідальність, ціну влади та сенс людського існування. Леся Українка майстерно використовує символіку та психологізм, створюючи напружену атмосферу та багатогранні образи. «Камінний господар» – це складний, інтелектуальний твір, що спонукає до роздумів і залишає незабутнє враження.",
         "українська", 128, "D:\\projects\\DB_Kurs\\QtAPP\\cover_img\\kaministiy_gospodar.jpg", {"Українка"}, "Класика"},
        {"Захар Беркут", "978-966-03-5112-2", QDate(1883, 1, 1), "А-ба-ба-га-ла-ма-га", 250.00, 120,
         "«Захар Беркут» – визначна історична повість Івана Франка, що переносить читача у XIII століття, в часи героїчної боротьби карпатської громади Тухля проти монгольської навали. В центрі твору – мудрий та відважний ватажок тухольців Захар Беркут, який втілює ідеал народного лідера, що ставить громадські інтереси вище за особисті. Повість майстерно поєднує захопливий сюжет, яскраві описи побуту та звичаїв гірського населення, та глибокі роздуми про патріотизм, єдність, свободу та зраду. Франко змальовує не лише зовнішню боротьбу з ворогом, але й внутрішні конфлікти, зокрема протистояння між Захаром Беркутом та боярином Тугаром Вовком, який заради власних амбіцій готовий зрадити свій народ. Особливої уваги заслуговує лінія кохання сина Захара, Максима, та дочки Тугара Вовка, Мирослави, яка символізує можливість примирення та єднання. «Захар Беркут» – це твір про силу народного духу, важливість згуртованості та вірності рідній землі, що залишається актуальним і сьогодні.",
         "українська", 320, "D:\\projects\\DB_Kurs\\QtAPP\\cover_img\\zahar_berkut.jpg", {"Франко"}, "Класика"},
        {"Украдене щастя", "978-617-569-098-1", QDate(1893, 1, 1), "Видавництво Старого Лева", 170.00, 90,
         "«Украдене щастя» – одна з найсильніших соціально-психологічних драм української літератури, написана Іваном Франком. П'єса розкриває трагічну історію любовного трикутника в галицькому селі кінця XIX століття. Головні герої – Анна, її чоловік Микола Задорожний та її колишній коханий Михайло Гурман – опиняються у вирі складних почуттів, соціальних умовностей та особистих трагедій. Франко майстерно показує, як суспільні норми, бідність та людська підступність руйнують долі та крадуть можливість простого людського щастя. Драма вражає глибиною психологічного аналізу персонажів, їхніх внутрішніх переживань, сумнівів та пристрастей. Кожен герой – це складна особистість зі своєю правдою та своїм болем. Мова твору – жива, народна, сповнена емоцій та драматизму. «Украдене щастя» – це гостра критика соціальної несправедливості та водночас глибоке дослідження людської душі, її прагнення до любові та щастя всупереч усім перешкодам. П'єса залишається актуальною і сьогодні, змушуючи замислитися над вічними питаннями.",
         "українська", 192, "D:\\projects\\DB_Kurs\\QtAPP\\cover_img\\ukradene_shastia.jpg", {"Франко"}, "Класика"},
        // Зарубіжні автори (12 книг)
        {"1984", "978-0-452-28423-4", QDate(1949, 6, 8), "Penguin Books", 150.00, 100,
         "«1984» Джорджа Орвелла – це моторошний і пророчий роман-антиутопія, що став класикою світової літератури. Опублікований у 1949 році, він змальовує жахливий світ тоталітарної держави Океанії, де панує всевладний режим на чолі з таємничим Великим Братом. Партія контролює кожен аспект життя громадян, від їхніх дій до думок, використовуючи постійний нагляд через телеекрани, поліцію думок та новомову – мову, створену для обмеження свободи мислення. Головний герой, Вінстон Сміт, працює в Міністерстві Правди, переписуючи історію відповідно до поточної лінії партії. Однак у глибині душі він ненавидить систему і починає вести таємний щоденник, що є смертельно небезпечним злочином. Його бунт, пошук правди та заборонене кохання з Джулією стають центральною темою роману. «1984» – це потужне попередження про небезпеку тоталітаризму, втрату індивідуальності, маніпуляцію інформацією та важливість свободи думки. Книга залишається надзвичайно актуальною, змушуючи замислюватися над сучасними суспільними процесами.",
         "англійська", 328, "D:\\projects\\DB_Kurs\\QtAPP\\cover_img\\1984.jpg", {"Orwell"}, "Наукова фантастика"},
        {"Animal Farm", "978-0-451-52634-2", QDate(1945, 8, 17), "Penguin Books", 120.00, 120,
         "«Колгосп тварин» (Animal Farm) Джорджа Орвелла – це гостра сатирична алегорія, що викриває події, які призвели до Російської революції 1917 року та подальшого встановлення сталінського режиму в Радянському Союзі. Написана у формі казки про тварин, які виганяють свого жорстокого господаря, фермера Джонса, і намагаються створити власне суспільство рівності та справедливості, книга майстерно показує, як ідеали революції поступово спотворюються. Влада на фермі переходить до свиней, які виявляються ще більш хитрими та безжальними експлуататорами, ніж люди. На чолі з Наполеоном (алегорія на Сталіна), вони встановлюють диктатуру, переписують історію, використовують пропаганду та терор для утримання контролю. Знаменитий лозунг «Усі тварини рівні» поступово трансформується в «Усі тварини рівні, але деякі рівніші за інших». «Колгосп тварин» – це позачасове попередження про небезпеку зловживання владою, маніпуляції та зради революційних ідеалів. Книга легко читається, але залишає глибокий слід, змушуючи аналізувати політичні процеси.",
         "англійська", 144, "D:\\projects\\DB_Kurs\\QtAPP\\cover_img\\Animal_Farm.jpg", {"Orwell"}, "Сучасна проза"},
        {"Harry Potter and the Philosopher's Stone", "978-0-7475-3269-9", QDate(1997, 6, 26), "Bloomsbury", 350.00, 80,
         "«Гаррі Поттер і філософський камінь» – перша книга з неймовірно популярної серії Дж. К. Роулінг, яка відкрила двері у чарівний світ для мільйонів читачів по всьому світу. Історія починається зі знайомства з Гаррі Поттером, хлопчиком-сиротою, який живе зі своїми жорстокими родичами Дурслями. У свій одинадцятий день народження Гаррі дізнається шокуючу правду: він – чарівник, і його чекає навчання у Гоґвортсі, школі чарів і чаклунства. Разом зі своїми новими друзями, Роном Візлі та Герміоною Ґрейнджер, Гаррі поринає у світ магії, квідичу, таємничих заклять та дивовижних істот. Однак радість навчання затьмарюється зловісною таємницею, пов'язаною з філософським каменем – легендарним артефактом, що дарує безсмертя. Гаррі та його друзям доведеться розгадати загадку і протистояти темним силам, які прагнуть заволодіти каменем. Ця книга – захоплива пригода, сповнена дружби, сміливості, гумору та магії, яка закладає фундамент для всієї епічної саги про хлопчика, який вижив.",
         "англійська", 223, "D:\\projects\\DB_Kurs\\QtAPP\\cover_img\\Harry Potter and the Philosopher's Stone.jpg", {"Rowling"}, "Фентезі"}, // Note: Publisher not in list
        {"The Shining", "978-0-385-12167-5", QDate(1977, 1, 28), "Doubleday", 280.00, 65,
         "«Сяйво» Стівена Кінга – це культовий роман жахів, що занурює читача в атмосферу ізоляції, божевілля та надприродного терору. Джек Торренс, письменник-початківець, який бореться з алкоголізмом та власними демонами, отримує роботу зимового доглядача у віддаленому готелі «Оверлук» у Скелястих горах Колорадо. Разом із дружиною Венді та маленьким сином Денні, який володіє екстрасенсорним даром, відомим як «сяйво», Джек сподівається знайти спокій та натхнення для написання п'єси. Однак готель, відрізаний від світу сніговими заметами, має власну темну історію та зловісну енергетику. Привиди минулого починають переслідувати мешканців, а надприродні сили готелю впливають на Джека, поступово доводячи його до божевілля. Денні, завдяки своєму «сяйву», бачить жахливі видіння та відчуває наростаючу загрозу. «Сяйво» – це майстерно написаний психологічний трилер, що досліджує теми залежності, насильства в сім'ї та боротьби людини з внутрішньою темрявою, підсиленою зловісним місцем.",
         "англійська", 447, "D:\\projects\\DB_Kurs\\QtAPP\\cover_img\\The_Shining.jpg", {"King"}, "Жахи"}, // Note: Publisher not in list
        {"It", "978-0-670-81302-5", QDate(1986, 9, 15), "Viking", 450.00, 45,
         "«Воно» Стівена Кінга – це монументальний епічний роман жахів, який став одним із найвідоміших творів автора. Історія розгортається у двох часових площинах у вигаданому містечку Деррі, штат Мен. У 1958 році семеро друзів-підлітків, які називають себе «Клубом Невдах», стикаються зі стародавнім космічним злом, що живе під містом і кожні 27 років пробуджується, щоб харчуватися страхами та життями дітей. Це зло приймає різні форми, найчастіше – моторошного клоуна Пеннівайза. Дітям вдається перемогти Воно, але вони дають клятву повернутися, якщо зло коли-небудь знову з'явиться. Через 27 років, у 1985 році, жахливі вбивства в Деррі починаються знову. Майк Генлон, єдиний з «Невдах», хто залишився в місті, скликає своїх друзів дитинства, які вже стали дорослими і майже забули про події минулого. Їм доведеться знову об'єднатися, подолати власні страхи та вступити у фінальну битву з Воно. Це глибока історія про дружбу, травми дитинства, силу пам'яті та боротьбу добра зі злом.",
         "англійська", 1138, "D:\\projects\\DB_Kurs\\QtAPP\\cover_img\\it.jpg", {"King"}, "Жахи"}, // Note: Publisher not in list
        {"Norwegian Wood", "978-0-375-70402-4", QDate(1987, 1, 1), "Vintage International", 240.00, 70,
         "«Норвезький ліс» Харукі Муракамі – це ностальгічний та меланхолійний роман про дорослішання, перше кохання, втрату та пошук себе на тлі студентських протестів у Токіо кінця 1960-х років. Головний герой, Тоору Ватанабе, згадує свої студентські роки та складні стосунки з двома дівчатами: Наоко, емоційно тендітною подругою його найкращого друга Кідзукі, який скоїв самогубство, та Мідорі, жвавою, незалежною та ексцентричною однокурсницею. Роман досліджує теми психічного здоров'я, сексуальності, смерті та складності людських взаємин. Ватанабе розривається між своїм обов'язком перед Наоко, яка бореться з глибокою депресією, та потягом до Мідорі, яка уособлює життя та надію. Муракамі майстерно передає атмосферу епохи, настрої молоді, їхні пошуки сенсу та кохання. Назва роману відсилає до однойменної пісні The Beatles, яка стає лейтмотивом твору, підкреслюючи його ностальгійний та сумний настрій. Це глибоко особиста та емоційна історія, яка залишає тривале враження.",
         "англійська", 296, "D:\\projects\\DB_Kurs\\QtAPP\\cover_img\\norwedianWood.jpg", {"Murakami"}, "Сучасна проза"}, // Note: Publisher not in list
        {"American Gods", "978-0-380-97365-1", QDate(2001, 6, 19), "HarperCollins", 320.00, 50,
         "«Американські боги» Ніла Геймана – це масштабний фентезійний роман, що поєднує елементи міфології, дорожньої пригоди та соціальної сатири. Головний герой, Тінь Мун, виходить із в'язниці на кілька днів раніше через раптову смерть дружини. Дорогою на похорон він зустрічає загадкового старого на ім'я Містер Середа, який пропонує Тіні роботу охоронця. Погодившись, Тінь опиняється втягнутим у прихований світ, де старі боги з міфологій усього світу, привезені до Америки іммігрантами протягом століть, живуть серед людей, втрачаючи свою силу через брак віри. Містер Середа, який насправді є скандинавським богом Одіном, збирає старих богів для епічної битви проти нових американських богів – уособлень технологій, медіа, грошей та сучасної культури. Тінь подорожує Америкою разом із Середою, зустрічаючи дивовижних персонажів та стаючи свідком дивних подій. Роман досліджує теми віри, ідентичності, імміграції та самої душі Америки. Це захоплива, багатошарова історія, сповнена оригінальних ідей та незабутніх образів.",
         "англійська", 465, "D:\\projects\\DB_Kurs\\QtAPP\\cover_img\\American Gods.jpg", {"Gaiman"}, "Фентезі"},
        {"The Handmaid's Tale", "978-0-385-49081-8", QDate(1985, 1, 1), "McClelland and Stewart", 270.00, 75,
         "«Оповідь служниці» Маргарет Етвуд – це потужний і тривожний антиутопічний роман, що змальовує тоталітарне теократичне суспільство Гілеад, яке виникло на території колишніх Сполучених Штатів Америки після державного перевороту. В умовах різкого падіння народжуваності режим позбавляє жінок усіх прав, класифікуючи їх за репродуктивною функцією. Головна героїня, Оффред (буквально «належить Фреду»), є Служницею – однією з небагатьох фертильних жінок, змушених служити Командорам та їхнім безплідним Дружинам, виношуючи для них дітей. Роман ведеться від першої особи Оффред, яка згадує своє минуле життя, коли вона мала ім'я, родину та свободу, і описує своє теперішнє існування, сповнене страху, приниження та боротьби за виживання. Етвуд майстерно створює гнітючу атмосферу суспільства, де панує релігійний фундаменталізм, мізогінія та постійний нагляд. «Оповідь служниці» – це гостра критика політичних та соціальних тенденцій, що досліджує теми жіночої ідентичності, репродуктивних прав, влади та опору.",
         "англійська", 311, "D:\\projects\\DB_Kurs\\QtAPP\\cover_img\\The Handmaid's Tale.jpg", {"Atwood"}, "Наукова фантастика"}, // Note: Publisher not in list
        {"Sapiens: A Brief History of Humankind", "978-0-06-231609-7", QDate(2011, 1, 1), "HarperCollins", 400.00, 90,
         "«Sapiens: Людина розумна. Коротка історія людства» Юваля Ноя Харарі – це захоплива науково-популярна книга, що пропонує масштабний огляд історії нашого виду від появи Homo sapiens у Східній Африці до політичних та технологічних революцій XXI століття. Харарі досліджує ключові моменти еволюції людства: когнітивну революцію, яка дозволила нам спілкуватися та створювати складні соціальні структури; аграрну революцію, що призвела до осілого способу життя та виникнення цивілізацій; об'єднання людства через гроші, релігії та імперії; та наукову революцію, яка кардинально змінила наше розуміння світу та наші можливості. Автор розглядає, як біологія та історія визначили наше сьогодення, і ставить провокаційні питання про наше майбутнє: куди ми рухаємося як вид? Чи зробили нас технологічні досягнення щасливішими? Які виклики чекають на нас попереду? Написана доступною мовою, книга поєднує знання з історії, біології, антропології та економіки, пропонуючи свіжий та глибокий погляд на те, що означає бути людиною.",
         "англійська", 464, "D:\\projects\\DB_Kurs\\QtAPP\\cover_img\\Sapiens.jpg", {"Harari"}, "Науково-популярне"},
        {"The Last Wish (The Witcher)", "978-0-316-43896-9", QDate(1993, 1, 1), "Orbit", 290.00, 60,
         "«Останнє бажання» – перша книга з легендарної фентезійної саги «Відьмак» польського письменника Анджея Сапковського. Це збірка оповідань, що знайомить читача з головним героєм – Ґеральтом з Рівії, професійним мисливцем на монстрів, відьмаком. У цьому похмурому, морально неоднозначному світі, натхненному слов'янською міфологією та класичними казками, Ґеральт подорожує, виконуючи небезпечні замовлення: вбиває чудовиськ, знімає прокляття та розплутує складні інтриги. Оповідання майстерно переплітають захопливі бойові сцени, детективні елементи та глибокі філософські роздуми про природу зла, вибір та ціну нейтралітету. Сапковський деконструює знайомі казкові сюжети, надаючи їм реалістичності та дорослого погляду. Ми зустрічаємо харизматичних персонажів, таких як чародійка Йеннефер та бард Любисток, які стануть важливими фігурами у подальшій долі Ґеральта. «Останнє бажання» – це чудовий вступ у світ Відьмака, сповнений пригод, чорного гумору та складних моральних дилем.",
         "англійська", 400, "D:\\projects\\DB_Kurs\\QtAPP\\cover_img\\The Last Wish (The Witcher).jpg", {"Sapkowski"}, "Фентезі"}, // Note: Publisher not in list
        {"The Hobbit", "978-0-547-92822-7", QDate(1937, 9, 21), "Houghton Mifflin Harcourt", 220.00, 110,
         "«Гобіт, або Туди і звідти» Дж. Р. Р. Толкіна – це чарівна та захоплива казкова повість, яка стала прелюдією до монументального «Володаря Перснів». Книга розповідає про неймовірні пригоди Більбо Беггінса, поважного гобіта, який понад усе цінує домашній затишок та спокійне життя у своїй нірці під пагорбом. Однак його життя кардинально змінюється, коли до нього несподівано навідується чарівник Ґандальф та тринадцять гномів на чолі з Торіном Дубощитом. Вони вмовляють Більбо приєднатися до їхньої небезпечної експедиції – походу до Самотньої Гори, щоб повернути скарби гномів, захоплені жахливим драконом Смауґом. Неохоче погодившись, Більбо вирушає у подорож, сповнену небезпек та дивовижних зустрічей. Він стикається з тролями, гоблінами, гігантськими павуками, ельфами та людьми, проявляючи несподівану для себе сміливість та кмітливість. Саме під час цієї подорожі Більбо знаходить таємничий Перстень. «Гобіт» – це класика дитячої літератури, яка однаково захоплює і дорослих своєю атмосферою, гумором та мудрістю.",
         "англійська", 310, "D:\\projects\\DB_Kurs\\QtAPP\\cover_img\\The Hobbit.jpg", {"Tolkien"}, "Фентезі"}, // Note: Publisher not in list
        {"The Lord of the Rings", "978-0-618-26027-9", QDate(1954, 7, 29), "Allen & Unwin", 600.00, 40,
         "«Володар Перснів» Дж. Р. Р. Толкіна – це епохальний шедевр фентезі, одна з найвизначніших книг XX століття, що створила цілий всесвіт Середзем'я. Роман є продовженням повісті «Гобіт» і розповідає про Велику війну за Перстень Всевладдя, створений Темним Володарем Сауроном для підкорення вільних народів Середзем'я. Головний герой – молодий гобіт Фродо Беггінс, племінник Більбо, отримує у спадок той самий Перстень і дізнається про його жахливу силу та призначення. Разом із вірними друзями-гобітами, чарівником Ґандальфом, спадкоємцем трону Ґондору Араґорном, ельфом Леґоласом, гномом Ґімлі та представником людей Боромиром, Фродо вирушає у небезпечну подорож до Мордору – єдиного місця, де Перстень може бути знищений у вогні Фатальної Гори. Їхній шлях сповнений випробувань, битв, зрад та героїчних вчинків. Толкін створив неймовірно деталізований світ з власною історією, мовами, народами та міфологією. «Володар Перснів» – це глибока алегорія боротьби добра зі злом, історія про дружбу, відвагу, самопожертву та надію.",
         "англійська", 1178, "D:\\projects\\DB_Kurs\\QtAPP\\cover_img\\The Lord of the Rings.jpg", {"Tolkien"}, "Фентезі"} // Note: Publisher not in list
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
        // Переконуємось, що 'description' є в списку стовпців
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
                bookQuery.bindValue(":description", book.description.isEmpty() ? QVariant(QVariant::String) : book.description); // Прив'язуємо опис (з перевіркою на порожній рядок)
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


    // 4. Customer (Генерація тестових клієнтів з паролями)
    if (success) {
        qInfo() << "Populating table customer (generating test data with passwords)...";
        QString insertCustomerSQL = R"(
        INSERT INTO customer (first_name, last_name, email, phone, address, password_hash, loyalty_program, join_date, loyalty_points)
        VALUES (:first_name, :last_name, :email, :phone, :address, :password_hash, :loyalty_program, :join_date, :loyalty_points)
        RETURNING customer_id;
    )"; // Додано password_hash
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

            // Генерація та хешування пароля (приклад: "password" + email)
            QString plainPassword = "password" + query.boundValue(":email").toString();
            QByteArray passwordHashBytes = QCryptographicHash::hash(plainPassword.toUtf8(), QCryptographicHash::Sha256);
            QString passwordHashHex = QString::fromUtf8(passwordHashBytes.toHex()); // Перетворюємо в QString
            query.bindValue(":password_hash", passwordHashHex); // Прив'язуємо QString

            if (executeInsertQuery(query, QString("Customer %1").arg(i+1), lastId)) {
                customerIds.append(lastId.toInt());
                // Цей рядок виводить email та пароль у лог при генерації даних
                qDebug() << "Generated customer" << query.boundValue(":email").toString() << "with password:" << plainPassword; // Тільки для тестування!
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

    // 8. comment (Генерація тестових коментарів)
    if (success && !bookIds.isEmpty() && !customerIds.isEmpty()) {
        qInfo() << "Populating table comment (generating test data)...";
        QString insertCommentSQL = R"(
            INSERT INTO comment (book_id, customer_id, comment_text, comment_date, rating)
            VALUES (:book_id, :customer_id, :comment_text, :comment_date, :rating);
        )";
        if (!query.prepare(insertCommentSQL)) {
            qCritical() << "Помилка підготовки запиту для comment:" << query.lastError().text();
            success = false;
        } else {
            int commentsCreated = 0;
            int targetCommentCount = numberOfRecords * 3; // Спробуємо створити більше коментарів
            QStringList sampleComments = {
                "Чудова книга!", "Дуже сподобалось.", "Рекомендую!", "Неймовірна історія.",
                "Захоплює з перших сторінок.", "Не міг відірватися.", "Варто прочитати.",
                "Глибокий зміст.", "Цікавий сюжет.", "Добре написано.", "Непогано.",
                "Очікував більшого.", "На один раз.", "Не дуже вразило.", "Спірно."
            };

            for (int i = 0; i < targetCommentCount && success; ++i) {
                int bookId = bookIds.at(QRandomGenerator::global()->bounded(bookIds.size()));
                int customerId = customerIds.at(QRandomGenerator::global()->bounded(customerIds.size()));
                QString commentText = sampleComments.at(QRandomGenerator::global()->bounded(sampleComments.size()));
                QDateTime commentDate = randomDateTime(QDateTime::currentDateTime().addDays(-180), QDateTime::currentDateTime());
                int rating = QRandomGenerator::global()->bounded(0, 6); // 0-5

                query.bindValue(":book_id", bookId);
                query.bindValue(":customer_id", customerId);
                query.bindValue(":comment_text", commentText);
                query.bindValue(":comment_date", commentDate);
                query.bindValue(":rating", rating == 0 ? QVariant(QVariant::Int) : rating); // NULL if 0

                if (query.exec()) {
                    commentsCreated++;
                } else {
                    qCritical().noquote() << QString("Error executing prepared INSERT (Comment %1):").arg(commentsCreated + 1);
                    qCritical() << query.lastError().text();
                    qCritical() << "Bound values:" << query.boundValues();
                    success = false; // Any error here is critical
                }
            }
             qInfo() << "Created" << commentsCreated << "comments.";
        }
    }


    // Завершуємо транзакцію (перенесено з кінця функції)
    if (success) {
        if (m_db.commit()) {
            qInfo() << "Data population transaction committed successfully. Added real books/authors, comments and ~" << numberOfRecords << "customers/orders.";
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

