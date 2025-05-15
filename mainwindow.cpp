#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "database.h"
#include <QStatusBar>
#include <QMessageBox>
#include <QDebug>
#include <QLabel>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QPixmap>
#include <QFrame>
#include <QSizePolicy>
#include <QScrollArea>
#include <QPainter>
#include <QBitmap>
#include <QDate>
#include <QPropertyAnimation>
#include <QEvent>
#include <QEnterEvent>
#include <QMap>
#include <QDateTime>
#include <QLocale>
#include <QComboBox>
#include <QLineEdit>
#include <QCompleter>
#include <QStringListModel>
#include <QListView>
#include <QMouseEvent>
#include <QTextEdit>
#include "starratingwidget.h"
#include <QCoreApplication>
#include <QDir>
#include <QHeaderView>
#include <QSpinBox>
#include <QToolButton>
#include <QMap>
#include <QScrollArea>
#include <QTimer>
#include <QListWidget>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QListWidgetItem>
#include <QParallelAnimationGroup>
#include "RangeSlider.h"
#include <QFrame>
#include <QVBoxLayout>
#include <QGridLayout>

MainWindow::MainWindow(DatabaseManager *dbManager, int customerId, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_dbManager(dbManager)
    , m_currentCustomerId(customerId)
{
    ui->setupUi(this);

    if (ui->cartButton) {
        ui->cartButton->setIcon(QIcon("D:/projects/DB_Kurs/QtAPP/untitled/icons/cart.png"));
        ui->cartButton->setText("");
        qInfo() << "Cart button icon explicitly set in constructor.";
    } else {
        qWarning() << "Constructor: cartButton is null, cannot set icon.";
    }


    if (!m_dbManager) {
        qCritical() << "MainWindow: DatabaseManager is null! Cannot function properly.";
        QMessageBox::critical(this, tr("Критична помилка"), tr("Менеджер бази даних не ініціалізовано."));
        return;
    }

    if (m_currentCustomerId <= 0) {
         qWarning() << "MainWindow: Invalid customer ID received:" << m_currentCustomerId;
         ui->statusBar->showMessage(tr("Помилка: Не вдалося визначити користувача."), 0);
    } else {
         ui->statusBar->showMessage(tr("Вітаємо!"), 5000);
         qInfo() << "MainWindow initialized for customer ID:" << m_currentCustomerId;
    }

    connect(ui->navHomeButton, &QPushButton::clicked, this, &MainWindow::on_navHomeButton_clicked);
    connect(ui->navBooksButton, &QPushButton::clicked, this, &MainWindow::on_navBooksButton_clicked);
    connect(ui->navAuthorsButton, &QPushButton::clicked, this, &MainWindow::on_navAuthorsButton_clicked);
    connect(ui->navOrdersButton, &QPushButton::clicked, this, &MainWindow::on_navOrdersButton_clicked);
    connect(ui->navProfileButton, &QPushButton::clicked, this, &MainWindow::on_navProfileButton_clicked);

    connect(ui->editProfileButton, &QPushButton::clicked, this, &MainWindow::on_editProfileButton_clicked);
    connect(ui->saveProfileButton, &QPushButton::clicked, this, &MainWindow::on_saveProfileButton_clicked);

    connect(ui->cartButton, &QPushButton::clicked, this, &MainWindow::on_cartButton_clicked);

    QWidget* cartButtonParent = ui->cartButton ? ui->cartButton->parentWidget() : nullptr;
    QLayout* parentLayout = cartButtonParent ? cartButtonParent->layout() : nullptr;

    if (ui->cartButton && parentLayout) {
        qInfo() << "Found parent layout for cartButton:" << parentLayout->metaObject()->className();
        QWidget* cartButtonContainer = new QWidget();
        QGridLayout* cartLayout = new QGridLayout(cartButtonContainer);
        cartLayout->setContentsMargins(0, 0, 0, 0);
        cartLayout->setSpacing(0);

        m_cartBadgeLabel = new QLabel(cartButtonContainer);
        m_cartBadgeLabel->setObjectName("cartBadgeLabel");
        m_cartBadgeLabel->setFixedSize(16, 16);
        m_cartBadgeLabel->setStyleSheet(
            "QLabel#cartBadgeLabel {"
            "  background-color: red;"
            "  color: white;"
            "  border-radius: 8px;"
            "  font-weight: bold;"
            "  font-size: 9pt;"
            "  padding: 0px;"
            "  qproperty-alignment: 'AlignCenter';"
            "}"
        );
        m_cartBadgeLabel->setText("0");
        m_cartBadgeLabel->hide();

        cartLayout->addWidget(ui->cartButton, 0, 0, 1, 1);
        cartLayout->addWidget(m_cartBadgeLabel, 0, 0, 1, 1, Qt::AlignTop | Qt::AlignRight);
        m_cartBadgeLabel->setContentsMargins(0, -4, -4, 0);

        int buttonIndex = parentLayout->indexOf(ui->cartButton);
        if (buttonIndex != -1) {
            QLayoutItem *item = parentLayout->takeAt(buttonIndex);

            if (qobject_cast<QBoxLayout*>(parentLayout)) {
                 qobject_cast<QBoxLayout*>(parentLayout)->insertWidget(buttonIndex, cartButtonContainer);
                 qInfo() << "Replaced cartButton with cartButtonContainer in parent layout (QBoxLayout).";
            } else if (qobject_cast<QGridLayout*>(parentLayout)) {
                 qWarning() << "Parent layout is QGridLayout. Adding container to the end instead of replacing.";
                 parentLayout->addWidget(cartButtonContainer);
            } else {
                 qWarning() << "Parent layout type (" << parentLayout->metaObject()->className() << ") not explicitly handled for replacement. Adding container to the end.";
                 parentLayout->addWidget(cartButtonContainer);
            }
        } else {
            qWarning() << "Could not find cartButton in its parent layout to replace it. Adding container to the end.";
            parentLayout->addWidget(cartButtonContainer);
        }
    } else {
         qWarning() << "cartButton or its parent layout not found. Cannot create badge container.";
    }

    if (ui->cartPage && ui->placeOrderButton) {
        connect(ui->placeOrderButton, &QPushButton::clicked, this, &MainWindow::on_placeOrderButton_clicked);
    } else {
        qWarning() << "Cart page or place order button not found in UI. Cannot connect signal.";
    }

    m_buttonOriginalText[ui->navHomeButton] = ui->navHomeButton->text();
    m_buttonOriginalText[ui->navBooksButton] = ui->navBooksButton->text();
    m_buttonOriginalText[ui->navAuthorsButton] = ui->navAuthorsButton->text();
    m_buttonOriginalText[ui->navOrdersButton] = ui->navOrdersButton->text();
    m_buttonOriginalText[ui->navProfileButton] = ui->navProfileButton->text();

    setupSidebarAnimation();

    ui->sidebarFrame->installEventFilter(this);

    m_isSidebarExpanded = true;
    ui->sidebarFrame->setMaximumWidth(m_collapsedWidth);
    toggleSidebar(false);

    if (!ui->cartItemsLayout) {
        qCritical() << "cartItemsLayout is null! Cart page might not work correctly.";
        if (ui->cartItemsContainerWidget) {
            QVBoxLayout *layout = new QVBoxLayout(ui->cartItemsContainerWidget);
            layout->setObjectName("cartItemsLayout");
            ui->cartItemsContainerWidget->setLayout(layout);
            qWarning() << "Dynamically created cartItemsLayout.";
        }
    } else {
         QLayoutItem* item = ui->cartItemsLayout->takeAt(0);
         if (item && item->spacerItem()) {
             delete item;
             qInfo() << "Removed initial spacer from cartItemsLayout.";
         } else if (item) {
             ui->cartItemsLayout->insertItem(0, item);
         }
    }


    qInfo() << "Завантаження даних для головної сторінки...";
    if (ui->classicsRowLayout) {
        QList<BookDisplayInfo> classicsBooks = m_dbManager->getBooksByGenre("Класика", 8);
        displayBooksInHorizontalLayout(classicsBooks, ui->classicsRowLayout);
    } else {
        qWarning() << "classicsRowLayout is null!";
    }
    if (ui->fantasyRowLayout) {
        QList<BookDisplayInfo> fantasyBooks = m_dbManager->getBooksByGenre("Фентезі", 8);
        displayBooksInHorizontalLayout(fantasyBooks, ui->fantasyRowLayout);
    } else {
        qWarning() << "fantasyRowLayout is null!";
    }
    if (ui->nonFictionRowLayout) {
        QList<BookDisplayInfo> nonFictionBooks = m_dbManager->getBooksByGenre("Науково-популярне", 8);
        displayBooksInHorizontalLayout(nonFictionBooks, ui->nonFictionRowLayout);
    } else {
        qWarning() << "nonFictionRowLayout is null!";
    }
    qInfo() << "Завершено завантаження даних для головної сторінки.";

    // Disable vertical scrollbars for horizontal book lists on the discover page
    // Attempt to find the QScrollArea containing each horizontal layout
    if (ui->classicsRowLayout) {
        QWidget* parentWidget = ui->classicsRowLayout->parentWidget();
        if (parentWidget) {
            qDebug() << "classicsRowLayout parent widget:" << parentWidget->objectName() << "Type:" << parentWidget->metaObject()->className();
            QScrollArea* classicsScrollArea = qobject_cast<QScrollArea*>(parentWidget->parentWidget());
            if (classicsScrollArea) {
                classicsScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                qInfo() << "Classics row ScrollArea found (" << classicsScrollArea->objectName() << "), vertical scrollbar disabled.";
            } else {
                qWarning() << "Could not find ScrollArea for classicsRowLayout (parent's parent is not ScrollArea). Parent's parent:" << (parentWidget->parentWidget() ? parentWidget->parentWidget()->objectName() : "null") << "Type:" << (parentWidget->parentWidget() ? parentWidget->parentWidget()->metaObject()->className() : "null");
            }
        } else {
            qWarning() << "classicsRowLayout has no parent widget!";
        }
    } else {
        qWarning() << "classicsRowLayout is null, cannot configure scrollbar.";
    }

    if (ui->fantasyRowLayout) {
        QWidget* parentWidget = ui->fantasyRowLayout->parentWidget();
        if (parentWidget) {
            qDebug() << "fantasyRowLayout parent widget:" << parentWidget->objectName() << "Type:" << parentWidget->metaObject()->className();
            QScrollArea* fantasyScrollArea = qobject_cast<QScrollArea*>(parentWidget->parentWidget());
            if (fantasyScrollArea) {
                fantasyScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                qInfo() << "Fantasy row ScrollArea found (" << fantasyScrollArea->objectName() << "), vertical scrollbar disabled.";
            } else {
                qWarning() << "Could not find ScrollArea for fantasyRowLayout (parent's parent is not ScrollArea). Parent's parent:" << (parentWidget->parentWidget() ? parentWidget->parentWidget()->objectName() : "null") << "Type:" << (parentWidget->parentWidget() ? parentWidget->parentWidget()->metaObject()->className() : "null");
            }
        } else {
            qWarning() << "fantasyRowLayout has no parent widget!";
        }
    } else {
        qWarning() << "fantasyRowLayout is null, cannot configure scrollbar.";
    }

    if (ui->nonFictionRowLayout) {
        QWidget* parentWidget = ui->nonFictionRowLayout->parentWidget();
        if (parentWidget) {
            qDebug() << "nonFictionRowLayout parent widget:" << parentWidget->objectName() << "Type:" << parentWidget->metaObject()->className();
            QScrollArea* nonFictionScrollArea = qobject_cast<QScrollArea*>(parentWidget->parentWidget());
            if (nonFictionScrollArea) {
                nonFictionScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                qInfo() << "Non-Fiction row ScrollArea found (" << nonFictionScrollArea->objectName() << "), vertical scrollbar disabled.";
            } else {
                qWarning() << "Could not find ScrollArea for nonFictionRowLayout (parent's parent is not ScrollArea). Parent's parent:" << (parentWidget->parentWidget() ? parentWidget->parentWidget()->objectName() : "null") << "Type:" << (parentWidget->parentWidget() ? parentWidget->parentWidget()->metaObject()->className() : "null");
            }
        } else {
            qWarning() << "nonFictionRowLayout has no parent widget!";
        }
    } else {
        qWarning() << "nonFictionRowLayout is null, cannot configure scrollbar.";
    }


    if (!ui->authorsContainerLayout) {
        qCritical() << "authorsContainerLayout is null!";
    } else {
        QList<AuthorDisplayInfo> authors = m_dbManager->getAllAuthorsForDisplay();
        displayAuthors(authors);
        if (!authors.isEmpty()) {
             qInfo() << "Автори успішно завантажені.";
        } else {
             qWarning() << "Не вдалося завантажити авторів.";
        }
    }

    setProfileEditingEnabled(false);

    setupSearchCompleter();

    setupAutoBanner();

    connect(ui->sendCommentButton, &QPushButton::clicked, this, &MainWindow::on_sendCommentButton_clicked);

    setupFilterPanel();

    m_filterApplyTimer = new QTimer(this);
    m_filterApplyTimer->setSingleShot(true);
    m_filterApplyTimer->setInterval(750);
    connect(m_filterApplyTimer, &QTimer::timeout, this, &MainWindow::applyFiltersWithDelay);


    loadCartFromDatabase();

    QScrollArea* booksScrollArea = ui->booksPage->findChild<QScrollArea*>();
    if (booksScrollArea) {
        booksScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        booksScrollArea->setWidgetResizable(true);
        qInfo() << "Books page ScrollArea horizontal scrollbar disabled.";
    } else {
        qWarning() << "Could not find QScrollArea on the books page!";
    }

    QScrollArea* authorsScrollArea = ui->authorsPage->findChild<QScrollArea*>();
    if (authorsScrollArea) {
        authorsScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        authorsScrollArea->setWidgetResizable(true);
        qInfo() << "Authors page ScrollArea horizontal scrollbar disabled.";
    } else {
        qWarning() << "Could not find QScrollArea on the authors page!";
    }

    m_orderDetailsPanel = ui->orderDetailsPanel;
    if (m_orderDetailsPanel) {
        m_orderDetailsIdLabel = m_orderDetailsPanel->findChild<QLabel*>("orderDetailsIdLabel");
        m_orderDetailsDateLabel = m_orderDetailsPanel->findChild<QLabel*>("orderDetailsDateLabel");
        m_orderDetailsTotalLabel = m_orderDetailsPanel->findChild<QLabel*>("orderDetailsTotalLabel");
        m_orderDetailsShippingLabel = m_orderDetailsPanel->findChild<QLabel*>("orderDetailsShippingLabel");
        m_orderDetailsPaymentLabel = m_orderDetailsPanel->findChild<QLabel*>("orderDetailsPaymentLabel");
        m_orderDetailsItemsLayout = m_orderDetailsPanel->findChild<QVBoxLayout*>("orderDetailsItemsLayout");
        m_orderDetailsStatusLayout = m_orderDetailsPanel->findChild<QVBoxLayout*>("orderDetailsStatusLayout");
        m_closeOrderDetailsButton = m_orderDetailsPanel->findChild<QPushButton*>("closeOrderDetailsButton");

        if (!m_orderDetailsIdLabel || !m_orderDetailsDateLabel || !m_orderDetailsTotalLabel ||
            !m_orderDetailsShippingLabel || !m_orderDetailsPaymentLabel || !m_orderDetailsItemsLayout ||
            !m_orderDetailsStatusLayout || !m_closeOrderDetailsButton)
        {
            qWarning() << "MainWindow Constructor: One or more widgets inside orderDetailsPanel not found!";
            m_orderDetailsPanel->setEnabled(false);
        } else {
             m_orderDetailsAnimation = new QPropertyAnimation(m_orderDetailsPanel, "maximumWidth", this);
             m_orderDetailsAnimation->setDuration(300);
             m_orderDetailsAnimation->setEasingCurve(QEasingCurve::InOutQuad);

             m_orderDetailsPanel->setMaximumWidth(0);
             m_orderDetailsPanel->setVisible(false);
             m_isOrderDetailsPanelVisible = false;

             connect(m_closeOrderDetailsButton, &QPushButton::clicked, this, &MainWindow::hideOrderDetailsPanel);
             qInfo() << "Order details panel initialized successfully.";
        }
    } else {
        qWarning() << "MainWindow Constructor: orderDetailsPanel not found in UI!";
    }

    QWidget* categoriesWidget = this->findChild<QWidget*>("categoriesWidget");
    if (categoriesWidget) {
        QPushButton* fictionButton = categoriesWidget->findChild<QPushButton*>("fictionCategoryButton");
        QPushButton* nonFictionButton = categoriesWidget->findChild<QPushButton*>("nonFictionCategoryButton");
        QPushButton* childrenButton = categoriesWidget->findChild<QPushButton*>("childrenCategoryButton");
        QPushButton* educationButton = categoriesWidget->findChild<QPushButton*>("educationCategoryButton");

        if (fictionButton) {
            connect(fictionButton, &QPushButton::clicked, this, [this](){ applyGenreFilter("Художня література"); });
        } else {
            qWarning() << "fictionCategoryButton not found inside categoriesWidget.";
        }
        if (nonFictionButton) {
            connect(nonFictionButton, &QPushButton::clicked, this, [this](){ applyGenreFilter("Науково-популярне"); });
        } else {
            qWarning() << "nonFictionCategoryButton not found inside categoriesWidget.";
        }
        if (childrenButton) {
            connect(childrenButton, &QPushButton::clicked, this, [this](){ applyGenreFilter("Дитяча література"); });
        } else {
            qWarning() << "childrenCategoryButton not found inside categoriesWidget.";
        }
        if (educationButton) {
            connect(educationButton, &QPushButton::clicked, this, [this](){ applyGenreFilter("Освіта"); });
        } else {
            qWarning() << "educationCategoryButton not found inside categoriesWidget.";
        }
        qInfo() << "Category button signals connected.";
    } else {
         qWarning() << "categoriesWidget not found. Cannot connect category button signals.";
    }

}

MainWindow::~MainWindow()
{
    if (m_dbManager) {
        m_dbManager->closeConnection();
    }
    delete ui;
}

void MainWindow::on_navHomeButton_clicked()
{
    ui->contentStackedWidget->setCurrentWidget(ui->discoverPage);
}

void MainWindow::on_navBooksButton_clicked()
{
    ui->contentStackedWidget->setCurrentWidget(ui->booksPage);
    resetFilters();
    if (ui->filterButton) {
        ui->filterButton->show();
    }
}

void MainWindow::on_navAuthorsButton_clicked()
{
    ui->contentStackedWidget->setCurrentWidget(ui->authorsPage);
    loadAndDisplayAuthors();
    if (ui->filterButton) {
        ui->filterButton->hide();
    }
}

void MainWindow::setupSidebarAnimation()
{
    m_sidebarAnimation = new QPropertyAnimation(ui->sidebarFrame, "maximumWidth", this);
    m_sidebarAnimation->setDuration(400);
    m_sidebarAnimation->setEasingCurve(QEasingCurve::InOutQuad);
}

void MainWindow::toggleSidebar(bool expand)
{
    if (m_isSidebarExpanded == expand && m_sidebarAnimation->state() == QAbstractAnimation::Stopped) {
        return;
    }
    if (m_sidebarAnimation->state() == QAbstractAnimation::Running) {
        m_sidebarAnimation->stop();
    }

    m_isSidebarExpanded = expand;

    for (auto it = m_buttonOriginalText.begin(); it != m_buttonOriginalText.end(); ++it) {
        QPushButton *button = it.key();
        const QString &originalText = it.value();
        if (expand) {
            button->setText(originalText);
            button->setIcon(button->icon());
            button->setToolTip("");
            button->setProperty("collapsed", false);
        } else {
            button->setText("");
            button->setIcon(button->icon());
            button->setToolTip(originalText);
            button->setProperty("collapsed", true);
        }
        button->style()->unpolish(button);
        button->style()->polish(button);
    }

    m_sidebarAnimation->setStartValue(ui->sidebarFrame->width());
    m_sidebarAnimation->setEndValue(expand ? m_expandedWidth : m_collapsedWidth);
    m_sidebarAnimation->start();
}


bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == ui->sidebarFrame) {
        if (event->type() == QEvent::Enter) {
            toggleSidebar(true);
            return true;
        } else if (event->type() == QEvent::Leave) {
            toggleSidebar(false);
            return true;
        }
    }
    if (qobject_cast<QFrame*>(watched) && watched->property("bookId").isValid()) {
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                int bookId = watched->property("bookId").toInt();
                qInfo() << "Book card clicked, bookId:" << bookId;
                showBookDetails(bookId);
                return true;
            }
        }
    }
    else if (qobject_cast<QFrame*>(watched) && watched->property("authorId").isValid()) {
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                int authorId = watched->property("authorId").toInt();
                qInfo() << "Author card clicked, authorId:" << authorId;
                showAuthorDetails(authorId);
                return true;
            }
        }
    }


    return QMainWindow::eventFilter(watched, event);
}


