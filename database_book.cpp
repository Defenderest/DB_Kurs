#include "database.h"
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QStringList> // Для getSearchSuggestions
#include <QDate> // Для дат

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
        bookInfo.found = true; // Книга знайдена, якщо запит її повернув

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
    details.found = false; // Явно встановлюємо за замовчуванням
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
        GROUP BY b.book_id, p.name, b.title, b.price, b.cover_image_path, b.stock_quantity, b.genre, b.description, b.publication_date, b.isbn, b.page_count, b.language -- Групуємо за всіма полями книги та видавця
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

    // Отримуємо коментарі до книги (викликаємо метод з database_comment.cpp)
    // Перевірка на null для m_dbManager не потрібна, бо ми вже всередині класу
    details.comments = getBookComments(bookId);
    qInfo() << "Fetched" << details.comments.size() << "comments for book ID:" << bookId;


    return details;
}

// Реалізація нового методу для отримання BookDisplayInfo за ID
BookDisplayInfo DatabaseManager::getBookDisplayInfoById(int bookId) const
{
    BookDisplayInfo bookInfo; // found = false за замовчуванням
    bookInfo.found = false;
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
        GROUP BY b.book_id, b.title, b.price, b.cover_image_path, b.stock_quantity, b.genre -- Групуємо за ID книги та іншими полями
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
        bookInfo.found = true;

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

// Оновлена реалізація методу для отримання пропозицій пошуку
QList<SearchSuggestionInfo> DatabaseManager::getSearchSuggestions(const QString &prefix, int limit) const
{
    QList<SearchSuggestionInfo> suggestions;
    // Змінено умову: тепер шукаємо з першої літери
    if (!m_isConnected || !m_db.isOpen() || prefix.isEmpty()) {
        // qWarning() << "Неможливо отримати пропозиції: немає з'єднання або префікс порожній.";
        return suggestions; // Повертаємо порожній список
    }

    // Оновлений SQL-запит: отримуємо тип, ID та шлях до зображення
    const QString sql = R"(
        SELECT 'book' AS type, book_id AS id, title AS display_text, cover_image_path AS image_path
        FROM book
        WHERE LOWER(title) LIKE LOWER(:prefix) || '%'
        UNION ALL -- Використовуємо UNION ALL для швидкості, сортування буде в кінці
        SELECT 'author' AS type, author_id AS id, first_name || ' ' || last_name AS display_text, image_path
        FROM author
        WHERE LOWER(first_name || ' ' || last_name) LIKE LOWER(:prefix) || '%'
        ORDER BY display_text -- Сортуємо за текстом пропозиції
        LIMIT :total_limit;
    )";

    QSqlQuery query(m_db);
    query.prepare(sql);
    query.bindValue(":prefix", prefix);
    query.bindValue(":total_limit", limit > 0 ? limit : 10); // Загальне обмеження

    qInfo() << "Executing SQL to get rich search suggestions for prefix:" << prefix << "with limit:" << query.boundValue(":total_limit").toInt();
    if (!query.exec()) {
        qCritical() << "Помилка при отриманні розширених пропозицій пошуку для префікса '" << prefix << "':";
        qCritical() << query.lastError().text();
        qCritical() << "SQL запит:" << query.lastQuery();
        qCritical() << "Bound values:" << query.boundValues();
        return suggestions; // Повертаємо порожній список у разі помилки
    }

    qInfo() << "Successfully fetched rich suggestions. Processing results...";
    int count = 0;
    while (query.next()) {
        SearchSuggestionInfo suggestion;
        QString typeStr = query.value("type").toString();
        suggestion.id = query.value("id").toInt();
        suggestion.displayText = query.value("display_text").toString();
        suggestion.imagePath = query.value("image_path").toString();

        if (typeStr == "book") {
            suggestion.type = SearchSuggestionInfo::Book;
        } else if (typeStr == "author") {
            suggestion.type = SearchSuggestionInfo::Author;
        } else {
            qWarning() << "Unknown suggestion type encountered:" << typeStr;
            continue; // Пропускаємо невідомий тип
        }

        suggestions.append(suggestion);
        count++;
    }
    qInfo() << "Processed" << count << "rich suggestions for prefix" << prefix;

    return suggestions;
}
