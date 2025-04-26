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
#include <QStandardItemModel> // Змінено з QStringListModel
#include <QMouseEvent> // Додано для обробки кліків миші
#include <QMap>        // Додано для QMap (використовується для кошика)
#include <QSpinBox>    // Додано для QSpinBox (використовується в кошику)
#include <QScrollArea> // Додано для QScrollArea (новий кошик)
#include <QTimer>      // Додано для таймера банера
#include <QStringList> // Додано для списку шляхів до зображень банера
#include <QRadioButton> // Додано для індикаторів банера
#include <QResizeEvent> // Додано для обробки зміни розміру
#include "searchsuggestiondelegate.h" // Додано включення делегата
#include "datatypes.h" // Додано для BookFilterCriteria


// Forward declarations
class DatabaseManager;
class QListWidget;     // Для списків фільтрів
// class QDoubleSpinBox;  // Замінено на QSlider та QLabel
class QSlider;         // Додано для фільтрів ціни
class QLabel;          // Додано для відображення значень ціни
class QCheckBox;       // Для фільтра "в наявності"
class QStandardItemModel; // Додано forward declaration
struct CustomerProfileInfo;
struct BookDetailsInfo;
class QLabel;
class QVBoxLayout;
class QGridLayout;
// Додамо forward declarations для нових віджетів профілю, якщо вони ще не включені
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
    void onSearchSuggestionActivated(const QModelIndex &index); // Слот для обробки вибору пропозиції
    void showAuthorDetails(int authorId); // Слот/метод для показу деталей автора
    void on_filterButton_clicked(); // Слот для кнопки відкриття/закриття панелі фільтрів
    void applyFilters(); // Слот для застосування фільтрів (буде змінено)
    void resetFilters(); // Слот для скидання фільтрів
    void onFilterCriteriaChanged(); // Новий слот для реагування на зміни
    void applyFiltersWithDelay(); // Новий слот для застосування із затримкою
    // Нові слоти для слайдерів ціни
    void onMinPriceSliderChanged(int value);
    void onMaxPriceSliderChanged(int value);

private:
    // Допоміжні функції для відображення даних
    void displayComments(const QList<CommentDisplayInfo> &comments); // Відображення списку коментарів
    void refreshBookComments(); // Оновлення списку коментарів на сторінці деталей
    // Оновлено сигнатуру displayBooks
    void displayBooks(const QList<BookDisplayInfo> &books, QGridLayout *targetLayout, QWidget *parentWidgetContext);
    void displayAuthors(const QList<AuthorDisplayInfo> &authors);
    // Допоміжна функція для відображення книг у горизонтальному layout
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
    // Заповнення сторінки деталей автора
    void populateAuthorDetailsPage(const AuthorDetailsInfo &details);

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
    void updateBannerImages(); // Оновлення зображень банера
    void setupFilterPanel(); // Налаштування панелі фільтрів
    void loadAndDisplayFilteredBooks(); // Завантаження та відображення книг з урахуванням фільтрів

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
    QStandardItemModel *m_searchSuggestionModel = nullptr; // Змінено тип моделі
    SearchSuggestionDelegate *m_searchDelegate = nullptr; // Додано вказівник на делегат

    // Кошик
    QMap<int, CartItem> m_cartItems; // Ключ - bookId (CartItem тепер з datatypes.h)
    QMap<int, QLabel*> m_cartSubtotalLabels; // Ключ - bookId, Значення - вказівник на мітку підсумку

    // ID книги та автора, що відображаються на сторінках деталей
    int m_currentBookDetailsId = -1;
    int m_currentAuthorDetailsId = -1;

    // Члени для автоматичного банера
    QTimer *m_bannerTimer = nullptr;
    QStringList m_bannerImagePaths;
    int m_currentBannerIndex = 0;
    QList<QRadioButton*> m_bannerIndicators; // Список індикаторів

    // Члени для панелі фільтрів
    QPropertyAnimation *m_filterPanelAnimation = nullptr;
    bool m_isFilterPanelVisible = false;
    int m_filterPanelWidth = 250; // Ширина панелі фільтрів
    BookFilterCriteria m_currentFilterCriteria; // Поточні критерії фільтрації

    // Вказівники на віджети фільтрів (припускаємо, що вони є в UI)
    QListWidget *m_genreFilterListWidget = nullptr;
    QListWidget *m_languageFilterListWidget = nullptr;
    // QDoubleSpinBox *m_minPriceFilterSpinBox = nullptr; // Замінено
    // QDoubleSpinBox *m_maxPriceFilterSpinBox = nullptr; // Замінено
    QSlider *m_minPriceSlider = nullptr; // Новий слайдер для мін. ціни
    QSlider *m_maxPriceSlider = nullptr; // Новий слайдер для макс. ціни
    QLabel *m_minPriceValueLabel = nullptr; // Мітка для значення мін. ціни
    QLabel *m_maxPriceValueLabel = nullptr; // Мітка для значення макс. ціни
    QCheckBox *m_inStockFilterCheckBox = nullptr;

    // Таймер для автоматичного застосування фільтрів
    QTimer *m_filterApplyTimer = nullptr;


protected:
    // Перехоплення подій для sidebarFrame
    bool eventFilter(QObject *watched, QEvent *event) override;
    // Перевизначення події зміни розміру вікна
    void resizeEvent(QResizeEvent *event) override;

};
#endif // MAINWINDOW_H