void MainWindow::setupFilterPanel()
{
    if (!ui->filterPanel || !ui->filterButton) {
        qWarning() << "Filter panel or filter button not found in UI. Filtering disabled.";
        return;
    }

    m_genreFilterListWidget = ui->filterPanel->findChild<QListWidget*>("genreFilterListWidget");
    m_languageFilterListWidget = ui->filterPanel->findChild<QListWidget*>("languageFilterListWidget");

    m_priceRangeSlider = nullptr;
    if (ui->priceRangeSlider) {
        RangeSlider *actualSlider = new RangeSlider(Qt::Horizontal, this);
        actualSlider->setObjectName("priceRangeSliderInstance");
        actualSlider->setMinimumHeight(25);
        actualSlider->setVisible(true);

        QWidget* placeholderParent = ui->priceRangeSlider->parentWidget();
        if (!placeholderParent) {
             qWarning() << "Placeholder 'priceRangeSlider' has no parent widget!";
             delete actualSlider;
             m_priceRangeSlider = nullptr;
             delete actualSlider;
        } else {
            QHBoxLayout *priceRangeLayout = ui->filterPanel->findChild<QHBoxLayout*>("priceRangeLayout");

            if (priceRangeLayout) {
                int index = priceRangeLayout->indexOf(ui->priceRangeSlider);
                if (index != -1) {
                    priceRangeLayout->removeWidget(ui->priceRangeSlider);

                    priceRangeLayout->insertWidget(index, actualSlider, 1);

                    m_priceRangeSlider = actualSlider;

                    delete ui->priceRangeSlider;
                    ui->priceRangeSlider = nullptr;

                    qInfo() << "Successfully replaced placeholder with RangeSlider in priceRangeLayout at index" << index;
                } else {
                    qWarning() << "Could not find placeholder 'priceRangeSlider' within 'priceRangeLayout'!";
                    delete actualSlider;
                    m_priceRangeSlider = nullptr;
                }
            } else {
                qWarning() << "Could not find QHBoxLayout named 'priceRangeLayout' inside filterPanel!";
                delete actualSlider;
                m_priceRangeSlider = nullptr;
            }
        }
    } else {
        qWarning() << "Placeholder widget 'priceRangeSlider' not found in UI!";
        m_priceRangeSlider = nullptr;
    }

    m_minPriceValueLabel = ui->filterPanel->findChild<QLabel*>("minPriceValueLabel");
    m_maxPriceValueLabel = ui->filterPanel->findChild<QLabel*>("maxPriceValueLabel");
    m_inStockFilterCheckBox = ui->filterPanel->findChild<QCheckBox*>("inStockFilterCheckBox");
    QPushButton *applyButton = ui->filterPanel->findChild<QPushButton*>("applyFiltersButton");
    QPushButton *resetButton = ui->filterPanel->findChild<QPushButton*>("resetFiltersButton");

    qDebug() << "Checking filter widgets:";
    qDebug() << "  genreFilterListWidget:" << (m_genreFilterListWidget ? "Found" : "NOT FOUND!");
    qDebug() << "  languageFilterListWidget:" << (m_languageFilterListWidget ? "Found" : "NOT FOUND!");
    qDebug() << "  priceRangeSlider (Instance created):" << (m_priceRangeSlider ? "OK" : "FAILED! Check UI placeholder and replacement logic.");
    qDebug() << "  minPriceValueLabel:" << (m_minPriceValueLabel ? "Found" : "NOT FOUND!");
    qDebug() << "  maxPriceValueLabel:" << (m_maxPriceValueLabel ? "Found" : "NOT FOUND!");
    qDebug() << "  inStockFilterCheckBox:" << (m_inStockFilterCheckBox ? "Found" : "NOT FOUND!");
    qDebug() << "  applyFiltersButton:" << (applyButton ? "Found" : "NOT FOUND!");
    qDebug() << "  resetFiltersButton:" << (resetButton ? "Found" : "NOT FOUND!");


    if (!m_genreFilterListWidget || !m_languageFilterListWidget || !m_priceRangeSlider ||
        !m_minPriceValueLabel || !m_maxPriceValueLabel ||
        !m_inStockFilterCheckBox || !applyButton || !resetButton)
    {
        qWarning() << "One or more filter widgets (including price labels) not found inside filterPanel. Filtering might be incomplete.";
        ui->filterButton->setEnabled(false);
        ui->filterButton->setToolTip(tr("Помилка: віджети фільтрації не знайдено."));
        return;
    }

    m_filterPanelAnimation = new QPropertyAnimation(ui->filterPanel, "maximumWidth", this);
    m_filterPanelAnimation->setDuration(300);
    m_filterPanelAnimation->setEasingCurve(QEasingCurve::InOutQuad);

    m_filterPanelWidth = ui->filterPanel->sizeHint().width();
    if (m_filterPanelWidth <= 0) {
        m_filterPanelWidth = 250;
        qWarning() << "Filter panel sizeHint width is 0, using default:" << m_filterPanelWidth;
    }
    qDebug() << "Initialized m_filterPanelWidth to:" << m_filterPanelWidth;


    ui->filterPanel->setMaximumWidth(0);
    m_isFilterPanelVisible = false;

    connect(ui->filterButton, &QPushButton::clicked, this, &MainWindow::on_filterButton_clicked);
    connect(resetButton, &QPushButton::clicked, this, &MainWindow::resetFilters);

    if (applyButton) {
        applyButton->hide();
    }

    if (m_genreFilterListWidget) {
        connect(m_genreFilterListWidget, &QListWidget::itemChanged, this, &MainWindow::onFilterCriteriaChanged);
    }
    if (m_languageFilterListWidget) {
        connect(m_languageFilterListWidget, &QListWidget::itemChanged, this, &MainWindow::onFilterCriteriaChanged);
    }

    if (m_priceRangeSlider) {
        connect(m_priceRangeSlider, &RangeSlider::lowerValueChanged, this, &MainWindow::onFilterCriteriaChanged);
        connect(m_priceRangeSlider, &RangeSlider::upperValueChanged, this, &MainWindow::onFilterCriteriaChanged);
        connect(m_priceRangeSlider, &RangeSlider::lowerValueChanged, this, &MainWindow::updateLowerPriceLabel);
        connect(m_priceRangeSlider, &RangeSlider::upperValueChanged, this, &MainWindow::updateUpperPriceLabel);
    }

    if (m_inStockFilterCheckBox) {
        connect(m_inStockFilterCheckBox, &QCheckBox::stateChanged, this, &MainWindow::onFilterCriteriaChanged);
    }


    if (m_dbManager) {
        QStringList genres = m_dbManager->getAllGenres();
        m_genreFilterListWidget->clear();
        for (const QString &genre : genres) {
            QListWidgetItem *item = new QListWidgetItem(genre, m_genreFilterListWidget);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Unchecked);
        }
        m_genreFilterListWidget->setSelectionMode(QAbstractItemView::MultiSelection);

        QStringList languages = m_dbManager->getAllLanguages();
        m_languageFilterListWidget->clear();
        for (const QString &lang : languages) {
             QListWidgetItem *item = new QListWidgetItem(lang, m_languageFilterListWidget);
             item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
             item->setCheckState(Qt::Unchecked);
        }
        m_languageFilterListWidget->setSelectionMode(QAbstractItemView::MultiSelection);

        const int maxPriceValue = 1000;
        const int minPriceValue = 0;

        if (m_priceRangeSlider) {
            m_priceRangeSlider->setRange(minPriceValue, maxPriceValue);
            m_priceRangeSlider->setLowerValue(minPriceValue);
            m_priceRangeSlider->setUpperValue(maxPriceValue);
            updateLowerPriceLabel(minPriceValue);
            updateUpperPriceLabel(maxPriceValue);
        }

    } else {
        qWarning() << "DatabaseManager is null, cannot populate filter options.";
        ui->filterButton->setEnabled(false);
        ui->filterButton->setToolTip(tr("Помилка: Немає доступу до бази даних для завантаження фільтрів."));
    }

    ui->filterButton->hide();

    QString filterPanelStyle = R"(
        QWidget#filterPanel {
            background-color: #f8f9fa;
            border-left: 1px solid #dee2e6;
            border-radius: 8px;
        }
        QLabel {
            font-weight: bold;
            margin-top: 10px;
            margin-bottom: 5px;
            color: #000000;
        }
        QListWidget {
            border: 1px solid #000000;
            border-radius: 4px;
            background-color: white;
            padding: 5px;
        }
        QListWidget::item {
            padding: 4px 0px;
            color: #000000;
        }
        QListWidget::item:selected {
            background-color: #e9ecef;
            color: #000000;
        }
        QListWidget::indicator:checked {
             image: url(D:/projects/DB_Kurs/QtAPP/untitled/icons//checkbox_checked.png);
        }
        QListWidget::indicator:unchecked {
             image: url(D:/projects/DB_Kurs/QtAPP/untitled/icons//checkbox_unchecked.png);
        }
        QSlider::groove:horizontal {
            border: 1px solid #bbb;
            background: white;
            height: 8px;
            border-radius: 4px;
        }
        QSlider::sub-page:horizontal {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #B1B1B1, stop:1 #c4c4c4);
            background: qlineargradient(x1:0, y1:0.2, x2:1, y2:1, stop:0 #5a6268, stop:1 #6c757d);
            border: 1px solid #4a5258;
            height: 10px;
            border-radius: 4px;
        }
        QSlider::add-page:horizontal {
            background: #e9ecef;
            border: 1px solid #ced4da;
            height: 10px;
            border-radius: 4px;
        }
        QSlider::handle:horizontal {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #f8f9fa, stop:1 #dee2e6);
            border: 1px solid #adb5bd;
            width: 16px;
            margin-top: -4px;
            margin-bottom: -4px;
            border-radius: 8px;
        }
        QSlider::handle:horizontal:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #e9ecef, stop:1 #ced4da);
            border: 1px solid #6c757d;
        }

        QCheckBox {
            spacing: 5px;
            margin-top: 10px;
            color: #000000; /* Ensure text color is black */
        }
        QCheckBox::indicator {
            width: 16px;
            height: 16px;
            border: 1px solid #adb5bd; /* Grey border */
            background-color: white; /* White background */
            border-radius: 3px; /* Optional: slight rounding */
        }
        QCheckBox::indicator:checked {
            border: 1px solid #adb5bd; /* Keep grey border */
            background-color: #adb5bd; /* Grey background when checked */
            color: white; /* Attempt to make the checkmark white */
        }

        QPushButton#resetFiltersButton {
            background-color: #6c757d;
            color: #000000;
            border: none;
            padding: 8px 15px;
            border-radius: 4px;
            margin-top: 15px;
        }
        QPushButton#resetFiltersButton:hover {
            background-color: #5a6268;
        }
        QPushButton#resetFiltersButton:pressed {
            background-color: #545b62;
        }
    )";
    ui->filterPanel->setStyleSheet(filterPanelStyle);

    if(resetButton) resetButton->setObjectName("resetFiltersButton");

}

