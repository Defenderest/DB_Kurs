#include "database.h"
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

QList<AuthorDisplayInfo> DatabaseManager::getAllAuthorsForDisplay() const
{
    QList<AuthorDisplayInfo> authors;
    if (!m_isConnected || !m_db.isOpen()) {
        qWarning() << "Неможливо отримати авторів: немає активного з'єднання з БД.";
        return authors;
    }

    const QString sql = getSqlQuery("GetAllAuthorsForDisplay");
    if (sql.isEmpty()) return authors;

    QSqlQuery query(m_db);
    qInfo() << "Executing SQL 'GetAllAuthorsForDisplay' to get authors for display...";
    if (!query.exec(sql)) {
        qCritical() << "Помилка при виконанні 'GetAllAuthorsForDisplay':";
        qCritical() << query.lastError().text();
        qCritical() << "SQL запит:" << sql;
        return authors;
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

AuthorDetailsInfo DatabaseManager::getAuthorDetails(int authorId) const
{
    AuthorDetailsInfo details;
    details.found = false;
    if (!m_isConnected || !m_db.isOpen() || authorId <= 0) {
        qWarning() << "Неможливо отримати деталі автора: немає з'єднання або невірний authorId.";
        return details;
    }

    const QString authorSql = getSqlQuery("GetAuthorDetailsById");
    if (authorSql.isEmpty()) return details;

    QSqlQuery authorQuery(m_db);
    if (!authorQuery.prepare(authorSql)) {
        qCritical() << "Помилка підготовки запиту 'GetAuthorDetailsById':" << authorQuery.lastError().text();
        return details;
    }
    authorQuery.bindValue(":authorId", authorId);

    qInfo() << "Executing SQL 'GetAuthorDetailsById' for author ID:" << authorId;
    if (!authorQuery.exec()) {
        qCritical() << "Помилка при виконанні 'GetAuthorDetailsById' для author ID '" << authorId << "':";
        qCritical() << authorQuery.lastError().text();
        qCritical() << "SQL запит:" << authorQuery.lastQuery();
        return details;
    }

    if (authorQuery.next()) {
        details.authorId = authorQuery.value("author_id").toInt();
        details.firstName = authorQuery.value("first_name").toString();
        details.lastName = authorQuery.value("last_name").toString();
        details.nationality = authorQuery.value("nationality").toString();
        details.imagePath = authorQuery.value("image_path").toString();
        details.biography = authorQuery.value("biography").toString();
        details.birthDate = authorQuery.value("birth_date").toDate();
        details.found = true;
        qInfo() << "Author details found for author ID:" << authorId;
    } else {
        qInfo() << "Author details not found for author ID:" << authorId;
        return details;
    }

    const QString booksSql = getSqlQuery("GetAuthorBooksForDisplay");
    if (booksSql.isEmpty()) return details;

    QSqlQuery booksQuery(m_db);
     if (!booksQuery.prepare(booksSql)) {
        qCritical() << "Помилка підготовки запиту 'GetAuthorBooksForDisplay':" << booksQuery.lastError().text();
        return details;
    }
    booksQuery.bindValue(":authorId", authorId);

    qInfo() << "Executing SQL 'GetAuthorBooksForDisplay' for author ID:" << authorId;
    if (!booksQuery.exec()) {
        qCritical() << "Помилка при виконанні 'GetAuthorBooksForDisplay' для автора ID '" << authorId << "':";
        qCritical() << booksQuery.lastError().text();
        qCritical() << "SQL запит:" << booksQuery.lastQuery();
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
            bookInfo.authors = booksQuery.value("authors").toString();
            bookInfo.genre = booksQuery.value("genre").toString();
            bookInfo.found = true;

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
