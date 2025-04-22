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

    // Встановлюємо банер програмно
    setupBannerImage();

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

// Налаштування автодоповнення для глобального пошуку
void MainWindow::setupSearchCompleter()
{
    if (!ui->globalSearchLineEdit) {
        qWarning() << "setupSearchCompleter: globalSearchLineEdit is null!";
        return;
    }

    m_searchSuggestionModel = new QStringListModel(this); // Модель для пропозицій
    m_searchCompleter = new QCompleter(m_searchSuggestionModel, this); // Комплітер

    m_searchCompleter->setCaseSensitivity(Qt::CaseInsensitive); // Нечутливий до регістру
    m_searchCompleter->setCompletionMode(QCompleter::PopupCompletion); // Випадаючий список
    m_searchCompleter->setFilterMode(Qt::MatchStartsWith); // Пропозиції, що починаються з введеного тексту
    // m_searchCompleter->setPopup(ui->globalSearchLineEdit->findChild<QListView*>()); // Використовуємо стандартний popup - findChild може бути ненадійним, краще залишити стандартний popup комплітера

    ui->globalSearchLineEdit->setCompleter(m_searchCompleter);

    // Підключаємо сигнал зміни тексту до слота оновлення пропозицій
    connect(ui->globalSearchLineEdit, &QLineEdit::textChanged, this, &MainWindow::updateSearchSuggestions);

    qInfo() << "Search completer setup complete for globalSearchLineEdit.";
}

// Слот для оновлення пропозицій пошуку при зміні тексту
void MainWindow::updateSearchSuggestions(const QString &text)
{
    if (!m_dbManager || !m_searchSuggestionModel) {
        return; // Немає менеджера БД або моделі
    }

    // Отримуємо пропозиції, тільки якщо текст достатньо довгий
    if (text.length() < 2) { // Мінімальна довжина для пошуку (можна змінити)
        m_searchSuggestionModel->setStringList({}); // Очищаємо модель, якщо текст короткий
        return;
    }

    QStringList suggestions = m_dbManager->getSearchSuggestions(text);
    m_searchSuggestionModel->setStringList(suggestions); // Оновлюємо модель пропозиціями

    // qInfo() << "Updated search suggestions for text:" << text << "Count:" << suggestions.count();
}
// --- Логіка кошика (Новий дизайн) ---