void MainWindow::on_filterButton_clicked()
{
    qDebug() << "--- Filter button clicked ---";
    qDebug() << "Initial state: m_isFilterPanelVisible =" << m_isFilterPanelVisible << ", Panel visible =" << ui->filterPanel->isVisible() << ", Panel width =" << ui->filterPanel->width();

    if (!ui->filterPanel || !m_filterPanelAnimation) {
        qWarning() << "Filter panel or animation is null! Aborting.";
        return;
    }

    if (m_filterPanelAnimation->state() == QAbstractAnimation::Running) {
        qDebug() << "Animation was running. Stopping it.";
        m_filterPanelAnimation->stop();
        qDebug() << "Panel state after stopping: Visible =" << ui->filterPanel->isVisible() << ", Width =" << ui->filterPanel->width();
    }

    bool targetVisibility = !m_isFilterPanelVisible;

    int startWidth = ui->filterPanel->width();
    int endWidth = targetVisibility ? m_filterPanelWidth : 0;

    qDebug() << "Target state: targetVisibility =" << targetVisibility;
    qDebug() << "Animation details: StartWidth =" << startWidth << ", EndWidth =" << endWidth;

    if (targetVisibility) {
        qDebug() << "Setting panel visible NOW (before show animation).";
        ui->filterPanel->setVisible(true);
    }
    ui->filterPanel->setMinimumWidth(0);

    m_filterPanelAnimation->setStartValue(startWidth);
    m_filterPanelAnimation->setEndValue(endWidth);

    disconnect(m_filterPanelAnimation, &QPropertyAnimation::finished, this, nullptr);
    qDebug() << "Disconnected previous finished signal connections.";

    connect(m_filterPanelAnimation, &QPropertyAnimation::finished, this, [this, targetVisibility]() {
        qDebug() << "--- Animation finished ---";
        qDebug() << "Target state was:" << targetVisibility;
        qDebug() << "Current panel state: Visible =" << ui->filterPanel->isVisible() << ", Width =" << ui->filterPanel->width();
        qDebug() << "Current state variable: m_isFilterPanelVisible =" << m_isFilterPanelVisible;

        if (!targetVisibility) {
             qDebug() << "Setting panel invisible NOW (after hide animation).";
             ui->filterPanel->setVisible(false);
             qDebug() << "Panel state after setting invisible: Visible =" << ui->filterPanel->isVisible() << ", Width =" << ui->filterPanel->width();
        } else {
             qDebug() << "Show animation finished. Panel should remain visible.";
        }

        if (m_isFilterPanelVisible != targetVisibility) {
             qDebug() << "Updating m_isFilterPanelVisible from" << m_isFilterPanelVisible << "to" << targetVisibility;
             m_isFilterPanelVisible = targetVisibility;
        } else {
             qDebug() << "m_isFilterPanelVisible already matches targetVisibility (" << targetVisibility << ")";
        }

        qDebug() << "--- Finished handler complete ---";

    });

    qDebug() << "Starting animation...";
    m_filterPanelAnimation->start();
    qDebug() << "Animation state after start:" << m_filterPanelAnimation->state();

    qDebug() << "--- Filter button click handler complete ---";
}


