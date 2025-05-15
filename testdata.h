#ifndef TESTDATA_H
#define TESTDATA_H

#include <QString>
#include <QDate>
#include <QDateTime>
#include <QList>
#include <QMap>
#include <QVariant>

class DatabaseManager;
class QSqlQuery;

struct PublisherData {
    QString name;
    QString contactInfo;
    int dbId = -1;
};

struct AuthorData {
    QString firstName;
    QString lastName;
    QDate birthDate;
    QString nationality;
    QString imagePath;
    QString biography;
    int dbId = -1;
};

struct BookData {
    QString title;
    QString isbn;
    QDate publicationDate;
    QString publisherName;
    double price;
    int stockQuantity;
    QString description;
    QString language;
    int pageCount;
    QString coverImagePath;
    QStringList authorLastNames;
    QString genre;
    int dbId = -1;
    int publisherDbId = -1;
    QList<int> authorDbIds;
};

bool populateTestData(DatabaseManager *dbManager, int numberOfRecords = 20);

QDate randomDate(const QDate &minDate, const QDate &maxDate);
QDateTime randomDateTime(const QDateTime &minDateTime, const QDateTime &maxDateTime);

#endif // TESTDATA_H