// Допоміжна функція для створення віджету одного товару в кошику
QWidget* MainWindow::createCartItemWidget(const CartItem &item, int bookId)
{
    // Основний фрейм для картки товару
    QFrame *itemFrame = new QFrame();
    itemFrame->setObjectName("cartItemFrame"); // Для стилізації
    itemFrame->setFrameShape(QFrame::StyledPanel); // Невидима рамка, керується стилем
    itemFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed); // Розширюється по ширині, фіксована висота

    QHBoxLayout *mainLayout = new QHBoxLayout(itemFrame);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(0, 0, 0, 0); // Відступи керуються стилем фрейму

    // 1. Обкладинка
    QLabel *coverLabel = new QLabel();
    coverLabel->setObjectName("cartItemCoverLabel");
    coverLabel->setAlignment(Qt::AlignCenter);
    QPixmap coverPixmap(item.book.coverImagePath);
    if (coverPixmap.isNull() || item.book.coverImagePath.isEmpty()) {
        coverLabel->setText(tr("Фото"));
        // Можна додати стиль для плейсхолдера, якщо потрібно
        // coverLabel->setStyleSheet("background-color: #e9ecef; color: #6c757d; border: 1px solid #dee2e6; border-radius: 4px;");
    } else {
        // Масштабуємо зображення до розміру QLabel (використовуємо minimumSize, визначений у стилі)
        // Переконуємось, що розмір QLabel встановлено (наприклад, через стиль або minimumSize)
        QSize labelSize = coverLabel->minimumSize(); // Беремо розмір з UI/стилю
        if (!labelSize.isValid() || labelSize.width() <= 0 || labelSize.height() <= 0) {
             // Якщо розмір не встановлено, використовуємо типовий
             labelSize = QSize(60, 85);
             qWarning() << "Cart item cover label size not set, using default:" << labelSize;
        }
        coverLabel->setPixmap(coverPixmap.scaled(labelSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        coverLabel->setText(""); // Прибираємо текст, якщо є зображення
    }
    mainLayout->addWidget(coverLabel);

    // 2. Інформація про книгу (Назва, Автор)
    QVBoxLayout *infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(2);
    QLabel *titleLabel = new QLabel(item.book.title);
    titleLabel->setObjectName("cartItemTitleLabel");
    titleLabel->setWordWrap(true);
    QLabel *authorLabel = new QLabel(item.book.authors);
    authorLabel->setObjectName("cartItemAuthorLabel");
    authorLabel->setWordWrap(true);
    infoLayout->addWidget(titleLabel);
    infoLayout->addWidget(authorLabel);
    infoLayout->addStretch(1); // Притискає текст вгору
    mainLayout->addLayout(infoLayout, 2); // Даємо більше місця для назви/автора

    // 3. Ціна за одиницю
    QLabel *priceLabel = new QLabel(QString::number(item.book.price, 'f', 2) + tr(" грн"));
    priceLabel->setObjectName("cartItemPriceLabel");
    priceLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    mainLayout->addWidget(priceLabel, 1); // Менше місця для ціни

    // 4. Кількість (SpinBox)
    QSpinBox *quantitySpinBox = new QSpinBox();
    quantitySpinBox->setObjectName("cartQuantitySpinBox");
    quantitySpinBox->setMinimum(1);
    quantitySpinBox->setMaximum(item.book.stockQuantity); // Обмеження по складу
    quantitySpinBox->setValue(item.quantity);
    quantitySpinBox->setAlignment(Qt::AlignCenter);
    quantitySpinBox->setProperty("bookId", bookId); // Зберігаємо ID для слота
    connect(quantitySpinBox, &QSpinBox::valueChanged, this, [this, bookId](int newValue){
        updateCartItemQuantity(bookId, newValue);
    });
    mainLayout->addWidget(quantitySpinBox);

    // 5. Сума за позицію
    QLabel *subtotalLabel = new QLabel(QString::number(item.book.price * item.quantity, 'f', 2) + tr(" грн"));
    subtotalLabel->setObjectName("cartItemSubtotalLabel");
    subtotalLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    subtotalLabel->setMinimumWidth(80); // Мінімальна ширина для суми
    mainLayout->addWidget(subtotalLabel, 1); // Менше місця для суми
    // Зберігаємо вказівник на мітку підсумку
    m_cartSubtotalLabels.insert(bookId, subtotalLabel);


    // 6. Кнопка видалення
    QPushButton *removeButton = new QPushButton(); // Використовуємо QPushButton для кращої стилізації
    removeButton->setObjectName("cartRemoveButton");
    removeButton->setToolTip(tr("Видалити '%1' з кошика").arg(item.book.title));
    removeButton->setCursor(Qt::PointingHandCursor);
    removeButton->setProperty("bookId", bookId);
    connect(removeButton, &QPushButton::clicked, this, [this, bookId](){
        removeCartItem(bookId);
    });
    mainLayout->addWidget(removeButton);

    itemFrame->setLayout(mainLayout);
    return itemFrame;
}


// Слот для кнопки "Додати в кошик" (залишається без змін)
void MainWindow::on_addToCartButtonClicked(int bookId)
{
    qInfo() << "Add to cart button clicked for book ID:" << bookId;
    if (!m_dbManager) {
        QMessageBox::critical(this, tr("Помилка"), tr("Помилка доступу до бази даних."));
        return;
    }

    // Отримуємо інформацію про книгу за допомогою нового методу
    BookDisplayInfo bookInfo = m_dbManager->getBookDisplayInfoById(bookId);

    if (!bookInfo.found) { // Перевіряємо прапорець found
         qWarning() << "Book with ID" << bookId << "not found for adding to cart.";
         QMessageBox::warning(this, tr("Помилка"), tr("Не вдалося знайти інформацію про книгу (ID: %1).").arg(bookId));
         return;
    }

    // Перевірка наявності на складі
    if (bookInfo.stockQuantity <= 0) {
        QMessageBox::information(this, tr("Немає в наявності"), tr("На жаль, книги '%1' зараз немає в наявності.").arg(bookInfo.title));
        return;
    }

    // Перевіряємо, чи товар вже є в кошику
    if (m_cartItems.contains(bookId)) {
        // Перевіряємо, чи не перевищить кількість наявну на складі
        if (m_cartItems[bookId].quantity + 1 > bookInfo.stockQuantity) {
             QMessageBox::information(this, tr("Обмеження кількості"), tr("Ви не можете додати більше одиниць книги '%1', ніж є на складі (%2).").arg(bookInfo.title).arg(bookInfo.stockQuantity));
             return;
        }
        // Збільшуємо кількість
        m_cartItems[bookId].quantity++;
        qInfo() << "Increased quantity for book ID" << bookId << "to" << m_cartItems[bookId].quantity;
    } else {
        // Додаємо новий товар
        CartItem newItem;
        newItem.book = bookInfo; // Зберігаємо інформацію про книгу
        newItem.quantity = 1;
        m_cartItems.insert(bookId, newItem);
        qInfo() << "Added new book ID" << bookId << "to cart.";
    }

    // Оновлюємо іконку кошика
    updateCartIcon();

    // Показуємо повідомлення в статус барі
    ui->statusBar->showMessage(tr("Книгу '%1' додано до кошика.").arg(bookInfo.title), 3000);

    // Оновлюємо сторінку кошика, якщо вона відкрита
    if (ui->contentStackedWidget->currentWidget() == ui->cartPage) {
        populateCartPage();
    }
}

// Слот для кнопки кошика в хедері
void MainWindow::on_cartButton_clicked()
{
    qInfo() << "Cart button clicked. Navigating to cart page.";
    // Перевіряємо, чи існує сторінка кошика
    if (!ui->cartPage) {
        qWarning() << "Cart page widget not found in UI!";
        QMessageBox::critical(this, tr("Помилка інтерфейсу"), tr("Сторінка кошика не знайдена."));
        return;
    }
    ui->contentStackedWidget->setCurrentWidget(ui->cartPage);
    populateCartPage(); // Заповнюємо/оновлюємо сторінку кошика
}

// Заповнення сторінки кошика (Новий дизайн)
void MainWindow::populateCartPage()
{
    qInfo() << "Populating cart page (new design)...";
    // Перевіряємо існування ключових віджетів нового дизайну
    if (!ui->cartScrollArea || !ui->cartItemsContainerWidget || !ui->cartItemsLayout || !ui->cartTotalLabel || !ui->placeOrderButton || !ui->cartTotalsWidget) {
        qWarning() << "populateCartPage: One or more new cart page widgets are null!";
        // Можна показати повідомлення про помилку
        if(ui->cartPage && ui->cartPage->layout()) {
             clearLayout(ui->cartPage->layout()); // Очистити, щоб не було старих елементів
             QLabel *errorLabel = new QLabel(tr("Помилка інтерфейсу: Не вдалося відобразити кошик."), ui->cartPage);
             ui->cartPage->layout()->addWidget(errorLabel);
        }
        return;
    }

    // Очищаємо layout від попередніх елементів (карток товарів та спейсера/мітки)
    clearLayout(ui->cartItemsLayout);
    // Очищаємо мапу вказівників на мітки підсумків
    m_cartSubtotalLabels.clear();


    // Видаляємо мітку про порожній кошик, якщо вона була додана раніше
    QLabel* emptyCartLabel = ui->cartItemsContainerWidget->findChild<QLabel*>("emptyCartLabel");
    if(emptyCartLabel) {
        delete emptyCartLabel;
    }

    if (m_cartItems.isEmpty()) {
        qInfo() << "Cart is empty.";
        // Показуємо мітку про порожній кошик всередині контейнера
        QLabel *noItemsLabel = new QLabel(tr("🛒\n\nВаш кошик порожній.\nЧас додати щось цікаве!"), ui->cartItemsContainerWidget);
        noItemsLabel->setObjectName("emptyCartLabel"); // Для стилізації
        noItemsLabel->setAlignment(Qt::AlignCenter);
        noItemsLabel->setWordWrap(true);
        // Додаємо мітку безпосередньо в layout контейнера (не в cartItemsLayout)
        // Потрібно переконатися, що cartItemsContainerWidget має свій layout, якщо ми хочемо центрувати мітку
        // Простіше додати її в cartItemsLayout і потім спейсер
        ui->cartItemsLayout->addWidget(noItemsLabel);
        ui->cartItemsLayout->addSpacerItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));

        ui->placeOrderButton->setEnabled(false); // Вимкнути кнопку замовлення
        ui->cartTotalLabel->setText(tr("Загальна сума: 0.00 грн")); // Скинути суму
        ui->cartTotalsWidget->setVisible(false); // Ховаємо блок з підсумками
        return;
    }

    // Показуємо блок з підсумками
    ui->cartTotalsWidget->setVisible(true);

    // Додаємо віджети для кожного товару
    for (auto it = m_cartItems.constBegin(); it != m_cartItems.constEnd(); ++it) {
        QWidget *itemWidget = createCartItemWidget(it.value(), it.key());
        if (itemWidget) {
            ui->cartItemsLayout->addWidget(itemWidget);
        }
    }

    // Додаємо спейсер в кінці, щоб притиснути картки вгору
    ui->cartItemsLayout->addSpacerItem(new QSpacerItem(20, 1, QSizePolicy::Minimum, QSizePolicy::Expanding));

    updateCartTotal(); // Оновлюємо загальну суму
    ui->placeOrderButton->setEnabled(true); // Увімкнути кнопку замовлення
    qInfo() << "Cart page populated with" << m_cartItems.size() << "items.";

    // Оновлюємо геометрію контейнера, щоб ScrollArea знала розмір вмісту
    ui->cartItemsContainerWidget->adjustSize();
}


