#include "database.h"
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QDateTime> // Для дат

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