void MainWindow::applyFilters()
{
    m_currentFilterCriteria = BookFilterCriteria();

    if (m_genreFilterListWidget) {
        for (int i = 0; i < m_genreFilterListWidget->count(); ++i) {
            QListWidgetItem *item = m_genreFilterListWidget->item(i);
            if (item && item->checkState() == Qt::Checked) {
                m_currentFilterCriteria.genres << item->text();
            }
        }
    }

    if (m_languageFilterListWidget) {
        for (int i = 0; i < m_languageFilterListWidget->count(); ++i) {
            QListWidgetItem *item = m_languageFilterListWidget->item(i);
            if (item && item->checkState() == Qt::Checked) {
                m_currentFilterCriteria.languages << item->text();
            }
        }
    }

    if (m_priceRangeSlider) {
        m_currentFilterCriteria.minPrice = static_cast<double>(m_priceRangeSlider->lowerValue());
        if (m_currentFilterCriteria.minPrice == m_priceRangeSlider->minimum()) {
             m_currentFilterCriteria.minPrice = -1.0;
        }

        m_currentFilterCriteria.maxPrice = static_cast<double>(m_priceRangeSlider->upperValue());
        if (m_currentFilterCriteria.maxPrice == m_priceRangeSlider->maximum()) {
             m_currentFilterCriteria.maxPrice = -1.0;
        }
    }

    if (m_inStockFilterCheckBox) {
        m_currentFilterCriteria.inStockOnly = m_inStockFilterCheckBox->isChecked();
    }

    qInfo() << "Applying filters:"
            << "Genres:" << m_currentFilterCriteria.genres
            << "Languages:" << m_currentFilterCriteria.languages
            << "MinPrice:" << m_currentFilterCriteria.minPrice
            << "MaxPrice:" << m_currentFilterCriteria.maxPrice
            << "InStockOnly:" << m_currentFilterCriteria.inStockOnly;

    loadAndDisplayFilteredBooks();

}