// Оновлення загальної суми кошика (Новий дизайн)
void MainWindow::updateCartTotal()
{
    if (!ui->cartTotalLabel) return; // Перевірка

    double total = 0.0;
    // Рахуємо суму безпосередньо з m_cartItems
    for (const auto &item : m_cartItems) {
        total += item.book.price * item.quantity;
    }

    ui->cartTotalLabel->setText(tr("Загальна сума: %1 грн").arg(QString::number(total, 'f', 2)));
    qInfo() << "Cart total updated:" << total;
}

// Оновлення іконки кошика (кількість товарів)
void MainWindow::updateCartIcon()
{
    if (!ui->cartButton) return; // Перевірка

    int totalItems = 0;
    for (const auto &item : m_cartItems) {
        totalItems += item.quantity;
    }

    if (totalItems > 0) {
        // Показуємо кількість на кнопці (простий варіант - змінити текст)
        ui->cartButton->setText(QString("🛒 (%1)").arg(totalItems));
        ui->cartButton->setToolTip(tr("Кошик (%1 товар(ів))").arg(totalItems));
    } else {
        // Повертаємо стандартний вигляд
        ui->cartButton->setText("🛒");
        ui->cartButton->setToolTip(tr("Кошик"));
    }
    qInfo() << "Cart icon updated. Total items:" << totalItems;
}

