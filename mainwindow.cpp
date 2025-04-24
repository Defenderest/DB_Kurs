#include "mainwindow.h"
#include "./ui_mainwindow.h" // Keep UI include first if it depends on Qt headers
#include "database.h"      // Include DatabaseManager implementation and BookDisplayInfo struct
#include <QStatusBar>      // For showing status messages
#include <QMessageBox>     // For showing error dialogs
#include <QDebug>          // For logging
#include <QLabel>          // Для відображення тексту та зображень
#include <QVBoxLayout>     // Для компонування елементів картки
#include <QGridLayout>     // Для розміщення карток у сітці
#include <QPushButton>     // Для кнопки "Додати в кошик" (приклад)
#include <QPixmap>         // Для роботи з зображеннями
#include <QFrame>          // Для рамки картки
#include <QSizePolicy>     // Для налаштування розмірів
#include <QScrollArea>     // Щоб переконатися, що вміст прокручується
#include <QPainter>         // Додано для малювання круглої маски
#include <QBitmap>          // Додано для QBitmap (використовується з QPainter)
#include <QDate>            // Додано для форматування дати
#include <QPropertyAnimation> // Додано для анімації
#include <QEvent>           // Для eventFilter
#include <QEnterEvent>      // Для подій наведення миші
#include <QMap>             // Для QMap
#include <QDateTime>        // Для форматування дати/часу замовлення
#include <QLocale>          // Для форматування чисел та дат
// #include <QGroupBox>        // Більше не потрібен для замовлень
// #include <QTableWidget>     // Більше не потрібен для замовлень
// #include <QHeaderView>      // Більше не потрібен для замовлень
#include <QLineEdit>        // Додано для QLineEdit у профілі
#include <QCompleter>       // Додано для автодоповнення
#include <QStringListModel> // Додано для моделі автодоповнення
#include <QListView>        // Додано для QListView (використовується в автодоповненні)
#include <QMouseEvent>      // Додано для подій миші
#include <QTextEdit>        // Додано для QTextEdit (опис книги)
#include "starratingwidget.h" // Додано для StarRatingWidget
#include <QCoreApplication> // Додано для applicationDirPath
#include <QDir>             // Додано для роботи зі шляхами
// #include <QTableWidget>     // Більше не потрібен для кошика
#include <QHeaderView>      // Може бути потрібен для інших таблиць
#include <QSpinBox>         // Додано для зміни кількості в кошику
#include <QToolButton>      // Додано для кнопки видалення в кошику
#include <QMap>             // Додано для m_cartItems
#include <QScrollArea>      // Додано для нового кошика
#include <QTimer>           // Додано для таймера банера