void MainWindow::resetFilters()
{
    if (m_genreFilterListWidget) {
        for (int i = 0; i < m_genreFilterListWidget->count(); ++i) {
            if (QListWidgetItem *item = m_genreFilterListWidget->item(i)) {
                item->setCheckState(Qt::Unchecked);
            }
        }
    }
    if (m_languageFilterListWidget) {
         for (int i = 0; i < m_languageFilterListWidget->count(); ++i) {
            if (QListWidgetItem *item = m_languageFilterListWidget->item(i)) {
                item->setCheckState(Qt::Unchecked);
            }
        }
    }
    if (m_priceRangeSlider) {
        int minVal = m_priceRangeSlider->minimum();
        int maxVal = m_priceRangeSlider->maximum();
        m_priceRangeSlider->setLowerValue(minVal);
        m_priceRangeSlider->setUpperValue(maxVal);
        updateLowerPriceLabel(minVal);
        updateUpperPriceLabel(maxVal);
    }

    if (m_inStockFilterCheckBox) {
        m_inStockFilterCheckBox->setChecked(false);
    }

    if (m_filterApplyTimer && m_filterApplyTimer->isActive()) {
        m_filterApplyTimer->stop();
        qDebug() << "Filter timer stopped due to reset.";
    }

    m_currentFilterCriteria = BookFilterCriteria();
    qInfo() << "Filters reset.";

    loadAndDisplayFilteredBooks();

     if (m_isFilterPanelVisible) {
        on_filterButton_clicked();
    }
}