// Слот для зміни кількості товару в кошику (Новий дизайн - виправлено виліт)
void MainWindow::updateCartItemQuantity(int bookId, int quantity)
{
    if (m_cartItems.contains(bookId)) {
        qInfo() << "Updating quantity for book ID" << bookId << "to" << quantity;

        // Перевіряємо, чи кількість не перевищує доступну (SpinBox сам обмежує, але для безпеки)
        int stockQuantity = m_cartItems[bookId].book.stockQuantity;
        if (quantity > stockQuantity) {
            qWarning() << "Attempted to set quantity" << quantity << "but only" << stockQuantity << "available for book ID" << bookId;
            quantity = stockQuantity;
            // Потрібно знайти SpinBox і встановити йому правильне значення, якщо воно змінилося
            // Це може бути складно, простіше попередити користувача або дозволити SpinBox обробити це.
            // Наразі просто використовуємо скориговане значення.
        }
        if (quantity < 1) { // Мінімальна кількість - 1
             qWarning() << "Attempted to set quantity less than 1 for book ID" << bookId;
             quantity = 1;
        }


        // Оновлюємо кількість у внутрішній структурі даних
        m_cartItems[bookId].quantity = quantity;

        // Оновлюємо мітку підсумку для цього товару
        QLabel *subtotalLabel = m_cartSubtotalLabels.value(bookId, nullptr);
        if (subtotalLabel) {
            double newSubtotal = m_cartItems[bookId].book.price * quantity;
            subtotalLabel->setText(QString::number(newSubtotal, 'f', 2) + tr(" грн"));
            qInfo() << "Updated subtotal label for book ID" << bookId;
        } else {
            qWarning() << "Could not find subtotal label for book ID" << bookId << "to update.";
            // Якщо мітки немає, можливо, варто перезавантажити кошик для консистентності
            if (ui->contentStackedWidget->currentWidget() == ui->cartPage) {
                populateCartPage();
            }
        }

        // Оновлюємо загальну суму та іконку кошика
        updateCartTotal();
        updateCartIcon();

    } else {
        qWarning() << "Attempted to update quantity for non-existent book ID in cart:" << bookId;
    }
}

