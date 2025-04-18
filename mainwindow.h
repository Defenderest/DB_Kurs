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

// Forward declarations
class DatabaseManager;
struct CustomerProfileInfo;
class QLabel;
class QVBoxLayout;
class QGridLayout;
class QPushButton;
class QFrame;
class QStackedWidget; // Додано
class QPropertyAnimation; // Додано

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
    void displayBooks(const QList<BookDisplayInfo> &books); // Метод для відображення книг

private slots:
    // Слоти для кнопок навігації
    void on_navButtonHome_clicked();
    void on_navButtonBooks_clicked();
    void on_navButtonAuthors_clicked();
    void on_navButtonOrders_clicked();
    void on_navButtonProfile_clicked();

    // Слот для завершення анімації (опціонально, якщо потрібні дії після)
    // void onSidebarAnimationFinished();

private:
    Ui::MainWindow *ui;
    DatabaseManager *m_dbManager; // Вказівник на менеджер БД
    int m_currentCustomerId;      // ID поточного користувача

    QPropertyAnimation *m_sidebarAnimation = nullptr; // Анімація для бокової панелі
    bool m_isSidebarExpanded = false;                 // Поточний стан панелі
    int m_collapsedWidth = 50;                        // Ширина згорнутої панелі
    int m_expandedWidth = 200;                       // Ширина розгорнутої панелі
    QMap<QPushButton*, QString> m_buttonOriginalText; // Зберігання оригінального тексту кнопок

    // Допоміжна функція для очищення layout
    void clearLayout(QLayout* layout);
    // Допоміжна функція для створення картки книги
    QWidget* createBookCardWidget(const BookDisplayInfo &bookInfo);
    // Допоміжна функція для відображення книг у горизонтальному layout
    void displayBooksInHorizontalLayout(const QList<BookDisplayInfo> &books, QHBoxLayout* layout);

    // Допоміжні функції для вкладки "Автори"
    QWidget* createAuthorCardWidget(const AuthorDisplayInfo &authorInfo);
    void displayAuthors(const QList<AuthorDisplayInfo> &authors);

    // Допоміжна функція для заповнення даних у вкладці профілю
    void populateProfilePanel(const CustomerProfileInfo &profileInfo); // Залишаємо, але буде використовуватись у pageProfile

    // Налаштування анімації
    void setupSidebarAnimation();
    // Функція для розгортання/згортання панелі
    void toggleSidebar(bool expand);

protected:
    // Перехоплення подій для sidebarFrame
    bool eventFilter(QObject *watched, QEvent *event) override;

};
#endif // MAINWINDOW_H