void MainWindow::loadAndDisplayFilteredBooks()
{
    if (!m_dbManager) {
        qWarning() << "Cannot load books: DatabaseManager is null.";
        if (ui->booksContainerLayout) {
            clearLayout(ui->booksContainerLayout);
            QLabel *errorLabel = new QLabel(tr("Помилка: Немає доступу до бази даних."), ui->booksContainerWidget);
            ui->booksContainerLayout->addWidget(errorLabel);
        }
        return;
    }
    if (!ui->booksContainerLayout || !ui->booksContainerWidget) {
        qCritical() << "Cannot display books: booksContainerLayout or booksContainerWidget is null!";
        return;
    }

    qInfo() << "Loading books with current filters...";
    QList<BookDisplayInfo> books = m_dbManager->getFilteredBooksForDisplay(m_currentFilterCriteria);

    displayBooks(books, ui->booksContainerLayout, ui->booksContainerWidget);

    if (!books.isEmpty()) {
         ui->statusBar->showMessage(tr("Книги успішно завантажено (%1 знайдено).").arg(books.size()), 4000);
    } else {
         qInfo() << "No books found matching the current filters.";
         ui->statusBar->showMessage(tr("Книг за вашим запитом не знайдено."), 4000);
    }
}

void MainWindow::onFilterCriteriaChanged()
{
    if (m_filterApplyTimer) {
        m_filterApplyTimer->start();
        qDebug() << "Filter criteria changed, timer (re)started.";
    }
}

