/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.8.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDateEdit>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QVBoxLayout *verticalLayout;
    QWidget *headerWidget;
    QHBoxLayout *headerLayout;
    QLabel *logoLabel;
    QLineEdit *globalSearchLineEdit;
    QSpacerItem *horizontalSpacer;
    QPushButton *wishlistButton;
    QPushButton *cartButton;
    QPushButton *profileButton;
    QTabWidget *mainTabWidget;
    QWidget *discoverTab;
    QVBoxLayout *discoverTabLayout;
    QScrollArea *discoverScrollArea;
    QWidget *discoverScrollContents;
    QVBoxLayout *discoverContentLayout;
    QLabel *bannerLabel;
    QLabel *categoryHeaderLabel;
    QWidget *categoriesWidget;
    QGridLayout *categoriesLayout;
    QPushButton *fictionCategoryButton;
    QPushButton *nonFictionCategoryButton;
    QPushButton *childrenCategoryButton;
    QPushButton *educationCategoryButton;
    QLabel *bestsellerHeaderLabel;
    QScrollArea *bestsellerScrollArea;
    QWidget *bestsellerRowWidget;
    QHBoxLayout *bestsellerRowLayout;
    QSpacerItem *bestsellerSpacer;
    QLabel *newReleaseHeaderLabel;
    QScrollArea *newReleaseScrollArea;
    QWidget *newReleaseRowWidget;
    QHBoxLayout *newReleaseRowLayout;
    QSpacerItem *newReleaseSpacer;
    QSpacerItem *discoverBottomSpacer;
    QWidget *booksTab;
    QVBoxLayout *booksTabLayout;
    QWidget *booksFilterWidget;
    QHBoxLayout *booksFilterLayout;
    QLineEdit *bookSearchLineEdit;
    QComboBox *categoryFilterComboBox;
    QPushButton *filterButton;
    QPushButton *sortButton;
    QSpacerItem *horizontalSpacer_2;
    QPushButton *viewModeButton;
    QScrollArea *booksScrollArea;
    QWidget *booksContainerWidget;
    QGridLayout *booksContainerLayout;
    QWidget *authorsTab;
    QVBoxLayout *authorsTabLayout;
    QWidget *authorsFilterWidget;
    QHBoxLayout *authorsFilterLayout;
    QLineEdit *authorSearchLineEdit;
    QSpacerItem *authorSearchSpacer;
    QScrollArea *authorsScrollArea;
    QWidget *authorsContainerWidget;
    QGridLayout *authorsContainerLayout;
    QWidget *ordersTab;
    QVBoxLayout *ordersTabLayout;
    QWidget *ordersFilterWidget;
    QHBoxLayout *ordersFilterLayout;
    QComboBox *orderStatusComboBox;
    QLabel *dateFilterLabel;
    QDateEdit *orderDateEdit;
    QSpacerItem *horizontalSpacer_3;
    QScrollArea *ordersScrollArea;
    QWidget *ordersContainerWidget;
    QVBoxLayout *ordersListLayout;
    QSpacerItem *ordersBottomSpacer;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(1200, 800);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        verticalLayout = new QVBoxLayout(centralwidget);
        verticalLayout->setSpacing(15);
        verticalLayout->setObjectName("verticalLayout");
        verticalLayout->setContentsMargins(20, 15, 20, 15);
        headerWidget = new QWidget(centralwidget);
        headerWidget->setObjectName("headerWidget");
        headerWidget->setMinimumSize(QSize(0, 50));
        headerLayout = new QHBoxLayout(headerWidget);
        headerLayout->setSpacing(12);
        headerLayout->setObjectName("headerLayout");
        headerLayout->setContentsMargins(0, 0, 0, 0);
        logoLabel = new QLabel(headerWidget);
        logoLabel->setObjectName("logoLabel");
        logoLabel->setMinimumSize(QSize(140, 40));
        logoLabel->setMaximumSize(QSize(16777215, 40));
        logoLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        headerLayout->addWidget(logoLabel);

        globalSearchLineEdit = new QLineEdit(headerWidget);
        globalSearchLineEdit->setObjectName("globalSearchLineEdit");
        globalSearchLineEdit->setMinimumSize(QSize(350, 40));
        globalSearchLineEdit->setMaximumSize(QSize(500, 40));

        headerLayout->addWidget(globalSearchLineEdit);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        headerLayout->addItem(horizontalSpacer);

        wishlistButton = new QPushButton(headerWidget);
        wishlistButton->setObjectName("wishlistButton");
        QIcon icon;
        icon.addFile(QString::fromUtf8("icons/heart.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::On);
        wishlistButton->setIcon(icon);
        wishlistButton->setIconSize(QSize(20, 20));
        wishlistButton->setFlat(true);

        headerLayout->addWidget(wishlistButton);

        cartButton = new QPushButton(headerWidget);
        cartButton->setObjectName("cartButton");
        QIcon icon1;
        icon1.addFile(QString::fromUtf8("icons/cart.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::On);
        cartButton->setIcon(icon1);
        cartButton->setIconSize(QSize(20, 20));
        cartButton->setFlat(true);

        headerLayout->addWidget(cartButton);

        profileButton = new QPushButton(headerWidget);
        profileButton->setObjectName("profileButton");
        QIcon icon2;
        icon2.addFile(QString::fromUtf8("icons/user.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::On);
        profileButton->setIcon(icon2);
        profileButton->setIconSize(QSize(20, 20));
        profileButton->setFlat(true);

        headerLayout->addWidget(profileButton);


        verticalLayout->addWidget(headerWidget);

        mainTabWidget = new QTabWidget(centralwidget);
        mainTabWidget->setObjectName("mainTabWidget");
        mainTabWidget->setDocumentMode(true);
        mainTabWidget->setTabsClosable(false);
        discoverTab = new QWidget();
        discoverTab->setObjectName("discoverTab");
        discoverTabLayout = new QVBoxLayout(discoverTab);
        discoverTabLayout->setSpacing(0);
        discoverTabLayout->setObjectName("discoverTabLayout");
        discoverTabLayout->setContentsMargins(0, 0, 0, 0);
        discoverScrollArea = new QScrollArea(discoverTab);
        discoverScrollArea->setObjectName("discoverScrollArea");
        discoverScrollArea->setWidgetResizable(true);
        discoverScrollContents = new QWidget();
        discoverScrollContents->setObjectName("discoverScrollContents");
        discoverScrollContents->setGeometry(QRect(0, 0, 1118, 640));
        discoverContentLayout = new QVBoxLayout(discoverScrollContents);
        discoverContentLayout->setSpacing(20);
        discoverContentLayout->setObjectName("discoverContentLayout");
        discoverContentLayout->setContentsMargins(10, 10, 10, 20);
        bannerLabel = new QLabel(discoverScrollContents);
        bannerLabel->setObjectName("bannerLabel");
        bannerLabel->setMinimumSize(QSize(0, 250));
        bannerLabel->setAlignment(Qt::AlignCenter);
        bannerLabel->setWordWrap(true);

        discoverContentLayout->addWidget(bannerLabel);

        categoryHeaderLabel = new QLabel(discoverScrollContents);
        categoryHeaderLabel->setObjectName("categoryHeaderLabel");

        discoverContentLayout->addWidget(categoryHeaderLabel);

        categoriesWidget = new QWidget(discoverScrollContents);
        categoriesWidget->setObjectName("categoriesWidget");
        categoriesLayout = new QGridLayout(categoriesWidget);
        categoriesLayout->setSpacing(20);
        categoriesLayout->setObjectName("categoriesLayout");
        categoriesLayout->setContentsMargins(0, 5, 0, 0);
        fictionCategoryButton = new QPushButton(categoriesWidget);
        fictionCategoryButton->setObjectName("fictionCategoryButton");
        QSizePolicy sizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(fictionCategoryButton->sizePolicy().hasHeightForWidth());
        fictionCategoryButton->setSizePolicy(sizePolicy);
        fictionCategoryButton->setMinimumSize(QSize(150, 100));

        categoriesLayout->addWidget(fictionCategoryButton, 0, 0, 1, 1);

        nonFictionCategoryButton = new QPushButton(categoriesWidget);
        nonFictionCategoryButton->setObjectName("nonFictionCategoryButton");
        sizePolicy.setHeightForWidth(nonFictionCategoryButton->sizePolicy().hasHeightForWidth());
        nonFictionCategoryButton->setSizePolicy(sizePolicy);
        nonFictionCategoryButton->setMinimumSize(QSize(150, 100));

        categoriesLayout->addWidget(nonFictionCategoryButton, 0, 1, 1, 1);

        childrenCategoryButton = new QPushButton(categoriesWidget);
        childrenCategoryButton->setObjectName("childrenCategoryButton");
        sizePolicy.setHeightForWidth(childrenCategoryButton->sizePolicy().hasHeightForWidth());
        childrenCategoryButton->setSizePolicy(sizePolicy);
        childrenCategoryButton->setMinimumSize(QSize(150, 100));

        categoriesLayout->addWidget(childrenCategoryButton, 0, 2, 1, 1);

        educationCategoryButton = new QPushButton(categoriesWidget);
        educationCategoryButton->setObjectName("educationCategoryButton");
        sizePolicy.setHeightForWidth(educationCategoryButton->sizePolicy().hasHeightForWidth());
        educationCategoryButton->setSizePolicy(sizePolicy);
        educationCategoryButton->setMinimumSize(QSize(150, 100));

        categoriesLayout->addWidget(educationCategoryButton, 0, 3, 1, 1);


        discoverContentLayout->addWidget(categoriesWidget);

        bestsellerHeaderLabel = new QLabel(discoverScrollContents);
        bestsellerHeaderLabel->setObjectName("bestsellerHeaderLabel");

        discoverContentLayout->addWidget(bestsellerHeaderLabel);

        bestsellerScrollArea = new QScrollArea(discoverScrollContents);
        bestsellerScrollArea->setObjectName("bestsellerScrollArea");
        bestsellerScrollArea->setMinimumHeight(280);
        bestsellerScrollArea->setMaximumHeight(320);
        bestsellerScrollArea->setFrameShape(QFrame::NoFrame);
        bestsellerScrollArea->setWidgetResizable(true);
        bestsellerScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        bestsellerScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        bestsellerRowWidget = new QWidget();
        bestsellerRowWidget->setObjectName("bestsellerRowWidget");
        bestsellerRowWidget->setGeometry(QRect(0, 0, 1098, 300));
        bestsellerRowLayout = new QHBoxLayout(bestsellerRowWidget);
        bestsellerRowLayout->setSpacing(20);
        bestsellerRowLayout->setObjectName("bestsellerRowLayout");
        bestsellerRowLayout->setContentsMargins(0, 5, 0, 5);
        bestsellerSpacer = new QSpacerItem(0, 0, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        bestsellerRowLayout->addItem(bestsellerSpacer);

        bestsellerScrollArea->setWidget(bestsellerRowWidget);

        discoverContentLayout->addWidget(bestsellerScrollArea);

        newReleaseHeaderLabel = new QLabel(discoverScrollContents);
        newReleaseHeaderLabel->setObjectName("newReleaseHeaderLabel");

        discoverContentLayout->addWidget(newReleaseHeaderLabel);

        newReleaseScrollArea = new QScrollArea(discoverScrollContents);
        newReleaseScrollArea->setObjectName("newReleaseScrollArea");
        newReleaseScrollArea->setMinimumHeight(280);
        newReleaseScrollArea->setMaximumHeight(320);
        newReleaseScrollArea->setFrameShape(QFrame::NoFrame);
        newReleaseScrollArea->setWidgetResizable(true);
        newReleaseScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        newReleaseScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        newReleaseRowWidget = new QWidget();
        newReleaseRowWidget->setObjectName("newReleaseRowWidget");
        newReleaseRowWidget->setGeometry(QRect(0, 0, 1098, 300));
        newReleaseRowLayout = new QHBoxLayout(newReleaseRowWidget);
        newReleaseRowLayout->setSpacing(20);
        newReleaseRowLayout->setObjectName("newReleaseRowLayout");
        newReleaseRowLayout->setContentsMargins(0, 5, 0, 5);
        newReleaseSpacer = new QSpacerItem(0, 0, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        newReleaseRowLayout->addItem(newReleaseSpacer);

        newReleaseScrollArea->setWidget(newReleaseRowWidget);

        discoverContentLayout->addWidget(newReleaseScrollArea);

        discoverBottomSpacer = new QSpacerItem(20, 0, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        discoverContentLayout->addItem(discoverBottomSpacer);

        discoverScrollArea->setWidget(discoverScrollContents);

        discoverTabLayout->addWidget(discoverScrollArea);

        QIcon icon3;
        icon3.addFile(QString::fromUtf8("icons/home.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::On);
        mainTabWidget->addTab(discoverTab, icon3, QString());
        booksTab = new QWidget();
        booksTab->setObjectName("booksTab");
        booksTabLayout = new QVBoxLayout(booksTab);
        booksTabLayout->setSpacing(16);
        booksTabLayout->setObjectName("booksTabLayout");
        booksTabLayout->setContentsMargins(0, 10, 0, 0);
        booksFilterWidget = new QWidget(booksTab);
        booksFilterWidget->setObjectName("booksFilterWidget");
        booksFilterWidget->setMinimumSize(QSize(0, 45));
        booksFilterLayout = new QHBoxLayout(booksFilterWidget);
        booksFilterLayout->setSpacing(16);
        booksFilterLayout->setObjectName("booksFilterLayout");
        booksFilterLayout->setContentsMargins(0, 0, 0, 5);
        bookSearchLineEdit = new QLineEdit(booksFilterWidget);
        bookSearchLineEdit->setObjectName("bookSearchLineEdit");
        bookSearchLineEdit->setMinimumSize(QSize(300, 40));
        bookSearchLineEdit->setMaximumSize(QSize(400, 40));

        booksFilterLayout->addWidget(bookSearchLineEdit);

        categoryFilterComboBox = new QComboBox(booksFilterWidget);
        categoryFilterComboBox->addItem(QString());
        categoryFilterComboBox->setObjectName("categoryFilterComboBox");
        categoryFilterComboBox->setMinimumSize(QSize(200, 40));

        booksFilterLayout->addWidget(categoryFilterComboBox);

        filterButton = new QPushButton(booksFilterWidget);
        filterButton->setObjectName("filterButton");
        filterButton->setMinimumSize(QSize(40, 40));
        filterButton->setMaximumSize(QSize(40, 40));
        QIcon icon4;
        icon4.addFile(QString::fromUtf8("icons/filter.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::On);
        filterButton->setIcon(icon4);
        filterButton->setIconSize(QSize(18, 18));

        booksFilterLayout->addWidget(filterButton);

        sortButton = new QPushButton(booksFilterWidget);
        sortButton->setObjectName("sortButton");
        sortButton->setMinimumSize(QSize(40, 40));
        sortButton->setMaximumSize(QSize(40, 40));
        QIcon icon5;
        icon5.addFile(QString::fromUtf8("icons/sort-amount-down.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::On);
        sortButton->setIcon(icon5);
        sortButton->setIconSize(QSize(18, 18));

        booksFilterLayout->addWidget(sortButton);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        booksFilterLayout->addItem(horizontalSpacer_2);

        viewModeButton = new QPushButton(booksFilterWidget);
        viewModeButton->setObjectName("viewModeButton");
        viewModeButton->setMinimumSize(QSize(40, 40));
        viewModeButton->setMaximumSize(QSize(40, 40));
        QIcon icon6;
        icon6.addFile(QString::fromUtf8("icons/grid.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::On);
        viewModeButton->setIcon(icon6);
        viewModeButton->setIconSize(QSize(18, 18));
        viewModeButton->setCheckable(true);

        booksFilterLayout->addWidget(viewModeButton);


        booksTabLayout->addWidget(booksFilterWidget);

        booksScrollArea = new QScrollArea(booksTab);
        booksScrollArea->setObjectName("booksScrollArea");
        booksScrollArea->setWidgetResizable(true);
        booksContainerWidget = new QWidget();
        booksContainerWidget->setObjectName("booksContainerWidget");
        booksContainerWidget->setGeometry(QRect(0, 0, 1118, 569));
        booksContainerLayout = new QGridLayout(booksContainerWidget);
        booksContainerLayout->setSpacing(24);
        booksContainerLayout->setObjectName("booksContainerLayout");
        booksContainerLayout->setContentsMargins(5, 10, 5, 10);
        booksScrollArea->setWidget(booksContainerWidget);

        booksTabLayout->addWidget(booksScrollArea);

        QIcon icon7;
        icon7.addFile(QString::fromUtf8("icons/book.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::On);
        mainTabWidget->addTab(booksTab, icon7, QString());
        authorsTab = new QWidget();
        authorsTab->setObjectName("authorsTab");
        authorsTabLayout = new QVBoxLayout(authorsTab);
        authorsTabLayout->setSpacing(16);
        authorsTabLayout->setObjectName("authorsTabLayout");
        authorsTabLayout->setContentsMargins(0, 10, 0, 0);
        authorsFilterWidget = new QWidget(authorsTab);
        authorsFilterWidget->setObjectName("authorsFilterWidget");
        authorsFilterWidget->setMinimumSize(QSize(0, 45));
        authorsFilterLayout = new QHBoxLayout(authorsFilterWidget);
        authorsFilterLayout->setSpacing(12);
        authorsFilterLayout->setObjectName("authorsFilterLayout");
        authorsFilterLayout->setContentsMargins(0, 0, 0, 5);
        authorSearchLineEdit = new QLineEdit(authorsFilterWidget);
        authorSearchLineEdit->setObjectName("authorSearchLineEdit");
        authorSearchLineEdit->setMinimumSize(QSize(300, 40));
        authorSearchLineEdit->setMaximumSize(QSize(400, 40));

        authorsFilterLayout->addWidget(authorSearchLineEdit);

        authorSearchSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        authorsFilterLayout->addItem(authorSearchSpacer);


        authorsTabLayout->addWidget(authorsFilterWidget);

        authorsScrollArea = new QScrollArea(authorsTab);
        authorsScrollArea->setObjectName("authorsScrollArea");
        authorsScrollArea->setWidgetResizable(true);
        authorsContainerWidget = new QWidget();
        authorsContainerWidget->setObjectName("authorsContainerWidget");
        authorsContainerWidget->setGeometry(QRect(0, 0, 1118, 569));
        authorsContainerLayout = new QGridLayout(authorsContainerWidget);
        authorsContainerLayout->setSpacing(24);
        authorsContainerLayout->setObjectName("authorsContainerLayout");
        authorsContainerLayout->setContentsMargins(5, 10, 5, 10);
        authorsScrollArea->setWidget(authorsContainerWidget);

        authorsTabLayout->addWidget(authorsScrollArea);

        QIcon icon8;
        icon8.addFile(QString::fromUtf8("icons/users.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::On);
        mainTabWidget->addTab(authorsTab, icon8, QString());
        ordersTab = new QWidget();
        ordersTab->setObjectName("ordersTab");
        ordersTabLayout = new QVBoxLayout(ordersTab);
        ordersTabLayout->setSpacing(16);
        ordersTabLayout->setObjectName("ordersTabLayout");
        ordersTabLayout->setContentsMargins(0, 10, 0, 0);
        ordersFilterWidget = new QWidget(ordersTab);
        ordersFilterWidget->setObjectName("ordersFilterWidget");
        ordersFilterWidget->setMinimumSize(QSize(0, 45));
        ordersFilterLayout = new QHBoxLayout(ordersFilterWidget);
        ordersFilterLayout->setSpacing(16);
        ordersFilterLayout->setObjectName("ordersFilterLayout");
        ordersFilterLayout->setContentsMargins(0, 0, 0, 5);
        orderStatusComboBox = new QComboBox(ordersFilterWidget);
        orderStatusComboBox->addItem(QString());
        orderStatusComboBox->addItem(QString());
        orderStatusComboBox->addItem(QString());
        orderStatusComboBox->addItem(QString());
        orderStatusComboBox->addItem(QString());
        orderStatusComboBox->addItem(QString());
        orderStatusComboBox->setObjectName("orderStatusComboBox");
        orderStatusComboBox->setMinimumSize(QSize(200, 40));

        ordersFilterLayout->addWidget(orderStatusComboBox);

        dateFilterLabel = new QLabel(ordersFilterWidget);
        dateFilterLabel->setObjectName("dateFilterLabel");
        dateFilterLabel->setMinimumSize(QSize(60, 0));
        dateFilterLabel->setAlignment(Qt::AlignRight|Qt::AlignVCenter);

        ordersFilterLayout->addWidget(dateFilterLabel);

        orderDateEdit = new QDateEdit(ordersFilterWidget);
        orderDateEdit->setObjectName("orderDateEdit");
        orderDateEdit->setMinimumSize(QSize(150, 40));
        orderDateEdit->setCalendarPopup(true);

        ordersFilterLayout->addWidget(orderDateEdit);

        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        ordersFilterLayout->addItem(horizontalSpacer_3);


        ordersTabLayout->addWidget(ordersFilterWidget);

        ordersScrollArea = new QScrollArea(ordersTab);
        ordersScrollArea->setObjectName("ordersScrollArea");
        ordersScrollArea->setWidgetResizable(true);
        ordersContainerWidget = new QWidget();
        ordersContainerWidget->setObjectName("ordersContainerWidget");
        ordersContainerWidget->setGeometry(QRect(0, 0, 1118, 569));
        ordersListLayout = new QVBoxLayout(ordersContainerWidget);
        ordersListLayout->setSpacing(16);
        ordersListLayout->setObjectName("ordersListLayout");
        ordersListLayout->setContentsMargins(5, 10, 5, 10);
        ordersBottomSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        ordersListLayout->addItem(ordersBottomSpacer);

        ordersScrollArea->setWidget(ordersContainerWidget);

        ordersTabLayout->addWidget(ordersScrollArea);

        QIcon icon9;
        icon9.addFile(QString::fromUtf8("icons/shopping-bag.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::On);
        mainTabWidget->addTab(ordersTab, icon9, QString());

        verticalLayout->addWidget(mainTabWidget);

        MainWindow->setCentralWidget(centralwidget);
        statusBar = new QStatusBar(MainWindow);
        statusBar->setObjectName("statusBar");
        MainWindow->setStatusBar(statusBar);

        retranslateUi(MainWindow);

        mainTabWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "\320\232\320\275\320\270\320\263\320\260\321\200\320\275\321\217", nullptr));
        MainWindow->setStyleSheet(QCoreApplication::translate("MainWindow", "\n"
"    QMainWindow {\n"
"      background-color: #f8f9fa; /* Light gray background */\n"
"      color: #212529; /* Dark text */\n"
"    }\n"
"\n"
"    QWidget {\n"
"      font-family: \"Segoe UI\", Arial, sans-serif;\n"
"      font-size: 10pt;\n"
"    }\n"
"\n"
"    /* Styling for the main content area of the tabs */\n"
"    QTabWidget::pane {\n"
"      border: none; /* Remove border around the content */\n"
"      background-color: #ffffff; /* White background for tab content */\n"
"      border-radius: 0 0 8px 8px; /* Round bottom corners */\n"
"      padding: 20px; /* Padding inside the tab content */\n"
"    }\n"
"    /* Styling for individual tabs */\n"
"    QTabBar::tab {\n"
"      padding: 14px 24px; /* Generous padding */\n"
"      margin-right: 2px; /* Small space between tabs */\n"
"      background-color: #e9ecef; /* Light gray inactive tab */\n"
"      border: 1px solid #dee2e6; /* Subtle border */\n"
"      border-bottom: none; /* Remove bottom border for inactive */\n"
"      border-top-left-ra"
                        "dius: 8px; /* Round top-left corner */\n"
"      border-top-right-radius: 8px; /* Round top-right corner */\n"
"      color: #495057; /* Medium gray text for inactive */\n"
"      font-weight: 500; /* Medium font weight */\n"
"    }\n"
"    /* Styling for the selected tab */\n"
"    QTabBar::tab:selected {\n"
"      background-color: #ffffff; /* White background for active tab */\n"
"      color: #000000; /* Black text for active tab */\n"
"      font-weight: 600; /* Bolder font weight */\n"
"      border-color: #dee2e6; /* Match pane border */\n"
"    }\n"
"    /* Styling for hovered tabs (not selected) */\n"
"    QTabBar::tab:hover:!selected {\n"
"        background-color: #f1f3f5; /* Slightly lighter gray on hover */\n"
"        color: #343a40; /* Darker text on hover */\n"
"    }\n"
"    /* Align tabs to the left */\n"
"    QTabWidget::tab-bar {\n"
"        alignment: left;\n"
"        left: 10px; /* Indent the tab bar slightly */\n"
"    }\n"
"\n"
"    /* Styling for input fields */\n"
"    QLineEdit, QCo"
                        "mboBox, QDateEdit {\n"
"      padding: 10px 12px;\n"
"      border: 1px solid #ced4da; /* Standard border */\n"
"      border-radius: 6px; /* Slightly rounded corners */\n"
"      background-color: #ffffff; /* White background */\n"
"      color: #212529; /* Dark text */\n"
"      min-height: 38px; /* Consistent height */\n"
"    }\n"
"    QLineEdit:focus, QComboBox:focus, QDateEdit:focus {\n"
"      border-color: #80bdff; /* Blue border on focus (Bootstrap-like) */\n"
"    }\n"
"    QLineEdit::placeholder {\n"
"      color: #6c757d; /* Gray placeholder text */\n"
"    }\n"
"    /* Styling for ComboBox dropdown arrow area */\n"
"    QComboBox::drop-down {\n"
"        subcontrol-origin: padding;\n"
"        subcontrol-position: top right;\n"
"        width: 20px;\n"
"        border-left-width: 1px;\n"
"        border-left-color: #ced4da;\n"
"        border-left-style: solid;\n"
"        border-top-right-radius: 6px; /* Match parent radius */\n"
"        border-bottom-right-radius: 6px; /* Match parent radius */"
                        "\n"
"    }\n"
"    /* Arrow icon for ComboBox */\n"
"    QComboBox::down-arrow {\n"
"        /* Assuming you have an icon file icons/chevron-down.svg */\n"
"        image: url(icons/chevron-down.svg);\n"
"        width: 12px;\n"
"        height: 12px;\n"
"    }\n"
"\n"
"    /* General Button Styling */\n"
"    QPushButton {\n"
"      padding: 10px 18px;\n"
"      background-color: #007bff; /* Primary blue */\n"
"      color: white;\n"
"      border: none;\n"
"      border-radius: 6px; /* Rounded corners */\n"
"      font-weight: 500; /* Medium weight */\n"
"      min-height: 38px; /* Match input field height */\n"
"    }\n"
"    QPushButton:hover {\n"
"      background-color: #0056b3; /* Darker blue on hover */\n"
"    }\n"
"    QPushButton:pressed {\n"
"      background-color: #004085; /* Even darker blue when pressed */\n"
"    }\n"
"\n"
"    /* Specific styling for header icon buttons */\n"
"    QPushButton#cartButton, QPushButton#wishlistButton, QPushButton#profileButton {\n"
"      background-color: trans"
                        "parent; /* No background */\n"
"      border: none;\n"
"      padding: 8px;\n"
"      border-radius: 18px; /* Circular hover effect */\n"
"      min-height: 36px; min-width: 36px; /* Fixed size */\n"
"      max-width: 36px; max-height: 36px;\n"
"    }\n"
"    QPushButton#cartButton:hover, QPushButton#wishlistButton:hover, QPushButton#profileButton:hover {\n"
"      background-color: #e9ecef; /* Light gray background on hover */\n"
"    }\n"
"    QPushButton#cartButton:pressed, QPushButton#wishlistButton:pressed, QPushButton#profileButton:pressed {\n"
"      background-color: #dee2e6; /* Darker gray when pressed */\n"
"    }\n"
"\n"
"    /* Specific styling for filter/sort icon buttons */\n"
"    QPushButton#filterButton, QPushButton#sortButton, QPushButton#viewModeButton {\n"
"        background-color: #f8f9fa; /* Match main background */\n"
"        border: 1px solid #ced4da; /* Standard border */\n"
"        color: #495057; /* Medium gray icon color (if needed) */\n"
"        min-width: 40px; max-width: 40px"
                        "; /* Fixed size */\n"
"        padding: 8px; /* Padding around icon */\n"
"        border-radius: 6px; /* Match input fields */\n"
"    }\n"
"     QPushButton#filterButton:hover, QPushButton#sortButton:hover, QPushButton#viewModeButton:hover {\n"
"        background-color: #e9ecef; /* Light gray on hover */\n"
"        border-color: #adb5bd; /* Darker border on hover */\n"
"     }\n"
"    QPushButton#filterButton:pressed, QPushButton#sortButton:pressed, QPushButton#viewModeButton:pressed {\n"
"        background-color: #dee2e6; /* Darker gray when pressed */\n"
"     }\n"
"\n"
"    /* Scroll Area Styling */\n"
"    QScrollArea {\n"
"      border: none; /* No border around scroll areas */\n"
"      background-color: transparent; /* Inherit background */\n"
"    }\n"
"    /* Vertical Scroll Bar Track */\n"
"    QScrollBar:vertical {\n"
"        border: none;\n"
"        background: #f1f3f5; /* Light gray track */\n"
"        width: 10px; /* Width of the scrollbar */\n"
"        margin: 0px 0px 0px 0px;\n"
"     "
                        "   border-radius: 5px; /* Rounded track */\n"
"    }\n"
"    /* Vertical Scroll Bar Handle */\n"
"    QScrollBar::handle:vertical {\n"
"        background: #ced4da; /* Medium gray handle */\n"
"        min-height: 20px; /* Minimum handle size */\n"
"        border-radius: 5px; /* Rounded handle */\n"
"    }\n"
"    QScrollBar::handle:vertical:hover {\n"
"        background: #adb5bd; /* Darker handle on hover */\n"
"    }\n"
"    /* Hide scroll bar arrows */\n"
"    QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {\n"
"        border: none;\n"
"        background: none;\n"
"        height: 0px;\n"
"    }\n"
"     /* Hide space below/above handle */\n"
"     QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {\n"
"         background: none;\n"
"     }\n"
"     /* Horizontal Scroll Bar (similar styling, adapt if needed) */\n"
"     QScrollBar:horizontal {\n"
"        border: none;\n"
"        background: #f1f3f5;\n"
"        height: 10px;\n"
"        margin: 0px 0px 0px 0px;\n"
"        bor"
                        "der-radius: 5px;\n"
"     }\n"
"     QScrollBar::handle:horizontal {\n"
"        background: #ced4da;\n"
"        min-width: 20px;\n"
"        border-radius: 5px;\n"
"     }\n"
"     QScrollBar::handle:horizontal:hover {\n"
"        background: #adb5bd;\n"
"     }\n"
"     QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {\n"
"        border: none;\n"
"        background: none;\n"
"        width: 0px;\n"
"     }\n"
"     QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {\n"
"         background: none;\n"
"     }\n"
"\n"
"\n"
"    /* Status Bar Styling */\n"
"    QStatusBar {\n"
"      background-color: #f8f9fa; /* Match main background */\n"
"      color: #6c757d; /* Medium gray text */\n"
"      font-size: 9pt; /* Slightly smaller font */\n"
"      border-top: 1px solid #dee2e6; /* Separator line */\n"
"    }\n"
"\n"
"    /* General Label Styling */\n"
"    QLabel {\n"
"       color: #212529; /* Dark text */\n"
"       background-color: transparent; /* No background unless spec"
                        "ified */\n"
"    }\n"
"\n"
"    /* Specific Label Styling */\n"
"    QLabel#logoLabel {\n"
"       font-size: 20px; /* Larger font for logo */\n"
"       font-weight: bold; /* Bold */\n"
"       color: #000000; /* Black */\n"
"       padding-left: 5px; /* Small padding */\n"
"    }\n"
"\n"
"    /* Styling for the main banner on Discover tab */\n"
"    QLabel#bannerLabel {\n"
"      /* Example gradient background */\n"
"      background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:0, stop:0 rgba(0, 123, 255, 0.8), stop:1 rgba(0, 86, 179, 0.9));\n"
"      border-radius: 12px; /* Rounded corners */\n"
"      color: #ffffff; /* White text */\n"
"      font-size: 28px; /* Large font */\n"
"      font-weight: bold; /* Bold */\n"
"      padding: 40px; /* Large padding */\n"
"      /* text-align: center; Property not available directly, use Alignment property */\n"
"    }\n"
"\n"
"    /* Styling for section headers */\n"
"    QLabel#categoryHeaderLabel, QLabel#bestsellerHeaderLabel, QLabel#newReleaseHeaderL"
                        "abel {\n"
"       font-size: 18px; /* Larger font for headers */\n"
"       font-weight: 600; /* Semi-bold */\n"
"       color: #343a40; /* Dark gray text */\n"
"       margin-top: 24px; /* Space above header */\n"
"       margin-bottom: 8px; /* Space below header */\n"
"       padding-bottom: 4px; /* Space before border */\n"
"       border-bottom: 1px solid #e9ecef; /* Subtle bottom border */\n"
"    }\n"
"\n"
"    /* Styling for category buttons on Discover tab */\n"
"    QWidget#categoriesWidget QPushButton {\n"
"        background-color: #ffffff; /* White background */\n"
"        color: #007bff; /* Blue text */\n"
"        border: 1px solid #007bff; /* Blue border */\n"
"        font-weight: 600; /* Semi-bold */\n"
"        text-align: center; /* Center text */\n"
"        padding: 15px; /* Generous padding */\n"
"        min-height: 100px; /* Fixed height */\n"
"        border-radius: 8px; /* Rounded corners */\n"
"    }\n"
"    QWidget#categoriesWidget QPushButton:hover {\n"
"        background-color: "
                        "#e7f3ff; /* Light blue background on hover */\n"
"        color: #0056b3; /* Darker blue text */\n"
"        border-color: #0056b3; /* Darker blue border */\n"
"    }\n"
"     QWidget#categoriesWidget QPushButton:pressed {\n"
"        background-color: #cce5ff; /* Even lighter blue when pressed */\n"
"     }\n"
"\n"
"   ", nullptr));
        logoLabel->setStyleSheet(QString());
        logoLabel->setText(QCoreApplication::translate("MainWindow", "\320\232\320\275\320\270\320\263\320\260\321\200\320\275\321\217", nullptr));
        globalSearchLineEdit->setPlaceholderText(QCoreApplication::translate("MainWindow", "\320\237\320\276\321\210\321\203\320\272 \320\272\320\275\320\270\320\263, \320\260\320\262\321\202\320\276\321\200\321\226\320\262, \320\262\320\270\320\264\320\260\320\262\320\275\320\270\321\206\321\202\320\262...", nullptr));
#if QT_CONFIG(tooltip)
        wishlistButton->setToolTip(QCoreApplication::translate("MainWindow", "\320\236\320\261\321\200\320\260\320\275\320\265", nullptr));
#endif // QT_CONFIG(tooltip)
        wishlistButton->setText(QString());
#if QT_CONFIG(tooltip)
        cartButton->setToolTip(QCoreApplication::translate("MainWindow", "\320\232\320\276\321\210\320\270\320\272", nullptr));
#endif // QT_CONFIG(tooltip)
        cartButton->setText(QString());
#if QT_CONFIG(tooltip)
        profileButton->setToolTip(QCoreApplication::translate("MainWindow", "\320\234\321\226\320\271 \320\277\321\200\320\276\321\204\321\226\320\273\321\214", nullptr));
#endif // QT_CONFIG(tooltip)
        profileButton->setText(QString());
        bannerLabel->setStyleSheet(QString());
        bannerLabel->setText(QCoreApplication::translate("MainWindow", "\360\237\223\232 \320\235\320\276\320\262\320\270\320\275\320\272\320\270 \321\202\320\260 \320\237\320\276\320\277\321\203\320\273\321\217\321\200\320\275\321\226 \320\232\320\275\320\270\320\263\320\270 \360\237\223\232", nullptr));
        categoryHeaderLabel->setStyleSheet(QString());
        categoryHeaderLabel->setText(QCoreApplication::translate("MainWindow", "\320\237\320\276\320\277\321\203\320\273\321\217\321\200\320\275\321\226 \320\272\320\260\321\202\320\265\320\263\320\276\321\200\321\226\321\227", nullptr));
        fictionCategoryButton->setStyleSheet(QString());
        fictionCategoryButton->setText(QCoreApplication::translate("MainWindow", "\320\245\321\203\320\264\320\276\320\266\320\275\321\217\n"
" \320\273\321\226\321\202\320\265\321\200\320\260\321\202\321\203\321\200\320\260", nullptr));
        nonFictionCategoryButton->setStyleSheet(QString());
        nonFictionCategoryButton->setText(QCoreApplication::translate("MainWindow", "\320\221\321\226\320\267\320\275\320\265\321\201\n"
" \321\202\320\260 \320\275\320\260\321\203\320\272\320\260", nullptr));
        childrenCategoryButton->setStyleSheet(QString());
        childrenCategoryButton->setText(QCoreApplication::translate("MainWindow", "\320\224\320\270\321\202\321\217\321\207\321\226\n"
" \320\272\320\275\320\270\320\263\320\270", nullptr));
        educationCategoryButton->setStyleSheet(QString());
        educationCategoryButton->setText(QCoreApplication::translate("MainWindow", "\320\236\321\201\320\262\321\226\321\202\320\260", nullptr));
        bestsellerHeaderLabel->setStyleSheet(QString());
        bestsellerHeaderLabel->setText(QCoreApplication::translate("MainWindow", "\360\237\224\245 \320\221\320\265\321\201\321\202\321\201\320\265\320\273\320\265\321\200\320\270", nullptr));
        newReleaseHeaderLabel->setStyleSheet(QString());
        newReleaseHeaderLabel->setText(QCoreApplication::translate("MainWindow", "\342\234\250 \320\235\320\276\320\262\320\270\320\275\320\272\320\270", nullptr));
        mainTabWidget->setTabText(mainTabWidget->indexOf(discoverTab), QCoreApplication::translate("MainWindow", "\320\223\320\276\320\273\320\276\320\262\320\275\320\260", nullptr));
        bookSearchLineEdit->setPlaceholderText(QCoreApplication::translate("MainWindow", "\320\237\320\276\321\210\321\203\320\272 \320\267\320\260 \320\275\320\260\320\267\320\262\320\276\321\216 \320\260\320\261\320\276 \320\260\320\262\321\202\320\276\321\200\320\276\320\274...", nullptr));
        categoryFilterComboBox->setItemText(0, QCoreApplication::translate("MainWindow", "\320\222\321\201\321\226 \320\272\320\260\321\202\320\265\320\263\320\276\321\200\321\226\321\227", nullptr));

#if QT_CONFIG(tooltip)
        filterButton->setToolTip(QCoreApplication::translate("MainWindow", "\320\244\321\226\320\273\321\214\321\202\321\200\320\270", nullptr));
#endif // QT_CONFIG(tooltip)
        filterButton->setText(QString());
#if QT_CONFIG(tooltip)
        sortButton->setToolTip(QCoreApplication::translate("MainWindow", "\320\241\320\276\321\200\321\202\321\203\320\262\320\260\320\275\320\275\321\217", nullptr));
#endif // QT_CONFIG(tooltip)
        sortButton->setText(QString());
#if QT_CONFIG(tooltip)
        viewModeButton->setToolTip(QCoreApplication::translate("MainWindow", "\320\222\320\270\320\263\320\273\321\217\320\264: \320\241\321\226\321\202\320\272\320\260/\320\241\320\277\320\270\321\201\320\276\320\272", nullptr));
#endif // QT_CONFIG(tooltip)
        viewModeButton->setText(QString());
        mainTabWidget->setTabText(mainTabWidget->indexOf(booksTab), QCoreApplication::translate("MainWindow", "\320\232\320\275\320\270\320\263\320\270", nullptr));
        authorSearchLineEdit->setPlaceholderText(QCoreApplication::translate("MainWindow", "\320\237\320\276\321\210\321\203\320\272 \320\267\320\260 \321\226\320\274'\321\217\320\274 \320\260\320\262\321\202\320\276\321\200\320\260...", nullptr));
        mainTabWidget->setTabText(mainTabWidget->indexOf(authorsTab), QCoreApplication::translate("MainWindow", "\320\220\320\262\321\202\320\276\321\200\320\270", nullptr));
        orderStatusComboBox->setItemText(0, QCoreApplication::translate("MainWindow", "\320\222\321\201\321\226 \321\201\321\202\320\260\321\202\321\203\321\201\320\270", nullptr));
        orderStatusComboBox->setItemText(1, QCoreApplication::translate("MainWindow", "\320\235\320\276\320\262\320\265", nullptr));
        orderStatusComboBox->setItemText(2, QCoreApplication::translate("MainWindow", "\320\222 \320\276\320\261\321\200\320\276\320\261\321\206\321\226", nullptr));
        orderStatusComboBox->setItemText(3, QCoreApplication::translate("MainWindow", "\320\222\321\226\320\264\320\277\321\200\320\260\320\262\320\273\320\265\320\275\320\276", nullptr));
        orderStatusComboBox->setItemText(4, QCoreApplication::translate("MainWindow", "\320\224\320\276\321\201\321\202\320\260\320\262\320\273\320\265\320\275\320\276", nullptr));
        orderStatusComboBox->setItemText(5, QCoreApplication::translate("MainWindow", "\320\241\320\272\320\260\321\201\320\276\320\262\320\260\320\275\320\276", nullptr));

        dateFilterLabel->setText(QCoreApplication::translate("MainWindow", "\320\224\320\260\321\202\320\260 \320\262\321\226\320\264:", nullptr));
        orderDateEdit->setDisplayFormat(QCoreApplication::translate("MainWindow", "dd.MM.yyyy", nullptr));
#if QT_CONFIG(tooltip)
        orderDateEdit->setToolTip(QCoreApplication::translate("MainWindow", "\320\244\321\226\320\273\321\214\321\202\321\200\321\203\320\262\320\260\321\202\320\270 \320\267\320\260\320\274\320\276\320\262\320\273\320\265\320\275\320\275\321\217 \320\262\321\226\320\264 \320\262\320\272\320\260\320\267\320\260\320\275\320\276\321\227 \320\264\320\260\321\202\320\270", nullptr));
#endif // QT_CONFIG(tooltip)
        mainTabWidget->setTabText(mainTabWidget->indexOf(ordersTab), QCoreApplication::translate("MainWindow", "\320\234\320\276\321\227 \320\267\320\260\320\274\320\276\320\262\320\273\320\265\320\275\320\275\321\217", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