MainWindow::MainWindow(DatabaseManager *dbManager, int customerId, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_dbManager(dbManager) // Ініціалізуємо вказівник переданим значенням
    , m_currentCustomerId(customerId) // Зберігаємо ID користувача
{
    ui->setupUi(this);

    // Перевіряємо, чи передано менеджер БД
    if (!m_dbManager) {
        qCritical() << "MainWindow: DatabaseManager is null! Cannot function properly.";
        QMessageBox::critical(this, tr("Критична помилка"), tr("Менеджер бази даних не ініціалізовано."));
        // Можливо, закрити вікно або заблокувати функціонал
        return;
    }

    // Перевіряємо ID користувача
    if (m_currentCustomerId <= 0) {
         qWarning() << "MainWindow: Invalid customer ID received:" << m_currentCustomerId;
         // Можна показати повідомлення або обмежити функціонал
         ui->statusBar->showMessage(tr("Помилка: Не вдалося визначити користувача."), 0);
    } else {
         ui->statusBar->showMessage(tr("Вітаємо!"), 5000);
         qInfo() << "MainWindow initialized for customer ID:" << m_currentCustomerId;
    }

    // Підключаємо сигнали кнопок навігації до слотів (використовуємо імена з UI)
    connect(ui->navHomeButton, &QPushButton::clicked, this, &MainWindow::on_navHomeButton_clicked);
    connect(ui->navBooksButton, &QPushButton::clicked, this, &MainWindow::on_navBooksButton_clicked);
    connect(ui->navAuthorsButton, &QPushButton::clicked, this, &MainWindow::on_navAuthorsButton_clicked);
    connect(ui->navOrdersButton, &QPushButton::clicked, this, &MainWindow::on_navOrdersButton_clicked);
    connect(ui->navProfileButton, &QPushButton::clicked, this, &MainWindow::on_navProfileButton_clicked); // Додано з'єднання для кнопки профілю

    // Підключаємо зміну комбо-боксу статусу замовлень до перезавантаження списку
    connect(ui->orderStatusComboBox, &QComboBox::currentIndexChanged, this, &MainWindow::loadAndDisplayOrders);

    // Підключаємо кнопки редагування та збереження профілю
    connect(ui->editProfileButton, &QPushButton::clicked, this, &MainWindow::on_editProfileButton_clicked);
    connect(ui->saveProfileButton, &QPushButton::clicked, this, &MainWindow::on_saveProfileButton_clicked);

    // Підключаємо кнопку кошика в хедері
    connect(ui->cartButton, &QPushButton::clicked, this, &MainWindow::on_cartButton_clicked);

    // Підключаємо кнопку "Оформити замовлення" на сторінці кошика (якщо вона вже існує в UI)
    // Переконайтесь, що віджет cartPage та placeOrderButton існують у вашому .ui файлі
    if (ui->cartPage && ui->placeOrderButton) {
        connect(ui->placeOrderButton, &QPushButton::clicked, this, &MainWindow::on_placeOrderButton_clicked);
    } else {
        qWarning() << "Cart page or place order button not found in UI. Cannot connect signal.";
    }

    // Зберігаємо оригінальний текст кнопок (без емодзі, беремо з UI)
    m_buttonOriginalText[ui->navHomeButton] = ui->navHomeButton->text();
    m_buttonOriginalText[ui->navBooksButton] = ui->navBooksButton->text();
    m_buttonOriginalText[ui->navAuthorsButton] = ui->navAuthorsButton->text();
    m_buttonOriginalText[ui->navOrdersButton] = ui->navOrdersButton->text();
    m_buttonOriginalText[ui->navProfileButton] = ui->navProfileButton->text();

    // Налаштовуємо анімацію бокової панелі
    setupSidebarAnimation();

    // Встановлюємо фільтр подій на sidebarFrame для відстеження наведення миші
    ui->sidebarFrame->installEventFilter(this);
    // Переконуємось, що панель спочатку згорнута
    toggleSidebar(false); // Згорнути без анімації при старті

    // --- Налаштування сторінки кошика ---
    // Переконуємось, що layout для елементів кошика існує
    if (!ui->cartItemsLayout) {
        qCritical() << "cartItemsLayout is null! Cart page might not work correctly.";
        // Можна створити layout динамічно, якщо його немає, але краще виправити UI
        if (ui->cartItemsContainerWidget) {
            QVBoxLayout *layout = new QVBoxLayout(ui->cartItemsContainerWidget);
            layout->setObjectName("cartItemsLayout");
            ui->cartItemsContainerWidget->setLayout(layout);
            qWarning() << "Dynamically created cartItemsLayout.";
        }
    } else {
         // Видаляємо спейсер, який був потрібен для таблиці/порожнього повідомлення
         QLayoutItem* item = ui->cartItemsLayout->takeAt(0); // Припускаємо, що спейсер перший
         if (item && item->spacerItem()) {
             delete item;
             qInfo() << "Removed initial spacer from cartItemsLayout.";
         } else if (item) {
             // Якщо це не спейсер, повертаємо його назад
             ui->cartItemsLayout->insertItem(0, item);
         }
    }


    // --- Завантаження та відображення даних для початкової сторінки (Головна) ---
    // Переконуємося, що відповідні layout'и існують перед заповненням
    // (Тепер вони всередині ui->discoverPage)
    qInfo() << "Завантаження даних для головної сторінки...";
    if (ui->classicsRowLayout) {
        QList<BookDisplayInfo> classicsBooks = m_dbManager->getBooksByGenre("Класика", 8); // Використовуємо getBooksByGenre
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

    // --- Завантаження даних для інших сторінок (можна зробити ледачим завантаженням при першому відкритті) ---
    // Завантаження книг для сторінки "Книги" (ui->booksPage)
    if (!ui->booksContainerLayout) {
         qCritical() << "booksContainerLayout is null!";
    } else {
        QList<BookDisplayInfo> books = m_dbManager->getAllBooksForDisplay();
        displayBooks(books); // Заповнюємо сторінку "Книги"
        if (!books.isEmpty()) {
             ui->statusBar->showMessage(tr("Книги успішно завантажено."), 4000);
        } else {
             qWarning() << "Не вдалося завантажити книги для сторінки 'Книги'.";
             // Повідомлення вже обробляється в displayBooks
        }
    }

    // Завантаження авторів для сторінки "Автори" (ui->authorsPage)
    if (!ui->authorsContainerLayout) {
        qCritical() << "authorsContainerLayout is null!";
    } else {
        QList<AuthorDisplayInfo> authors = m_dbManager->getAllAuthorsForDisplay();
        displayAuthors(authors);
        if (!authors.isEmpty()) {
             qInfo() << "Автори успішно завантажені.";
        } else {
             qWarning() << "Не вдалося завантажити авторів.";
             // Повідомлення вже обробляється в displayAuthors
        }
    }

    // Завантаження профілю для сторінки "Профіль" (ui->pageProfile)
    // (Завантаження відбувається при кліку на кнопку профілю в бічній панелі)

    // Завантаження замовлень (якщо потрібно при старті)
    // loadAndDisplayOrders();

    // Встановлюємо початковий стан сторінки профілю (не в режимі редагування)
    setProfileEditingEnabled(false);

    // Налаштовуємо автодоповнення для глобального пошуку
    setupSearchCompleter();

    // Блок else для помилки підключення більше не потрібен тут,
    // оскільки dbManager передається і перевіряється на початку конструктора.

    // Встановлюємо автоматичний банер програмно
    setupAutoBanner();

    // Підключаємо кнопку відправки коментаря
    connect(ui->sendCommentButton, &QPushButton::clicked, this, &MainWindow::on_sendCommentButton_clicked);
}

MainWindow::~MainWindow()
{
    // DatabaseManager будет удален автоматически благодаря установке родителя (this)
    // Но соединение лучше закрыть явно, если оно еще открыто
    if (m_dbManager) {
        m_dbManager->closeConnection();
        // delete m_dbManager; // Не нужно, если 'this' был родителем
    }
    delete ui;
} // Closing brace for destructor

// TODO: Додати метод для завантаження та відображення замовлень
// void MainWindow::loadAndDisplayOrders() { ... }


// --- Реалізація слотів та функцій ---

// Слоти для кнопок навігації
void MainWindow::on_navHomeButton_clicked()
{
    ui->contentStackedWidget->setCurrentWidget(ui->discoverPage); // Використовуємо ім'я сторінки з UI
}

void MainWindow::on_navBooksButton_clicked()
{
    ui->contentStackedWidget->setCurrentWidget(ui->booksPage); // Використовуємо ім'я сторінки з UI
    // Можна додати ледаче завантаження тут, якщо не зроблено в конструкторі
}

void MainWindow::on_navAuthorsButton_clicked()
{
    ui->contentStackedWidget->setCurrentWidget(ui->authorsPage); // Використовуємо ім'я сторінки з UI
    // Можна додати ледаче завантаження тут
}

// Налаштування анімації бокової панелі
void MainWindow::setupSidebarAnimation()
{
    m_sidebarAnimation = new QPropertyAnimation(ui->sidebarFrame, "maximumWidth", this);
    m_sidebarAnimation->setDuration(350); // Збільшено тривалість анімації
    m_sidebarAnimation->setEasingCurve(QEasingCurve::InOutCubic); // Змінено криву на більш плавну
}

// Функція для розгортання/згортання панелі
void MainWindow::toggleSidebar(bool expand)
{
    if (m_isSidebarExpanded == expand && m_sidebarAnimation->state() == QAbstractAnimation::Stopped) {
        return; // Вже в потрібному стані і анімація не йде
    }
     // Якщо анімація ще триває, зупиняємо її перед запуском нової
    if (m_sidebarAnimation->state() == QAbstractAnimation::Running) {
        m_sidebarAnimation->stop();
    }

    m_isSidebarExpanded = expand;

    // Оновлюємо текст кнопок
    for (auto it = m_buttonOriginalText.begin(); it != m_buttonOriginalText.end(); ++it) {
        QPushButton *button = it.key();
        const QString &originalText = it.value();
        if (expand) {
            button->setText(originalText); // Встановлюємо повний текст
            button->setIcon(button->icon()); // Переконуємось, що іконка є (вона вже має бути встановлена з UI)
            button->setToolTip(""); // Очистити підказку, коли текст видно
            button->setProperty("collapsed", false); // Встановлюємо властивість
        } else {
            button->setText(""); // Прибираємо текст
            button->setIcon(button->icon()); // Залишаємо тільки іконку
            button->setToolTip(originalText); // Показати текст як підказку
            button->setProperty("collapsed", true); // Встановлюємо властивість
        }
        // Примусово оновлюємо стиль кнопки
        button->style()->unpolish(button);
        button->style()->polish(button);
    }

    m_sidebarAnimation->setStartValue(ui->sidebarFrame->width());
    m_sidebarAnimation->setEndValue(expand ? m_expandedWidth : m_collapsedWidth);
    m_sidebarAnimation->start();
}


// Перехоплення подій для sidebarFrame
bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == ui->sidebarFrame) {
        if (event->type() == QEvent::Enter) {
            // Миша увійшла в область sidebarFrame
            toggleSidebar(true); // Розгорнути
            return true; // Подія оброблена
        } else if (event->type() == QEvent::Leave) {
            // Миша покинула область sidebarFrame
            toggleSidebar(false); // Згорнути
            return true; // Подія оброблена
        }
    }
    // --- Обробка кліків на картках книг ---
    // Перевіряємо, чи об'єкт є QFrame (картка книги) і чи має властивість bookId
    if (qobject_cast<QFrame*>(watched) && watched->property("bookId").isValid()) {
        if (event->type() == QEvent::MouseButtonPress) {
            // Переконуємось, що це ліва кнопка миші
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                int bookId = watched->property("bookId").toInt();
                qInfo() << "Book card clicked, bookId:" << bookId;
                showBookDetails(bookId); // Викликаємо слот для показу деталей
                return true; // Подія оброблена
            }
        }
    }

    // Передаємо подію батьківському класу для стандартної обробки
    return QMainWindow::eventFilter(watched, event);
} // Closing brace for eventFilter


