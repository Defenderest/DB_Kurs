#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <QVariant> // Нужно для lastInsertId() или query.value()
#include <QVector>  // Для хранения сгенерированных ID
#include <QDebug>

class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager();

    bool connectToDatabase(const QString &host,
                           int port,
                           const QString &dbName,
                           const QString &user,
                           const QString &password);

    // Метод для создания структуры таблиц
    bool createSchemaTables();

    // Новый метод для заполнения таблиц тестовыми данными
    bool populateTestData(int numberOfRecords = 20); // По умолчанию 20 записей

    QSqlError lastError() const;
    void closeConnection();
    bool printAllData() const;

    // Новий метод для отримання книг для відображення
    QList<BookDisplayInfo> getAllBooksForDisplay() const;

private:
    QSqlDatabase m_db;
    bool m_isConnected = false;

    // Вспомогательная функция для выполнения запроса с проверкой ошибки
    bool executeQuery(QSqlQuery &query, const QString &sql, const QString &description);

    // Вспомогательная функция для выполнения подготовленного INSERT с возвратом ID
    bool executeInsertQuery(QSqlQuery &query, const QString &description, QVariant &insertedId);
};

#endif // DATABASE_H