// Слот для видалення товару з кошика (Новий дизайн)
void MainWindow::removeCartItem(int bookId)
{
     if (m_cartItems.contains(bookId)) {
         QString bookTitle = m_cartItems[bookId].book.title; // Зберігаємо назву для повідомлення
         m_cartItems.remove(bookId);
         qInfo() << "Removed book ID" << bookId << "from cart.";
         ui->statusBar->showMessage(tr("Книгу '%1' видалено з кошика.").arg(bookTitle), 3000);

         // Перезаповнюємо сторінку кошика, щоб видалити віджет
         if (ui->contentStackedWidget->currentWidget() == ui->cartPage) {
            populateCartPage();
         }
         // updateCartTotal(); // populateCartPage вже викликає updateCartTotal
         updateCartIcon(); // Оновлюємо іконку
     } else {
         qWarning() << "Attempted to remove non-existent book ID from cart:" << bookId;
     }
}

// Слот для кнопки "Оформити замовлення"
void MainWindow::on_placeOrderButton_clicked()
{
    qInfo() << "Place order button clicked.";
    if (m_cartItems.isEmpty()) {
        QMessageBox::information(this, tr("Порожній кошик"), tr("Ваш кошик порожній. Додайте товари перед оформленням замовлення."));
        return;
    }
    if (!m_dbManager) {
        QMessageBox::critical(this, tr("Помилка"), tr("Помилка доступу до бази даних. Неможливо оформити замовлення."));
        return;
    }
     if (m_currentCustomerId <= 0) {
        QMessageBox::critical(this, tr("Помилка"), tr("Неможливо оформити замовлення, користувач не визначений."));
        return;
    }

    // --- Отримання даних для замовлення ---
    // 1. Адреса доставки (беремо з профілю або запитуємо)
    CustomerProfileInfo profile = m_dbManager->getCustomerProfileInfo(m_currentCustomerId);
    QString shippingAddress = profile.found ? profile.address : "";
    if (shippingAddress.isEmpty()) {
        // Перевіряємо існування віджетів профілю перед фокусуванням
        if (!ui->pageProfile || !ui->profileAddressLineEdit) {
             QMessageBox::warning(this, tr("Адреса доставки"), tr("Будь ласка, вкажіть адресу доставки у вашому профілі перед оформленням замовлення.\n(Помилка: не знайдено поля адреси в інтерфейсі профілю)"));
             return;
        }
        QMessageBox::warning(this, tr("Адреса доставки"), tr("Будь ласка, вкажіть адресу доставки у вашому профілі перед оформленням замовлення."));
        // Перенаправляємо на сторінку профілю
        on_navProfileButton_clicked();
        ui->profileAddressLineEdit->setFocus(); // Встановлюємо фокус на поле адреси
        setProfileEditingEnabled(true); // Вмикаємо редагування
        return;
    }

    // 2. Спосіб оплати (поки що фіксований)
    QString paymentMethod = tr("Готівка при отриманні"); // Або показати діалог вибору

    // Готуємо дані для createOrder (bookId -> quantity)
    QMap<int, int> itemsMap;
    for (auto it = m_cartItems.constBegin(); it != m_cartItems.constEnd(); ++it) {
        itemsMap.insert(it.key(), it.value().quantity);
    }


    // --- Виклик методу DatabaseManager для створення замовлення ---
    int newOrderId = -1;
    bool success = m_dbManager->createOrder(m_currentCustomerId, itemsMap, shippingAddress, paymentMethod, newOrderId);

    if (success && newOrderId > 0) {
        QMessageBox::information(this, tr("Замовлення оформлено"), tr("Ваше замовлення №%1 успішно оформлено!").arg(newOrderId));
        m_cartItems.clear(); // Очищаємо кошик
        updateCartIcon(); // Оновлюємо іконку
        populateCartPage(); // Оновлюємо сторінку кошика (стане порожньою)
        // Можна перейти на сторінку замовлень
        on_navOrdersButton_clicked();
    } else {
        QMessageBox::critical(this, tr("Помилка оформлення"), tr("Не вдалося оформити замовлення. Перевірте журнал помилок або спробуйте пізніше."));
        qWarning() << "Failed to create order. DB Error:" << m_dbManager->lastError().text();
    }
}


// --- Кінець логіки кошика ---


// --- Кінець реалізації слотів та функцій ---