// --- Логіка автоматичного банера ---

// Нова функція для оновлення зображень банера
void MainWindow::updateBannerImages()
{
    QList<QLabel*> bannerLabels = {ui->bannerLabel1, ui->bannerLabel2, ui->bannerLabel3};

    // Перевіряємо, чи шляхи до зображень вже завантажені
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
                // Масштабуємо Pixmap до поточного розміру QLabel, зберігаючи пропорції та заповнюючи простір
                QSize labelSize = bannerLabels[i]->size();
                // Перевірка на валідний розмір (більше 0)
                if (labelSize.isValid() && labelSize.width() > 0 && labelSize.height() > 0) {
                    // Змінюємо режим масштабування на KeepAspectRatio, щоб зображення завжди вміщувалося
                    QPixmap scaledPixmap = bannerPixmap.scaled(labelSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                    bannerLabels[i]->setPixmap(scaledPixmap);
                } else {
                    // Якщо розмір ще не встановлено, просто встановлюємо оригінальний pixmap
                    // або чекаємо наступного resizeEvent
                    bannerLabels[i]->setPixmap(bannerPixmap);
                    qDebug() << "Banner label" << i+1 << "size is invalid during update:" << labelSize << ". Setting original pixmap.";
                }
                bannerLabels[i]->setAlignment(Qt::AlignCenter); // Центруємо зображення
            }
        } else if (bannerLabels[i]) {
             bannerLabels[i]->setText(tr("Банер %1").arg(i + 1)); // Текст за замовчуванням, якщо шляху немає
             bannerLabels[i]->setAlignment(Qt::AlignCenter);
        }
    }
}