void MainWindow::applyFiltersWithDelay()
{
    qDebug() << "Timer timed out, applying filters...";
    applyFilters();
}

void MainWindow::updateLowerPriceLabel(int value)
{
    if (m_minPriceValueLabel) {
        m_minPriceValueLabel->setText(QString::number(value) + " " + tr("грн"));
    }
}

void MainWindow::updateUpperPriceLabel(int value)
{
    if (m_maxPriceValueLabel) {
        m_maxPriceValueLabel->setText(QString::number(value) + " " + tr("грн"));
    }
}


void MainWindow::updateBannerImages()
{
    QList<QLabel*> bannerLabels = {ui->bannerImageLabel_1, ui->bannerImageLabel_2, ui->bannerImageLabel_3};

    if (m_bannerImagePaths.isEmpty()) {
        qWarning() << "Banner image paths are empty. Cannot update images.";
        return;
    }

    for (int i = 0; i < bannerLabels.size(); ++i) {
        if (i < m_bannerImagePaths.size() && bannerLabels[i]) {
            QPixmap bannerPixmap(m_bannerImagePaths[i]);
            if (bannerPixmap.isNull()) {
                qWarning() << "Failed to load banner image:" << m_bannerImagePaths[i];
                bannerLabels[i]->setText(tr("Помилка завантаження банера %1").arg(i + 1));
                bannerLabels[i]->setAlignment(Qt::AlignCenter);
            } else {
                QSize labelSize = bannerLabels[i]->size();
                if (labelSize.isValid() && labelSize.width() > 0 && labelSize.height() > 0) {
                    QPixmap scaledPixmap = bannerPixmap.scaled(labelSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

                    QBitmap mask(scaledPixmap.size());
                    mask.fill(Qt::color0);
                    QPainter painter(&mask);
                    painter.setRenderHint(QPainter::Antialiasing);
                    painter.setBrush(Qt::color1);
                    painter.drawRoundedRect(scaledPixmap.rect(), 18, 18);
                    painter.end();

                    scaledPixmap.setMask(mask);

                    bannerLabels[i]->setPixmap(scaledPixmap);
                } else {
                    bannerLabels[i]->setPixmap(bannerPixmap);
                    qDebug() << "Banner label" << i+1 << "size is invalid during update:" << labelSize << ". Setting original pixmap.";
                }
                bannerLabels[i]->setAlignment(Qt::AlignCenter);
            }
        } else if (bannerLabels[i]) {
             bannerLabels[i]->setText(tr("Банер %1").arg(i + 1));
             bannerLabels[i]->setAlignment(Qt::AlignCenter);
        }
    }
}


void MainWindow::setupAutoBanner()
{
    m_bannerImagePaths.clear();
    m_bannerImagePaths << ":/images/banner1.jpg"
                       << ":/images/banner2.jpg"
                       << ":/images/banner3.jpg";

    const int expectedBannerCount = 3;
    if (m_bannerImagePaths.size() != expectedBannerCount) {
        qWarning() << "Expected" << expectedBannerCount << "banner images, but found" << m_bannerImagePaths.size();
        return;
    }

    m_bannerTimer = new QTimer(this);
    connect(m_bannerTimer, &QTimer::timeout, this, &MainWindow::showNextBanner);
    m_bannerTimer->start(5000);

    m_bannerIndicators << ui->indicatorDot1 << ui->indicatorDot2 << ui->indicatorDot3;
    if (m_bannerIndicators.size() != m_bannerImagePaths.size()) {
        qWarning() << "Mismatch between number of banner images and indicator dots!";
    }

    ui->bannerStackedWidget->setCurrentIndex(m_currentBannerIndex);
    if (!m_bannerIndicators.isEmpty() && m_currentBannerIndex < m_bannerIndicators.size()) {
        m_bannerIndicators[m_currentBannerIndex]->setChecked(true);
    }
}

void MainWindow::showNextBanner()
{
    if (m_bannerImagePaths.isEmpty() || m_bannerIndicators.isEmpty()) return;

    if (m_currentBannerIndex < m_bannerIndicators.size()) {
        m_bannerIndicators[m_currentBannerIndex]->setChecked(false);
    }

    m_currentBannerIndex = (m_currentBannerIndex + 1) % m_bannerImagePaths.size();

    ui->bannerStackedWidget->setCurrentIndex(m_currentBannerIndex);

    if (m_currentBannerIndex < m_bannerIndicators.size()) {
        m_bannerIndicators[m_currentBannerIndex]->setChecked(true);
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);

    qDebug() << "Window resized to:" << event->size();

    updateBannerImages();

    if (ui->contentStackedWidget && ui->contentStackedWidget->currentWidget() == ui->booksPage) {
        qDebug() << "Books page is active, triggering layout update via loadAndDisplayFilteredBooks().";
        loadAndDisplayFilteredBooks();
    }
    else if (ui->contentStackedWidget && ui->contentStackedWidget->currentWidget() == ui->authorsPage) {
        qDebug() << "Authors page is active, triggering layout update via loadAndDisplayAuthors().";
        loadAndDisplayAuthors();
    }
}

void MainWindow::showAuthorDetails(int authorId)
{
    qInfo() << "Attempting to show details for author ID:" << authorId;
    if (authorId <= 0) {
        qWarning() << "Invalid author ID received:" << authorId;
        QMessageBox::warning(this, tr("Помилка"), tr("Некоректний ідентифікатор автора."));
        return;
    }
    if (!m_dbManager) {
        QMessageBox::critical(this, tr("Помилка"), tr("Помилка доступу до бази даних."));
        return;
    }
    if (!ui->authorDetailsPage) {
         QMessageBox::critical(this, tr("Помилка інтерфейсу"), tr("Сторінка деталей автора не знайдена."));
         return;
    }

    AuthorDetailsInfo authorDetails = m_dbManager->getAuthorDetails(authorId);

    if (!authorDetails.found) {
        QMessageBox::warning(this, tr("Помилка"), tr("Не вдалося знайти інформацію для автора з ID %1.").arg(authorId));
        return;
    }

    populateAuthorDetailsPage(authorDetails);

    m_currentAuthorDetailsId = authorId;

    ui->contentStackedWidget->setCurrentWidget(ui->authorDetailsPage);
}

void MainWindow::populateAuthorDetailsPage(const AuthorDetailsInfo &details)
{
    if (!ui->authorDetailPhotoLabel || !ui->authorDetailNameLabel || !ui->authorDetailNationalityLabel ||
        !ui->authorDetailBiographyLabel || !ui->authorBooksHeaderLabel || !ui->authorBooksLayout ||
        !ui->authorBooksContainerWidget)
    {
        qWarning() << "populateAuthorDetailsPage: One or more author detail page widgets are null!";
        return;
    }

    QPixmap photoPixmap(details.imagePath);
    if (photoPixmap.isNull() || details.imagePath.isEmpty()) {
        ui->authorDetailPhotoLabel->setText(tr("👤"));
        ui->authorDetailPhotoLabel->setStyleSheet("QLabel { background-color: #e0e0e0; color: #555; border-radius: 90px; font-size: 80pt; qproperty-alignment: AlignCenter; border: 1px solid #ccc; }");
    } else {
        QPixmap scaledPixmap = photoPixmap.scaled(ui->authorDetailPhotoLabel->size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        QBitmap mask(scaledPixmap.size());
        mask.fill(Qt::color0);
        QPainter painter(&mask);
        painter.setBrush(Qt::color1);
        painter.drawEllipse(0, 0, scaledPixmap.width(), scaledPixmap.height());
        painter.end();
        scaledPixmap.setMask(mask);
        ui->authorDetailPhotoLabel->setPixmap(scaledPixmap);
        ui->authorDetailPhotoLabel->setStyleSheet("QLabel { border-radius: 90px; border: 1px solid #ccc; }");
    }

    ui->authorDetailNameLabel->setText(details.firstName + " " + details.lastName);

    QString nationalityAndYears = details.nationality;
    QString yearsString;
    if (details.birthDate.isValid()) {
        yearsString += QString::number(details.birthDate.year());
    }

    if (!nationalityAndYears.isEmpty() && !yearsString.isEmpty()) {
        nationalityAndYears += " (" + yearsString + ")";
    } else if (!yearsString.isEmpty()) {
        nationalityAndYears = yearsString;
    }
    ui->authorDetailNationalityLabel->setText(nationalityAndYears.isEmpty() ? tr("(Інформація відсутня)") : nationalityAndYears);


    ui->authorDetailBiographyLabel->setWordWrap(true);
    ui->authorDetailBiographyLabel->setText(details.biography.isEmpty() ? tr("(Опис відсутній)") : details.biography);

    ui->authorBooksHeaderLabel->setText(tr("Книги автора (%1)").arg(details.books.size()));
    displayBooks(details.books, ui->authorBooksLayout, ui->authorBooksContainerWidget);

    qInfo() << "Author details page populated for:" << details.firstName << details.lastName;
}

void MainWindow::loadCartFromDatabase()
{
    if (!m_dbManager) {
        qWarning() << "loadCartFromDatabase: DatabaseManager is null.";
        return;
    }
    if (m_currentCustomerId <= 0) {
        qWarning() << "loadCartFromDatabase: Invalid customer ID.";
        return;
    }

    qInfo() << "Завантаження корзини з БД для customerId:" << m_currentCustomerId;
    QMap<int, int> dbCartItems = m_dbManager->getCartItems(m_currentCustomerId);

    m_cartItems.clear();

    if (dbCartItems.isEmpty()) {
        qInfo() << "Корзина в БД порожня.";
        updateCartIcon();
        return;
    }

    int itemsLoaded = 0;
    int itemsSkipped = 0;
    for (auto it = dbCartItems.constBegin(); it != dbCartItems.constEnd(); ++it) {
        int bookId = it.key();
        int quantity = it.value();

        BookDisplayInfo bookInfo = m_dbManager->getBookDisplayInfoById(bookId);
        if (bookInfo.found) {
            if (quantity > bookInfo.stockQuantity) {
                qWarning() << "loadCartFromDatabase: Кількість товару (ID:" << bookId << ") в корзині (" << quantity
                           << ") перевищує наявну на складі (" << bookInfo.stockQuantity << "). Встановлюємо кількість на" << bookInfo.stockQuantity;
                quantity = bookInfo.stockQuantity;
                if (quantity > 0) {
                    m_dbManager->addOrUpdateCartItem(m_currentCustomerId, bookId, quantity);
                } else {
                    m_dbManager->removeCartItem(m_currentCustomerId, bookId);
                    itemsSkipped++;
                    continue;
                }
            }

            CartItem newItem;
            newItem.book = bookInfo;
            newItem.quantity = quantity;
            m_cartItems.insert(bookId, newItem);
            itemsLoaded++;
        } else {
            qWarning() << "loadCartFromDatabase: Не вдалося знайти інформацію для книги з ID" << bookId << ", яка є в корзині БД. Видаляємо з корзини БД.";
            m_dbManager->removeCartItem(m_currentCustomerId, bookId);
            itemsSkipped++;
        }
    }

    qInfo() << "Корзину завантажено з БД. Завантажено:" << itemsLoaded << ", Пропущено/Видалено:" << itemsSkipped;

    updateCartIcon();
    if (ui->contentStackedWidget->currentWidget() == ui->cartPage) {
        populateCartPage();
    }
}

void MainWindow::applyGenreFilter(const QString &genreName)
{
    qInfo() << "Applying filter for genre:" << genreName;

    ui->contentStackedWidget->setCurrentWidget(ui->booksPage);

    resetFilters();

    if (m_genreFilterListWidget) {
        bool genreFound = false;
        for (int i = 0; i < m_genreFilterListWidget->count(); ++i) {
            QListWidgetItem *item = m_genreFilterListWidget->item(i);
            if (item && item->text() == genreName) {
                m_genreFilterListWidget->blockSignals(true);
                item->setCheckState(Qt::Checked);
                m_genreFilterListWidget->blockSignals(false);
                genreFound = true;
                qInfo() << "Genre item found and checked:" << genreName;
                break;
            }
        }
        if (!genreFound) {
            qWarning() << "Genre item not found in filter list:" << genreName;
            QMessageBox::warning(this, tr("Помилка фільтрації"), tr("Жанр '%1' не знайдено у списку фільтрів.").arg(genreName));
            return;
        }
    } else {
        qWarning() << "applyGenreFilter: m_genreFilterListWidget is null!";
        return;
    }

    applyFilters();

    if (ui->filterButton) {
        ui->filterButton->show();
    }

    if (m_isFilterPanelVisible) {
        on_filterButton_clicked();
    }
}
