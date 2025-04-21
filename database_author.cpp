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
