#include "database.h"
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

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
        // authorInfo.found = true; // Поле found не визначено в AuthorDisplayInfo

        authors.append(authorInfo);
        count++;
    }
    qInfo() << "Processed" << count << "authors for display.";

    return authors;
}

// Реалізація нового методу для отримання детальної інформації про автора
AuthorDetailsInfo DatabaseManager::getAuthorDetails(int authorId) const
{
    AuthorDetailsInfo details;
    details.found = false;
    if (!m_isConnected || !m_db.isOpen() || authorId <= 0) {
        qWarning() << "Неможливо отримати деталі автора: немає з'єднання або невірний authorId.";
        return details;
    }

    // --- Отримання основної інформації про автора ---
    const QString authorSql = R"(
        SELECT
            author_id, first_name, last_name, nationality, image_path, biography, birth_date, death_date
        FROM author
        WHERE author_id = :authorId
        LIMIT 1;
    )";

    // --- Тимчасова зміна для діагностики помилки "EXECUTE (9)" ---
    // !!! ПОПЕРЕДЖЕННЯ: Цей підхід вразливий до SQL ін'єкцій і використовується лише для тестування.
    // !!! Поверніться до prepare/bindValue після діагностики.
    QString directAuthorSql = QString(R"(
        SELECT
            author_id, first_name, last_name, nationality, image_path, biography, birth_date, death_date
        FROM author
        WHERE author_id = %1
        LIMIT 1;
    )").arg(authorId); // Пряма підстановка ID

    QSqlQuery authorQuery(m_db);
    qInfo() << "Executing DIRECT SQL to get author details for author ID:" << authorId;
    qInfo() << "Direct SQL:" << directAuthorSql; // Логуємо прямий запит

    if (!authorQuery.exec(directAuthorSql)) { // Виконуємо прямий запит
        qCritical() << "Помилка при отриманні деталей автора (прямий запит) для author ID '" << authorId << "':";
        qCritical() << authorQuery.lastError().text();
        qCritical() << "SQL запит:" << directAuthorSql; // Логуємо прямий запит у разі помилки
        return details;
    }
    // --- Кінець тимчасової зміни ---

    if (authorQuery.next()) {
        details.authorId = authorQuery.value("author_id").toInt();
        details.firstName = authorQuery.value("first_name").toString();
        details.lastName = authorQuery.value("last_name").toString();
        details.nationality = authorQuery.value("nationality").toString();
        details.imagePath = authorQuery.value("image_path").toString();
        details.biography = authorQuery.value("biography").toString();
        details.birthDate = authorQuery.value("birth_date").toDate();
        details.deathDate = authorQuery.value("death_date").toDate(); // Може бути NULL -> invalid QDate
        details.found = true;
        qInfo() << "Author details found for author ID:" << authorId;
    } else {
        qInfo() << "Author details not found for author ID:" << authorId;
        return details; // Якщо автора не знайдено, повертаємо одразу
    }

    // --- Отримання книг цього автора ---
    // Використовуємо запит, схожий на getBookDisplayInfoById, але фільтруємо за author_id
    const QString booksSql = R"(
        SELECT
            b.book_id, b.title, b.price, b.cover_image_path, b.stock_quantity, b.genre,
            STRING_AGG(DISTINCT a_other.first_name || ' ' || a_other.last_name, ', ') AS authors -- Збираємо ВСІХ авторів книги
        FROM book b
        INNER JOIN book_author ba ON b.book_id = ba.book_id
        LEFT JOIN book_author ba_other ON b.book_id = ba_other.book_id -- Ще один join для збору всіх авторів
        LEFT JOIN author a_other ON ba_other.author_id = a_other.author_id
        WHERE ba.author_id = :authorId -- Фільтруємо за ID потрібного автора
        GROUP BY b.book_id, b.title, b.price, b.cover_image_path, b.stock_quantity, b.genre
        ORDER BY b.title;
    )";

    QSqlQuery booksQuery(m_db);
    booksQuery.prepare(booksSql);
    booksQuery.bindValue(":authorId", authorId);

    qInfo() << "Executing SQL to get books for author ID:" << authorId;
    if (!booksQuery.exec()) {
        qCritical() << "Помилка при отриманні книг для автора ID '" << authorId << "':";
        qCritical() << booksQuery.lastError().text();
        qCritical() << "SQL запит:" << booksQuery.lastQuery();
        // Не повертаємо помилку, просто список книг буде порожнім
    } else {
        qInfo() << "Successfully fetched books for author ID:" << authorId << ". Processing results...";
        int count = 0;
        while (booksQuery.next()) {
            BookDisplayInfo bookInfo;
            bookInfo.bookId = booksQuery.value("book_id").toInt();
            bookInfo.title = booksQuery.value("title").toString();
            bookInfo.price = booksQuery.value("price").toDouble();
            bookInfo.coverImagePath = booksQuery.value("cover_image_path").toString();
            bookInfo.stockQuantity = booksQuery.value("stock_quantity").toInt();
            bookInfo.authors = booksQuery.value("authors").toString(); // Всі автори книги
            bookInfo.genre = booksQuery.value("genre").toString();
            bookInfo.found = true; // Книга знайдена

            if (booksQuery.value("authors").isNull()) {
                 bookInfo.authors = "";
            }
            details.books.append(bookInfo);
            count++;
        }
         qInfo() << "Processed" << count << "books for author ID:" << authorId;
    }

    return details;
}
