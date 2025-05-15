#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QList>
#include <QHBoxLayout>
#include "database.h"
#include <QWidget>
#include <QLayout>
#include <QDate>
#include <QPropertyAnimation>
#include <QEvent>
#include <QEnterEvent>
#include <QMap>
#include <QLineEdit>
#include <QCompleter>
#include <QStandardItemModel>
#include <QMouseEvent>
#include <QMap>
#include <QSpinBox>
#include <QScrollArea>
#include <QTimer>
#include <QStringList>
#include <QRadioButton>
#include <QResizeEvent>
#include "searchsuggestiondelegate.h"
#include "datatypes.h"
#include "checkoutdialog.h"


class CheckoutDialog;
class DatabaseManager;
class QListWidget;
class RangeSlider;
class QLabel;
class QCheckBox;
class QStandardItemModel;
struct CustomerProfileInfo;
struct BookDetailsInfo;
class QLabel;
class QVBoxLayout;
class QGridLayout;
class QPushButton;
class QFrame;
class QStackedWidget;
class QPropertyAnimation;
class QLabel;
class QVBoxLayout;
class QLabel;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(DatabaseManager *dbManager, int customerId, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_navHomeButton_clicked();
    void on_navBooksButton_clicked();
    void on_navAuthorsButton_clicked();
    void on_navOrdersButton_clicked();
    void on_navProfileButton_clicked();
    void on_editProfileButton_clicked();
    void on_saveProfileButton_clicked();
    void updateSearchSuggestions(const QString &text);
    void showBookDetails(int bookId);
    void on_addToCartButtonClicked(int bookId);
    void on_cartButton_clicked();
    void updateCartItemQuantity(int bookId, int quantity);
    void removeCartItem(int bookId);
    void on_placeOrderButton_clicked();
    void on_sendCommentButton_clicked();
    void showOrderDetails(int orderId);
    void hideOrderDetailsPanel();
    void showNextBanner();
    void onSearchSuggestionActivated(const QModelIndex &index);
    void showAuthorDetails(int authorId);
    void on_filterButton_clicked();
    void applyFilters();
    void resetFilters();
    void onFilterCriteriaChanged();
    void applyFiltersWithDelay();
    void updateLowerPriceLabel(int value);
    void updateUpperPriceLabel(int value);
    void applyGenreFilter(const QString &genreName);
    void finalizeOrder(const QString &shippingAddress, const QString &paymentMethod);

private:
    void displayComments(const QList<CommentDisplayInfo> &comments);
    void refreshBookComments();
    void displayBooks(const QList<BookDisplayInfo> &books, QGridLayout *targetLayout, QWidget *parentWidgetContext);
    void displayAuthors(const QList<AuthorDisplayInfo> &authors);
    void displayBooksInHorizontalLayout(const QList<BookDisplayInfo> &books, QHBoxLayout* layout);
    QWidget* createBookCardWidget(const BookDisplayInfo &bookInfo);
    QWidget* createAuthorCardWidget(const AuthorDisplayInfo &authorInfo);
    QWidget* createCommentWidget(const CommentDisplayInfo &commentInfo);
    void populateProfilePanel(const CustomerProfileInfo &profileInfo);

    QWidget* createOrderWidget(const OrderDisplayInfo &orderInfo);
    void displayOrders(const QList<OrderDisplayInfo> &orders);
    void loadAndDisplayOrders();

    void clearLayout(QLayout* layout);

    void setProfileEditingEnabled(bool enabled);
    void populateBookDetailsPage(const BookDetailsInfo &details);
    void populateAuthorDetailsPage(const AuthorDetailsInfo &details);
    void populateOrderDetailsPanel(const OrderDisplayInfo &orderInfo);

    void populateCartPage();
    void updateCartTotal();
    void updateCartIcon();
    QWidget* createCartItemWidget(const CartItem &item, int bookId);

    void setupSidebarAnimation();
    void toggleSidebar(bool expand);
    void setupSearchCompleter();
    void setupAutoBanner();
    void updateBannerImages();
    void setupFilterPanel();
    void loadAndDisplayFilteredBooks();
    void loadAndDisplayAuthors();
    void loadCartFromDatabase();

    Ui::MainWindow *ui;
    DatabaseManager *m_dbManager;
    int m_currentCustomerId;

    QPropertyAnimation *m_sidebarAnimation = nullptr;
    bool m_isSidebarExpanded = false;
    int m_collapsedWidth = 50;
    int m_expandedWidth = 200;
    QMap<QPushButton*, QString> m_buttonOriginalText;

    QCompleter *m_searchCompleter = nullptr;
    QStandardItemModel *m_searchSuggestionModel = nullptr;
    SearchSuggestionDelegate *m_searchDelegate = nullptr;

    QMap<int, CartItem> m_cartItems;
    QMap<int, QLabel*> m_cartSubtotalLabels;

    int m_currentBookDetailsId = -1;
    int m_currentAuthorDetailsId = -1;

    QTimer *m_bannerTimer = nullptr;
    QStringList m_bannerImagePaths;
    int m_currentBannerIndex = 0;
    QList<QRadioButton*> m_bannerIndicators;

    QPropertyAnimation *m_filterPanelAnimation = nullptr;
    bool m_isFilterPanelVisible = false;
    int m_filterPanelWidth = 250;
    BookFilterCriteria m_currentFilterCriteria;

    QListWidget *m_genreFilterListWidget = nullptr;
    QListWidget *m_languageFilterListWidget = nullptr;
    RangeSlider *m_priceRangeSlider = nullptr;
    QLabel *m_minPriceValueLabel = nullptr;
    QLabel *m_maxPriceValueLabel = nullptr;
    QCheckBox *m_inStockFilterCheckBox = nullptr;

    QTimer *m_filterApplyTimer = nullptr;

    QFrame *m_orderDetailsPanel = nullptr;
    QPropertyAnimation *m_orderDetailsAnimation = nullptr;
    bool m_isOrderDetailsPanelVisible = false;
    int m_orderDetailsPanelWidth = 350;
    QLabel *m_orderDetailsIdLabel = nullptr;
    QLabel *m_orderDetailsDateLabel = nullptr;
    QLabel *m_orderDetailsTotalLabel = nullptr;
    QLabel *m_orderDetailsShippingLabel = nullptr;
    QLabel *m_orderDetailsPaymentLabel = nullptr;
    QVBoxLayout *m_orderDetailsItemsLayout = nullptr;
    QVBoxLayout *m_orderDetailsStatusLayout = nullptr;
    QPushButton *m_closeOrderDetailsButton = nullptr;

    QLabel *m_cartBadgeLabel = nullptr;


protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

};
#endif // MAINWINDOW_H