void MainWindow::setupAutoBanner()
{
    // 1. Вкажіть шляхи до ваших трьох зображень банерів у ресурсах
    m_bannerImagePaths.clear(); // Очищаємо список перед додаванням
    m_bannerImagePaths << ":/images/banner1.jpg"
                       << ":/images/banner2.jpg"
                       << ":/images/banner3.jpg";

    // Перевірка кількості банерів (має бути 3, як міток в UI)
    // Припускаємо, що в UI є 3 QLabel: bannerLabel1, bannerLabel2, bannerLabel3
    const int expectedBannerCount = 3;
    if (m_bannerImagePaths.size() != expectedBannerCount) {
        qWarning() << "Expected" << expectedBannerCount << "banner images, but found" << m_bannerImagePaths.size();
        // Можна додати обробку помилки, наприклад, не запускати таймер
        return;
    }

    // 2. Викликаємо функцію для початкового завантаження та масштабування зображень
    // Важливо: розміри віджетів можуть бути ще не остаточними тут.
    // resizeEvent буде викликаний пізніше для коректного масштабування.
    updateBannerImages();

    // 3. Налаштовуємо та запускаємо таймер
    m_bannerTimer = new QTimer(this);
    connect(m_bannerTimer, &QTimer::timeout, this, &MainWindow::showNextBanner);
    m_bannerTimer->start(5000); // Перемикати кожні 5 секунд (5000 мс)

    // 4. Збираємо індикатори
    m_bannerIndicators << ui->indicatorDot1 << ui->indicatorDot2 << ui->indicatorDot3;
    // Перевіряємо, чи кількість індикаторів відповідає кількості банерів
    if (m_bannerIndicators.size() != m_bannerImagePaths.size()) {
        qWarning() << "Mismatch between number of banner images and indicator dots!";
        // Можливо, варто відключити індикатори або таймер
    }

    // 5. Встановлюємо початковий банер та індикатор
    ui->bannerStackedWidget->setCurrentIndex(m_currentBannerIndex);
    if (!m_bannerIndicators.isEmpty() && m_currentBannerIndex < m_bannerIndicators.size()) {
        m_bannerIndicators[m_currentBannerIndex]->setChecked(true);
    }
}

