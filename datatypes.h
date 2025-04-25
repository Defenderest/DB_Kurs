#ifndef DATATYPES_H
#define DATATYPES_H

#include <QString>
#include <QDate>
#include <QDateTime>
#include <QList>
#include <QMap> // Потрібно для QMap у createOrder та CartItem

// --- Структури даних ---

// Структура для передачі даних книги в UI (перенесено з database.h)
struct BookDisplayInfo {
    int bookId;
    QString title;
    QString authors; // Об'єднані імена авторів
    double price;
    QString coverImagePath;
    int stockQuantity;
    QString genre; // Додано поле жанру
    bool found = false; // Прапорець, чи знайдено книгу
};

// Структура для передачі даних автора в UI (перенесено з database.h)
struct AuthorDisplayInfo {
    int authorId;
    QString firstName;
    QString lastName;
    QString nationality;
    QString imagePath; // Шлях до зображення автора
};

// Структура для передачі даних для входу (перенесено з database.h)
struct CustomerLoginInfo {
    int customerId = -1;
    QString passwordHash;
    bool found = false; // Прапорець, чи знайдено користувача
};

// Структура для передачі детальної інформації про книгу в UI (перенесено з database.h)
struct BookDetailsInfo {
    int bookId = -1;
    QString title;
    QString authors; // Об'єднані імена авторів
    double price = 0.0;
    QString coverImagePath;
    int stockQuantity = 0;
    QString genre;
    QString description;
    QString publisherName;
    QDate publicationDate;
    QString isbn;
    int pageCount = 0;
    QString language;
    bool found = false; // Прапорець, чи знайдено книгу
    // Поля для рейтингу та коментарів
    // double averageRating; // Можна додати середній рейтинг
    QList<struct CommentDisplayInfo> comments; // Список коментарів
};

// Структура для відображення одного коментаря (перенесено з database.h)
struct CommentDisplayInfo {
    QString authorName; // Ім'я та прізвище автора коментаря
    QDateTime commentDate;
    int rating; // 0-5 (0 - без оцінки)
    QString commentText;
};


// Структура для передачі даних для реєстрації нового користувача (перенесено з database.h)
struct CustomerRegistrationInfo {
    QString firstName;
    QString lastName;
    QString email;
    QString password; // Пароль у відкритому вигляді перед хешуванням
};

// Структура для позиції замовлення в UI (перенесено з database.h)
struct OrderItemDisplayInfo {
    QString bookTitle;
    int quantity;
    double pricePerUnit;
};

// Структура для статусу замовлення в UI (перенесено з database.h)
struct OrderStatusDisplayInfo {
    QString status;
    QDateTime statusDate;
    QString trackingNumber;
};

// Структура для повного замовлення в UI (перенесено з database.h)
struct OrderDisplayInfo {
    int orderId;
    QDateTime orderDate;
    double totalAmount;
    QString shippingAddress;
    QString paymentMethod;
    QList<OrderItemDisplayInfo> items;
    QList<OrderStatusDisplayInfo> statuses;
};


// Структура для передачі повної інформації профілю користувача в UI (перенесено з database.h)
struct CustomerProfileInfo {
    int customerId = -1;
    QString firstName;
    QString lastName;
    QString email;
    QString phone;
    QString address;
    QDate joinDate;
    bool loyaltyProgram = false;
    int loyaltyPoints = 0;
    bool found = false; // Прапорець, чи знайдено користувача
};

// Структура для елемента кошика (перенесено з mainwindow.h)
struct CartItem {
    BookDisplayInfo book; // Зберігаємо основну інформацію про книгу
    int quantity;
};

// Структура для передачі даних пропозиції пошуку
struct SearchSuggestionInfo {
    enum SuggestionType { Book, Author };

    QString displayText;    // Текст, що відображається (назва книги або ім'я автора)
    SuggestionType type;    // Тип пропозиції (книга чи автор)
    int id;                 // ID книги або автора
    QString imagePath;      // Шлях до обкладинки книги або портрета автора
    double price = 0.0;     // Ціна (актуально тільки для книг)
};

// Структура для передачі детальної інформації про автора в UI
struct AuthorDetailsInfo {
    int authorId = -1;
    QString firstName;
    QString lastName;
    QString nationality;
    QString imagePath;
    QString biography;
    QDate birthDate; // Додано дату народження
    // QDate deathDate; // Видалено, оскільки стовпця немає в БД
    QList<BookDisplayInfo> books; // Список книг цього автора
    bool found = false; // Прапорець, чи знайдено автора
};


#endif // DATATYPES_H
