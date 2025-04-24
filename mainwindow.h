#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QList> // Для списку книг
#include <QHBoxLayout>
#include "database.h"
#include <QWidget> // Додано для QWidget* у конструкторі та типів повернення
#include <QLayout>      // Додано для QLayout* у clearLayout
#include <QDate>        // Додано для QDate у CustomerProfileInfo
#include <QPropertyAnimation> // Для анімації бокової панелі
#include <QEvent>       // Для eventFilter
#include <QEnterEvent>  // Для подій наведення миші
#include <QMap>         // Для збереження тексту кнопок
#include <QLineEdit>    // Додано для редагування телефону
#include <QCompleter>   // Додано для автодоповнення
#include <QStringListModel> // Додано для моделі автодоповнення
#include <QMouseEvent> // Додано для обробки кліків миші
#include <QMap>        // Додано для QMap (використовується для кошика)
#include <QSpinBox>    // Додано для QSpinBox (використовується в кошику)
#include <QScrollArea> // Додано для QScrollArea (новий кошик)
#include <QTimer>      // Додано для таймера банера
#include <QStringList> // Додано для списку шляхів до зображень банера


// Forward declarations
class DatabaseManager;
// class QTableWidget; // Більше не використовується для кошика
struct CustomerProfileInfo;
struct BookDetailsInfo; // Додано forward declaration
class QLabel;
class QVBoxLayout;
class QGridLayout;
class QPushButton;
class QFrame;
class QStackedWidget;
class QPropertyAnimation;
class QLabel; // Додано forward declaration для QLabel у m_cartSubtotalLabels

// Структура CartItem тепер визначена в datatypes.h

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    // Змінено конструктор: приймає DatabaseManager та ID користувача
    explicit MainWindow(DatabaseManager *dbManager, int customerId, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // Слоти для кнопок навігації
    void on_navHomeButton_clicked();   // Перейменовано
    void on_navBooksButton_clicked();
    void on_navAuthorsButton_clicked();
    void on_navOrdersButton_clicked();
    void on_navProfileButton_clicked(); // Слот для кнопки профілю в бічній панелі
    void on_editProfileButton_clicked(); // Слот для кнопки редагування профілю
    void on_saveProfileButton_clicked();
    void updateSearchSuggestions(const QString &text);
    void showBookDetails(int bookId);
    void on_addToCartButtonClicked(int bookId); // Слот для кнопки "Додати в кошик"
    void on_cartButton_clicked(); // Слот для кнопки кошика в хедері
    void updateCartItemQuantity(int bookId, int quantity); // Слот для зміни кількості
    void removeCartItem(int bookId); // Слот для видалення товару
    void on_placeOrderButton_clicked(); // Слот для кнопки "Оформити замовлення"
    void on_sendCommentButton_clicked(); // Слот для кнопки відправки коментаря
    void showOrderDetailsPlaceholder(int orderId); // Тимчасовий слот для деталей замовлення
    void showNextBanner(); // Слот для перемикання банера

private:
    // Допоміжні функції для відображення даних
    void displayComments(const QList<CommentDisplayInfo> &comments); // Відображення списку коментарів
    void refreshBookComments(); // Оновлення списку коментарів на сторінці деталей
    void displayBooks(const QList<BookDisplayInfo> &books); // Метод для відображення книг
    void displayAuthors(const QList<AuthorDisplayInfo> &authors);
    void displayBooksInHorizontalLayout(const QList<BookDisplayInfo> &books, QHBoxLayout* layout);
    QWidget* createBookCardWidget(const BookDisplayInfo &bookInfo);
    QWidget* createAuthorCardWidget(const AuthorDisplayInfo &authorInfo);
    QWidget* createCommentWidget(const CommentDisplayInfo &commentInfo); // Додано оголошення
    void populateProfilePanel(const CustomerProfileInfo &profileInfo); // Заповнення сторінки профілю

    // Функції для відображення замовлень
    QWidget* createOrderWidget(const OrderDisplayInfo &orderInfo);
    void displayOrders(const QList<OrderDisplayInfo> &orders);
    void loadAndDisplayOrders(); // Завантаження та відображення замовлень

    // Допоміжна функція для очищення layout
    void clearLayout(QLayout* layout);

    // Керування режимом редагування профілю
    void setProfileEditingEnabled(bool enabled);
    // Заповнення сторінки деталей книги
    void populateBookDetailsPage(const BookDetailsInfo &details);

    // Керування кошиком
    void populateCartPage(); // Заповнення сторінки кошика
    void updateCartTotal(); // Оновлення загальної суми кошика
    void updateCartIcon(); // Оновлення іконки кошика (кількість товарів)
    QWidget* createCartItemWidget(const CartItem &item, int bookId); // Створення віджету товару для кошика

    // Налаштування анімації та стану бічної панелі
    void setupSidebarAnimation();
    void toggleSidebar(bool expand);
    void setupSearchCompleter();
    // void setupBannerImage(); // Видалено, замінено на setupAutoBanner
    void setupAutoBanner(); // Налаштування автоматичного банера

    // Члени класу
    Ui::MainWindow *ui;
    DatabaseManager *m_dbManager; // Вказівник на менеджер БД
    int m_currentCustomerId;      // ID поточного користувача

    QPropertyAnimation *m_sidebarAnimation = nullptr; // Анімація для бокової панелі
    bool m_isSidebarExpanded = false;                 // Поточний стан панелі
    int m_collapsedWidth = 50;                        // Ширина згорнутої панелі
    int m_expandedWidth = 200;                       // Ширина розгорнутої панелі
    QMap<QPushButton*, QString> m_buttonOriginalText; // Зберігання оригінального тексту кнопок

    // Члени для автодоповнення пошуку
    QCompleter *m_searchCompleter = nullptr;
    QStringListModel *m_searchSuggestionModel = nullptr;

    // Кошик
    QMap<int, CartItem> m_cartItems; // Ключ - bookId (CartItem тепер з datatypes.h)
    QMap<int, QLabel*> m_cartSubtotalLabels; // Ключ - bookId, Значення - вказівник на мітку підсумку

    // ID книги, що відображається на сторінці деталей
    int m_currentBookDetailsId = -1;

    // Члени для автоматичного банера
    QTimer *m_bannerTimer = nullptr;
    QStringList m_bannerImagePaths;
    int m_currentBannerIndex = 0;
    QList<QRadioButton*> m_bannerIndicators; // Список індикаторів


protected:
    // Перехоплення подій для sidebarFrame
    bool eventFilter(QObject *watched, QEvent *event) override;

};
#endif // MAINWINDOW_H