void MainWindow::showNextBanner()
{
    if (m_bannerImagePaths.isEmpty() || m_bannerIndicators.isEmpty()) return; // Немає банерів або індикаторів

    // Знімаємо позначку з поточного індикатора
    if (m_currentBannerIndex < m_bannerIndicators.size()) {
        m_bannerIndicators[m_currentBannerIndex]->setChecked(false);
    }

    // Переходимо до наступного індексу
    m_currentBannerIndex = (m_currentBannerIndex + 1) % m_bannerImagePaths.size();

    // Встановлюємо новий банер
    ui->bannerStackedWidget->setCurrentIndex(m_currentBannerIndex);

    // Встановлюємо позначку на новому індикаторі
    if (m_currentBannerIndex < m_bannerIndicators.size()) {
        m_bannerIndicators[m_currentBannerIndex]->setChecked(true);
    }
}

// --- Кінець логіки автоматичного банера ---


// --- Обробка зміни розміру вікна ---
void MainWindow::resizeEvent(QResizeEvent *event)
{
    // Викликаємо реалізацію базового класу
    QMainWindow::resizeEvent(event);

    // Оновлюємо зображення банерів відповідно до нового розміру
    updateBannerImages();
}
// --- Кінець обробки зміни розміру вікна ---


// [Визначення функцій setupSearchCompleter та updateSearchSuggestions переміщено до mainwindow_search.cpp]

// --- Логіка кошика (Новий дизайн) ---

// [Визначення функцій createCartItemWidget, on_addToCartButtonClicked, on_cartButton_clicked,
//  populateCartPage, updateCartTotal, updateCartIcon, updateCartItemQuantity,
//  removeCartItem, on_placeOrderButton_clicked переміщено до mainwindow_cart.cpp]

// --- Кінець логіки кошика ---


// --- Кінець реалізації слотів та функцій ---
