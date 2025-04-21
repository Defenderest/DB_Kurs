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
}


// Метод для очищення Layout
void MainWindow::clearLayout(QLayout* layout) {
    if (!layout) return;
    QLayoutItem* item;
    while ((item = layout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            delete item->widget(); // Видаляємо віджет
        }
        delete item; // Видаляємо елемент layout
    }
}

// Метод для створення віджету картки книги
QWidget* MainWindow::createBookCardWidget(const BookDisplayInfo &bookInfo)
{
    // Основний віджет картки (використовуємо QFrame для рамки)
    QFrame *cardFrame = new QFrame();
    cardFrame->setFrameShape(QFrame::StyledPanel); // Додає рамку
    cardFrame->setFrameShadow(QFrame::Raised);     // Додає тінь
    cardFrame->setLineWidth(1);
    cardFrame->setMinimumSize(200, 300); // Мінімальний розмір картки
    // cardFrame->setMaximumSize(250, 350); // Максимальний розмір можна прибрати або залишити, якщо потрібно обмеження
    cardFrame->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred); // Дозволяємо розтягуватись в комірці
    cardFrame->setStyleSheet("QFrame { background-color: white; border-radius: 8px; }"); // Стиль картки

    // Вертикальний layout для вмісту картки
    QVBoxLayout *cardLayout = new QVBoxLayout(cardFrame);
    cardLayout->setSpacing(8);
    cardLayout->setContentsMargins(10, 10, 10, 10);

    // 1. Обкладинка книги (QLabel)
    QLabel *coverLabel = new QLabel();
    coverLabel->setAlignment(Qt::AlignCenter);
    coverLabel->setMinimumHeight(150); // Мінімальна висота для обкладинки
    coverLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding); // Розтягувати
    QPixmap coverPixmap(bookInfo.coverImagePath);
    if (coverPixmap.isNull()) {
        // Якщо зображення не завантажилось, показуємо плейсхолдер
        coverLabel->setText(tr("Немає\nобкладинки"));
        coverLabel->setStyleSheet("QLabel { background-color: #e0e0e0; color: #555; border-radius: 4px; }");
    } else {
        // Масштабуємо зображення, зберігаючи пропорції
        coverLabel->setPixmap(coverPixmap.scaled(180, 240, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    cardLayout->addWidget(coverLabel);

    // 2. Назва книги (QLabel)
    QLabel *titleLabel = new QLabel(bookInfo.title);
    titleLabel->setWordWrap(true); // Переносити текст
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("QLabel { font-weight: bold; font-size: 11pt; }");
    cardLayout->addWidget(titleLabel);

    // 3. Автор(и) (QLabel)
    QLabel *authorLabel = new QLabel(bookInfo.authors.isEmpty() ? tr("Невідомий автор") : bookInfo.authors);
    authorLabel->setWordWrap(true);
    authorLabel->setAlignment(Qt::AlignCenter);
    authorLabel->setStyleSheet("QLabel { color: #555; font-size: 9pt; }");
    cardLayout->addWidget(authorLabel);

    // 4. Ціна (QLabel)
    QLabel *priceLabel = new QLabel(QString::number(bookInfo.price, 'f', 2) + tr(" грн"));
    priceLabel->setAlignment(Qt::AlignCenter);
    priceLabel->setStyleSheet("QLabel { font-weight: bold; color: #007bff; font-size: 10pt; margin-top: 5px; }");
    cardLayout->addWidget(priceLabel);

    // Додаємо розтягувач, щоб притиснути кнопку вниз (якщо вона є)
    cardLayout->addStretch(1);

    // 5. Кнопка "Додати в кошик" (QPushButton - приклад)
    QPushButton *addToCartButton = new QPushButton(tr("🛒 Додати"));
    addToCartButton->setStyleSheet("QPushButton { background-color: #28a745; color: white; border: none; border-radius: 8px; padding: 8px; font-size: 9pt; } QPushButton:hover { background-color: #218838; }");
    addToCartButton->setToolTip(tr("Додати '%1' до кошика").arg(bookInfo.title));
    // Зберігаємо bookId як властивість кнопки для легкого доступу в слоті
    addToCartButton->setProperty("bookId", bookInfo.bookId);
    // Підключаємо сигнал кнопки до слота on_addToCartButtonClicked
    connect(addToCartButton, &QPushButton::clicked, this, [this, bookId = bookInfo.bookId](){
        on_addToCartButtonClicked(bookId);
    });
    cardLayout->addWidget(addToCartButton);


    // --- Додавання обробки кліків ---
    // Встановлюємо bookId як динамічну властивість для легкого доступу в eventFilter
    cardFrame->setProperty("bookId", bookInfo.bookId);
    // Встановлюємо фільтр подій на сам фрейм картки
    cardFrame->installEventFilter(this);
    // Змінюємо курсор при наведенні, щоб показати клікабельність
    cardFrame->setCursor(Qt::PointingHandCursor);


    cardFrame->setLayout(cardLayout); // Встановлюємо layout для фрейму
    return cardFrame;
}


// Слот для відображення книг у сітці
void MainWindow::displayBooks(const QList<BookDisplayInfo> &books)
{
    const int maxColumns = 5; // Змінено кількість колонок у сітці на 5

    // Очищаємо попередні віджети з booksContainerLayout (всередині pageBooks)
    if (!ui->booksContainerLayout) {
        qWarning() << "displayBooks: booksContainerLayout is null!";
        // Можливо, показати повідомлення в statusBar або в самій області
        QLabel *errorLabel = new QLabel(tr("Помилка: Не вдалося знайти область для відображення книг."), ui->booksContainerWidget);
        errorLabel->setAlignment(Qt::AlignCenter);
        ui->booksContainerWidget->setLayout(new QVBoxLayout()); // Потрібен layout для додавання мітки
        ui->booksContainerWidget->layout()->addWidget(errorLabel);
        return;
    }
    clearLayout(ui->booksContainerLayout);

    if (books.isEmpty()) {
        QLabel *noBooksLabel = new QLabel(tr("Не вдалося завантажити книги або їх немає в базі даних."), ui->booksContainerWidget);
        noBooksLabel->setAlignment(Qt::AlignCenter);
        noBooksLabel->setWordWrap(true);
        // Додаємо мітку безпосередньо в layout, якщо він існує
        ui->booksContainerLayout->addWidget(noBooksLabel, 0, 0, 1, maxColumns); // Розтягнути на кілька колонок
        return; // Виходимо, якщо книг немає
    }


    int row = 0;
    int col = 0;
    // const int maxColumns = 5; // Перенесено на початок функції

    for (const BookDisplayInfo &bookInfo : books) {
        QWidget *bookCard = createBookCardWidget(bookInfo);
        if (bookCard) {
            ui->booksContainerLayout->addWidget(bookCard, row, col);
            col++;
            if (col >= maxColumns) {
                col = 0;
                row++;
            }
        }
    }

    // Встановлюємо однакове розтягування для колонок з картками
    for (int c = 0; c < maxColumns; ++c) {
        ui->booksContainerLayout->setColumnStretch(c, 1);
    }
    // Встановлюємо розтягування для колонки після карток (де горизонтальний спейсер)
    // Це гарантує, що колонки з картками будуть однакової ширини, а зайвий простір піде в останню колонку.
    ui->booksContainerLayout->setColumnStretch(maxColumns, 99); // Велике значення, щоб забрати весь зайвий простір

    // Видаляємо попередні розширювачі, якщо вони були додані раніше (про всяк випадок)
    QLayoutItem* item;
    // Видаляємо вертикальний розширювач знизу (якщо він є)
    item = ui->booksContainerLayout->itemAtPosition(row + 1, 0);
    if (item && item->spacerItem()) {
        ui->booksContainerLayout->removeItem(item);
        delete item;
    }
     // Видаляємо горизонтальний розширювач справа (якщо він є)
    item = ui->booksContainerLayout->itemAtPosition(0, maxColumns);
     if (item && item->spacerItem()) {
        ui->booksContainerLayout->removeItem(item);
        delete item;
    }

    // Додаємо горизонтальний розширювач в першому рядку після останньої колонки карток,
    // щоб притиснути картки вліво.
    ui->booksContainerLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum), 0, maxColumns);
    // Додаємо вертикальний розширювач під останнім рядком карток,
    // щоб притиснути картки вгору.
    ui->booksContainerLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding), row + 1, 0, 1, maxColumns);


    // Переконуємося, що контейнер оновився
    ui->booksContainerWidget->updateGeometry();
    // ui->booksScrollArea->updateGeometry(); // Зазвичай не потрібно викликати для ScrollArea
}


// Допоміжна функція для відображення книг у горизонтальному layout
void MainWindow::displayBooksInHorizontalLayout(const QList<BookDisplayInfo> &books, QHBoxLayout* layout)
{
    if (!layout) {
        qWarning() << "Target layout for horizontal display is null!";
        return;
    }
    // Очищаємо попередні віджети та розширювачі
    clearLayout(layout);

    if (books.isEmpty()) {
        // Якщо книг немає, показуємо повідомлення
        QLabel *noBooksLabel = new QLabel(tr("Для цього розділу книг не знайдено."));
        noBooksLabel->setAlignment(Qt::AlignCenter);
        noBooksLabel->setStyleSheet("QLabel { color: #777; font-style: italic; }");
        layout->addWidget(noBooksLabel, 1); // Додаємо з розтягуванням
    } else {
        for (const BookDisplayInfo &bookInfo : books) {
            QWidget *bookCard = createBookCardWidget(bookInfo);
            if (bookCard) {
                // Встановлюємо фіксовану або максимальну ширину для карток у горизонтальному ряду
                bookCard->setMinimumWidth(180);
                bookCard->setMaximumWidth(220);
                layout->addWidget(bookCard);
            }
        }
        // Додаємо розширювач в кінці, щоб притиснути картки вліво
        layout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum));
    }

    // Оновлюємо геометрію батьківського віджета (QWidget всередині QScrollArea)
    if (layout->parentWidget()) {
        layout->parentWidget()->updateGeometry();
    }
}


// Метод для створення віджету картки автора
QWidget* MainWindow::createAuthorCardWidget(const AuthorDisplayInfo &authorInfo)
{
    // Основний віджет картки (QFrame)
    QFrame *cardFrame = new QFrame();
    cardFrame->setFrameShape(QFrame::StyledPanel);
    cardFrame->setFrameShadow(QFrame::Raised);
    cardFrame->setLineWidth(1);
    cardFrame->setMinimumSize(180, 250); // Розмір картки автора
    cardFrame->setMaximumSize(220, 280);
    cardFrame->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed); // Фіксована висота
    cardFrame->setStyleSheet("QFrame { background-color: white; border-radius: 8px; }");

    // Вертикальний layout для вмісту картки
    QVBoxLayout *cardLayout = new QVBoxLayout(cardFrame);
    cardLayout->setSpacing(6);
    cardLayout->setContentsMargins(10, 10, 10, 10);

    // 1. Фото автора (QLabel)
    QLabel *photoLabel = new QLabel();
    photoLabel->setAlignment(Qt::AlignCenter);
    photoLabel->setMinimumSize(150, 150); // Розмір фото
    photoLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed); // Фіксована висота
    QPixmap photoPixmap(authorInfo.imagePath);
    if (photoPixmap.isNull() || authorInfo.imagePath.isEmpty()) {
        // Плейсхолдер, якщо фото немає
        photoLabel->setText(tr("👤")); // Іконка користувача
        photoLabel->setStyleSheet("QLabel { background-color: #e0e0e0; color: #555; border-radius: 75px; font-size: 80pt; qproperty-alignment: AlignCenter; }"); // Круглий фон
    } else {
        // Масштабуємо фото і робимо його круглим (якщо можливо)
        QPixmap scaledPixmap = photoPixmap.scaled(150, 150, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        // Створення круглої маски
        QBitmap mask(scaledPixmap.size());
        mask.fill(Qt::color0); // Прозорий фон
        QPainter painter(&mask);
        painter.setBrush(Qt::color1); // Непрозора маска
        painter.drawEllipse(0, 0, scaledPixmap.width(), scaledPixmap.height());
        painter.end();
        scaledPixmap.setMask(mask);
        photoLabel->setPixmap(scaledPixmap);
        photoLabel->setStyleSheet("QLabel { border-radius: 75px; }"); // Додатково для рамки, якщо потрібно
    }
    cardLayout->addWidget(photoLabel, 0, Qt::AlignHCenter); // Центруємо фото

    // 2. Ім'я та Прізвище (QLabel)
    QLabel *nameLabel = new QLabel(authorInfo.firstName + " " + authorInfo.lastName);
    nameLabel->setWordWrap(true);
    nameLabel->setAlignment(Qt::AlignCenter);
    nameLabel->setStyleSheet("QLabel { font-weight: bold; font-size: 11pt; margin-top: 5px; }");
    cardLayout->addWidget(nameLabel);

    // 3. Національність (QLabel - опціонально)
    if (!authorInfo.nationality.isEmpty()) {
        QLabel *nationalityLabel = new QLabel(authorInfo.nationality);
        nationalityLabel->setAlignment(Qt::AlignCenter);
        nationalityLabel->setStyleSheet("QLabel { color: #777; font-size: 9pt; }");
        cardLayout->addWidget(nationalityLabel);
    }

    // Додаємо розтягувач, щоб притиснути кнопку вниз (якщо вона буде)
    cardLayout->addStretch(1);

    // 4. Кнопка "Переглянути книги" (приклад)
    QPushButton *viewBooksButton = new QPushButton(tr("Переглянути книги"));
    viewBooksButton->setStyleSheet("QPushButton { background-color: #0078d4; color: white; border: none; border-radius: 8px; padding: 6px; font-size: 9pt; } QPushButton:hover { background-color: #106ebe; }"); // Збільшено border-radius
    viewBooksButton->setToolTip(tr("Переглянути книги автора %1 %2").arg(authorInfo.firstName, authorInfo.lastName));
    // connect(viewBooksButton, &QPushButton::clicked, this, [this, authorInfo](){ /* логіка перегляду книг автора */ });
    cardLayout->addWidget(viewBooksButton);


    cardFrame->setLayout(cardLayout);
    return cardFrame;
}

// Метод для відображення авторів у сітці
void MainWindow::displayAuthors(const QList<AuthorDisplayInfo> &authors)
{
    const int maxColumns = 5; // Кількість колонок (можна змінити)

    // Очищаємо попередні віджети з authorsContainerLayout (всередині pageAuthors)
     if (!ui->authorsContainerLayout) {
        qWarning() << "displayAuthors: authorsContainerLayout is null!";
        QLabel *errorLabel = new QLabel(tr("Помилка: Не вдалося знайти область для відображення авторів."), ui->authorsContainerWidget);
        errorLabel->setAlignment(Qt::AlignCenter);
        ui->authorsContainerWidget->setLayout(new QVBoxLayout());
        ui->authorsContainerWidget->layout()->addWidget(errorLabel);
        return;
    }
    clearLayout(ui->authorsContainerLayout);

    if (authors.isEmpty()) {
        QLabel *noAuthorsLabel = new QLabel(tr("Не вдалося завантажити авторів або їх немає в базі даних."), ui->authorsContainerWidget);
        noAuthorsLabel->setAlignment(Qt::AlignCenter);
        noAuthorsLabel->setWordWrap(true);
        ui->authorsContainerLayout->addWidget(noAuthorsLabel, 0, 0, 1, maxColumns); // Розтягнути на кілька колонок
        return;
    }

    int row = 0;
    int col = 0;
    // const int maxColumns = 5; // Перенесено на початок функції

    for (const AuthorDisplayInfo &authorInfo : authors) {
        QWidget *authorCard = createAuthorCardWidget(authorInfo);
        if (authorCard) {
            ui->authorsContainerLayout->addWidget(authorCard, row, col);
            col++;
            if (col >= maxColumns) {
                col = 0;
                row++;
            }
        }
    }

    // Встановлюємо розтягування колонок та додаємо розширювачі (аналогічно displayBooks)
    for (int c = 0; c < maxColumns; ++c) {
        ui->authorsContainerLayout->setColumnStretch(c, 1);
    }
    ui->authorsContainerLayout->setColumnStretch(maxColumns, 99);

    // Видаляємо старі розширювачі (про всяк випадок)
    QLayoutItem* itemV = ui->authorsContainerLayout->itemAtPosition(row + 1, 0);
    if (itemV && itemV->spacerItem()) { delete ui->authorsContainerLayout->takeAt(ui->authorsContainerLayout->indexOf(itemV)); }
    QLayoutItem* itemH = ui->authorsContainerLayout->itemAtPosition(0, maxColumns);
    if (itemH && itemH->spacerItem()) { delete ui->authorsContainerLayout->takeAt(ui->authorsContainerLayout->indexOf(itemH)); }

    // Додаємо нові розширювачі
    ui->authorsContainerLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum), 0, maxColumns);
    ui->authorsContainerLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding), row + 1, 0, 1, maxColumns);

    // Оновлюємо геометрію
    ui->authorsContainerWidget->updateGeometry();
    // ui->authorsScrollArea->updateGeometry(); // Зазвичай не потрібно
}

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

void MainWindow::on_navOrdersButton_clicked()
{
    ui->contentStackedWidget->setCurrentWidget(ui->ordersPage); // Переключаємо на сторінку замовлень
    loadAndDisplayOrders(); // Завантажуємо та відображаємо замовлення
}

// Слот для кнопки профілю в бічній панелі
void MainWindow::on_navProfileButton_clicked()
{
    qInfo() << "Navigating to profile page for customer ID:" << m_currentCustomerId;
    ui->contentStackedWidget->setCurrentWidget(ui->pageProfile);

    // Завантажуємо дані профілю при переході на сторінку
    if (m_currentCustomerId <= 0) {
        QMessageBox::warning(this, tr("Профіль користувача"), tr("Неможливо завантажити профіль, оскільки користувач не визначений."));
        populateProfilePanel(CustomerProfileInfo()); // Показати помилку в полях
        return;
    }
    if (!m_dbManager) {
         QMessageBox::critical(this, tr("Помилка"), tr("Помилка доступу до бази даних."));
         populateProfilePanel(CustomerProfileInfo()); // Показати помилку в полях
         return;
    }

    CustomerProfileInfo profile = m_dbManager->getCustomerProfileInfo(m_currentCustomerId);
    if (!profile.found) {
        QMessageBox::warning(this, tr("Профіль користувача"), tr("Не вдалося знайти інформацію для вашого профілю."));
    }
    populateProfilePanel(profile); // Заповнюємо сторінку профілю
    setProfileEditingEnabled(false); // Переконуємось, що режим редагування вимкнено при переході
}

// Слот для кнопки редагування профілю
void MainWindow::on_editProfileButton_clicked()
{
    setProfileEditingEnabled(true);
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
}


// Слот для відображення сторінки з деталями книги
void MainWindow::showBookDetails(int bookId)
{
    qInfo() << "Attempting to show details for book ID:" << bookId;
    if (bookId <= 0) {
        qWarning() << "Invalid book ID received:" << bookId;
        QMessageBox::warning(this, tr("Помилка"), tr("Некоректний ідентифікатор книги."));
        return;
    }
    if (!m_dbManager) {
        QMessageBox::critical(this, tr("Помилка"), tr("Помилка доступу до бази даних."));
        return;
    }
    if (!ui->bookDetailsPage) {
         QMessageBox::critical(this, tr("Помилка інтерфейсу"), tr("Сторінка деталей книги не знайдена."));
         return;
    }

    // Отримуємо деталі книги з бази даних
    BookDetailsInfo bookDetails = m_dbManager->getBookDetails(bookId);

    if (!bookDetails.found) {
        QMessageBox::warning(this, tr("Помилка"), tr("Не вдалося знайти інформацію для книги з ID %1.").arg(bookId));
        return;
    }

    // Заповнюємо сторінку даними
    populateBookDetailsPage(bookDetails);

    // Зберігаємо ID поточної книги
    m_currentBookDetailsId = bookId;

    // Переключаємо StackedWidget на сторінку деталей
    ui->contentStackedWidget->setCurrentWidget(ui->bookDetailsPage);
}


// Допоміжна функція для відображення списку коментарів
void MainWindow::displayComments(const QList<CommentDisplayInfo> &comments)
{
    // Очищаємо попередні коментарі та спейсер
    clearLayout(ui->commentsListLayout); // Використовуємо правильний layout

    if (comments.isEmpty()) {
        QLabel *noCommentsLabel = new QLabel(tr("Відгуків ще немає. Будьте першим!"));
        noCommentsLabel->setAlignment(Qt::AlignCenter);
        noCommentsLabel->setStyleSheet("color: #6c757d; font-style: italic; padding: 20px;");
        ui->commentsListLayout->addWidget(noCommentsLabel);
        // Додаємо спейсер, щоб мітка була по центру, якщо немає коментарів
        ui->commentsListLayout->addSpacerItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));
    } else {
        for (const CommentDisplayInfo &commentInfo : comments) {
            QWidget *commentWidget = createCommentWidget(commentInfo);
            if (commentWidget) {
                ui->commentsListLayout->addWidget(commentWidget);
            }
        }
        // Додаємо спейсер в кінці, щоб притиснути коментарі вгору
        ui->commentsListLayout->addSpacerItem(new QSpacerItem(20, 1, QSizePolicy::Minimum, QSizePolicy::Expanding));
    }
}

// Допоміжна функція для оновлення списку коментарів на сторінці деталей
void MainWindow::refreshBookComments()
{
    if (m_currentBookDetailsId <= 0 || !m_dbManager) {
        qWarning() << "Cannot refresh comments: invalid book ID or DB manager.";
        // Можна очистити список або показати помилку
        displayComments({});
        return;
    }
    qInfo() << "Refreshing comments for book ID:" << m_currentBookDetailsId;
    QList<CommentDisplayInfo> comments = m_dbManager->getBookComments(m_currentBookDetailsId);
    displayComments(comments);
}


// Допоміжна функція для створення віджету коментаря (оновлений дизайн)
QWidget* MainWindow::createCommentWidget(const CommentDisplayInfo &commentInfo)
{
    // Основний контейнер коментаря
    QFrame *commentFrame = new QFrame();
    commentFrame->setObjectName("commentFrame"); // Для стилізації
    commentFrame->setFrameShape(QFrame::StyledPanel);
    commentFrame->setFrameShadow(QFrame::Plain); // Використовуємо тінь через стиль
    commentFrame->setLineWidth(0); // Рамка через стиль
    // Стиль з тінню, заокругленням та відступами
    commentFrame->setStyleSheet(R"(
        QFrame#commentFrame {
            background-color: #ffffff; /* Білий фон */
            border: 1px solid #e9ecef; /* Світло-сіра рамка */
            border-radius: 8px; /* Більше заокруглення */
            padding: 15px; /* Збільшені відступи */
            margin-bottom: 10px; /* Відступ між коментарями */
            /* Можна додати тінь, але це може вплинути на продуктивність */
            /* box-shadow: 0 2px 4px rgba(0, 0, 0, 0.05); */
        }
    )");

    QVBoxLayout *mainLayout = new QVBoxLayout(commentFrame);
    mainLayout->setSpacing(8); // Збільшений відступ між елементами
    mainLayout->setContentsMargins(0, 0, 0, 0); // Відступи керуються padding у стилі фрейму

    // --- Верхній рядок: Автор та Дата ---
    QHBoxLayout *headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(10);

    // Ім'я автора (виділено)
    QLabel *authorLabel = new QLabel(commentInfo.authorName);
    authorLabel->setStyleSheet("font-weight: 600; font-size: 11pt; color: #343a40;"); // Жирний, трохи більший

    // Дата (менш помітна, праворуч)
    QLabel *dateLabel = new QLabel(QLocale::system().toString(commentInfo.commentDate, QLocale::ShortFormat));
    dateLabel->setStyleSheet("color: #868e96; font-size: 9pt;"); // Світліший сірий, менший шрифт
    dateLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    headerLayout->addWidget(authorLabel);
    headerLayout->addStretch(1); // Розтягувач між автором та датою
    headerLayout->addWidget(dateLabel);
    mainLayout->addLayout(headerLayout);

    mainLayout->addLayout(headerLayout); // Додаємо хедер

    // --- Рядок рейтингу (використовуємо StarRatingWidget) ---
    StarRatingWidget *ratingWidget = new StarRatingWidget();
    ratingWidget->setMaxRating(5);
    ratingWidget->setRating(commentInfo.rating > 0 ? commentInfo.rating : 0); // Встановлюємо рейтинг (0 якщо не було)
    ratingWidget->setReadOnly(true); // Тільки для відображення
    // Можна налаштувати розмір, якщо потрібно
    ratingWidget->setMinimumHeight(20);
    ratingWidget->setMaximumHeight(20);
    mainLayout->addWidget(ratingWidget); // Додаємо віджет рейтингу

    // --- Текст коментаря ---
    QLabel *commentTextLabel = new QLabel(commentInfo.commentText);
    commentTextLabel->setWordWrap(true); // Перенесення слів обов'язкове
    commentTextLabel->setStyleSheet("color: #495057; font-size: 10pt; line-height: 1.5;"); // Стандартний текст, міжрядковий інтервал
    mainLayout->addWidget(commentTextLabel);

    // Встановлюємо layout для фрейму
    commentFrame->setLayout(mainLayout);
    return commentFrame;
}


// Заповнення сторінки деталей книги даними
void MainWindow::populateBookDetailsPage(const BookDetailsInfo &details)
{
    // Перевірка існування віджетів на сторінці деталей (замінено bookDetailRatingLabel на bookDetailStarRatingWidget)
    if (!ui->bookDetailCoverLabel || !ui->bookDetailTitleLabel || !ui->bookDetailAuthorLabel ||
        !ui->bookDetailGenreLabel || !ui->bookDetailPublisherLabel || !ui->bookDetailYearLabel ||
        !ui->bookDetailPagesLabel || !ui->bookDetailIsbnLabel || !ui->bookDetailPriceLabel ||
        !ui->bookDetailDescriptionLabel || !ui->bookDetailAddToCartButton || !ui->bookDetailStarRatingWidget) // Додано перевірку нового віджета
    {
        qWarning() << "populateBookDetailsPage: One or more detail page widgets are null!";
        // Можна показати повідомлення про помилку на самій сторінці
        if(ui->bookDetailsPageLayout) {
            clearLayout(ui->bookDetailsPageLayout);
            QLabel *errorLabel = new QLabel(tr("Помилка інтерфейсу: Не вдалося відобразити деталі книги."), ui->bookDetailsPage);
            ui->bookDetailsPageLayout->addWidget(errorLabel);
        }
        return;
    }

    // 1. Обкладинка
    QPixmap coverPixmap(details.coverImagePath);
    if (coverPixmap.isNull() || details.coverImagePath.isEmpty()) {
        ui->bookDetailCoverLabel->setText(tr("Немає\nобкладинки"));
        ui->bookDetailCoverLabel->setStyleSheet("QLabel { background-color: #e0e0e0; color: #555; border: 1px solid #ccc; border-radius: 4px; }");
    } else {
        ui->bookDetailCoverLabel->setPixmap(coverPixmap.scaled(ui->bookDetailCoverLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        ui->bookDetailCoverLabel->setStyleSheet("QLabel { background-color: transparent; border: 1px solid #ccc; border-radius: 4px; }"); // Забираємо фон
    }

    // 2. Текстові поля
    ui->bookDetailTitleLabel->setText(details.title.isEmpty() ? tr("(Без назви)") : details.title);
    ui->bookDetailAuthorLabel->setText(details.authors.isEmpty() ? tr("(Автор невідомий)") : details.authors);
    ui->bookDetailGenreLabel->setText(tr("Жанр: %1").arg(details.genre.isEmpty() ? "-" : details.genre));
    ui->bookDetailPublisherLabel->setText(tr("Видавництво: %1").arg(details.publisherName.isEmpty() ? "-" : details.publisherName));
    ui->bookDetailYearLabel->setText(tr("Рік видання: %1").arg(details.publicationDate.isValid() ? QString::number(details.publicationDate.year()) : "-"));
    ui->bookDetailPagesLabel->setText(tr("Сторінок: %1").arg(details.pageCount > 0 ? QString::number(details.pageCount) : "-"));
    ui->bookDetailIsbnLabel->setText(tr("ISBN: %1").arg(details.isbn.isEmpty() ? "-" : details.isbn));
    ui->bookDetailPriceLabel->setText(QString::number(details.price, 'f', 2) + tr(" грн"));
    ui->bookDetailDescriptionLabel->setText(details.description.isEmpty() ? tr("(Опис відсутній)") : details.description); // Використовуємо setText для QLabel

    // 3. Кнопка "Додати в кошик" (можна додати логіку або сховати, якщо немає в наявності)
    ui->bookDetailAddToCartButton->setEnabled(details.stockQuantity > 0);
    ui->bookDetailAddToCartButton->setToolTip(details.stockQuantity > 0 ? tr("Додати '%1' до кошика").arg(details.title) : tr("Немає в наявності"));
    // TODO: Підключити сигнал кнопки до слота додавання в кошик

    // 4. Рейтинг (поки що просто текст)
    // 4. Рейтинг (використовуємо StarRatingWidget)
    int averageRating = 0; // За замовчуванням 0 зірок
    int ratedCount = 0;
    if (!details.comments.isEmpty()) {
        double totalRating = 0;
        for(const auto& comment : details.comments) {
            if (comment.rating > 0) {
                totalRating += comment.rating;
                ratedCount++;
            }
        }
        if (ratedCount > 0) {
            averageRating = qRound(totalRating / ratedCount); // Округлюємо до цілого
        }
    }
    // Встановлюємо рейтинг у віджет
    ui->bookDetailStarRatingWidget->setRating(averageRating);
    // Можна додати підказку з кількістю відгуків
    ui->bookDetailStarRatingWidget->setToolTip(tr("Середній рейтинг: %1 з 5 (%2 відгуків)")
                                                 .arg(averageRating)
                                                 .arg(ratedCount));


    // 5. Коментарі (використовуємо нову функцію)
    displayComments(details.comments);

    // 6. Налаштування секції додавання коментаря
    bool userHasCommented = false;
    bool canComment = (m_currentCustomerId > 0); // Чи може користувач взагалі коментувати (чи авторизований)

    if (canComment && m_dbManager) {
        userHasCommented = m_dbManager->hasUserCommentedOnBook(details.bookId, m_currentCustomerId);
    }

    // Отримуємо вказівники на віджети
    QLineEdit *commentEdit = ui->newCommentTextEdit;
    StarRatingWidget *ratingWidget = ui->newCommentStarRatingWidget;
    QPushButton *sendButton = ui->sendCommentButton;
    QLabel *alreadyCommentedLabel = ui->alreadyCommentedLabel;

    if (commentEdit && ratingWidget && sendButton && alreadyCommentedLabel) {
        if (!canComment) {
            // Користувач не авторизований
            commentEdit->setVisible(false);
            ratingWidget->setVisible(false);
            sendButton->setVisible(false);
            alreadyCommentedLabel->setText(tr("Будь ласка, увійдіть, щоб залишити відгук."));
            alreadyCommentedLabel->setVisible(true);
        } else if (userHasCommented) {
            // Користувач вже залишив відгук
            commentEdit->setVisible(false);
            ratingWidget->setVisible(false);
            sendButton->setVisible(false);
            alreadyCommentedLabel->setText(tr("Ви вже залишили відгук для цієї книги."));
            alreadyCommentedLabel->setVisible(true);
        } else {
            // Користувач може залишити відгук
            commentEdit->clear();
            ratingWidget->setRating(0);
            commentEdit->setVisible(true);
            ratingWidget->setVisible(true);
            sendButton->setVisible(true);
            alreadyCommentedLabel->setVisible(false); // Ховаємо мітку
        }
    } else {
        qWarning() << "populateBookDetailsPage: Could not find all comment input widgets!";
    }


    qInfo() << "Book details page populated for:" << details.title;
}


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

// Метод для створення віджету картки замовлення (Новий дизайн)
QWidget* MainWindow::createOrderWidget(const OrderDisplayInfo &orderInfo)
{
    // Основний віджет-контейнер для картки замовлення
    QFrame *orderCard = new QFrame();
    orderCard->setObjectName("orderCardWidget"); // Ім'я для застосування стилів з UI
    orderCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed); // Розширюється по ширині, фіксована висота

    // Головний горизонтальний layout картки
    QHBoxLayout *mainLayout = new QHBoxLayout(orderCard);
    mainLayout->setSpacing(15); // Відступ між основними блоками
    mainLayout->setContentsMargins(0, 0, 0, 0); // Відступи керуються стилем QFrame#orderCardWidget

    // --- Ліва частина: ID та Дата ---
    QVBoxLayout *infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(4); // Менший відступ між ID та датою

    QLabel *idLabel = new QLabel(tr("Замовлення №%1").arg(orderInfo.orderId));
    idLabel->setObjectName("orderIdLabel"); // Ім'я для стилів

    QString dateString = orderInfo.orderDate.isValid()
                         ? QLocale::system().toString(orderInfo.orderDate, "dd.MM.yyyy") // Тільки дата
                         : tr("(невідома дата)");
    QLabel *dateLabel = new QLabel(dateString);
    dateLabel->setObjectName("orderDateLabel"); // Ім'я для стилів

    infoLayout->addWidget(idLabel);
    infoLayout->addWidget(dateLabel);
    infoLayout->addStretch(1); // Притискає ID та дату вгору

    mainLayout->addLayout(infoLayout, 1); // Додаємо ліву частину (з розтягуванням)

    // --- Центральна частина: Сума ---
    QLabel *totalLabel = new QLabel(QString::number(orderInfo.totalAmount, 'f', 2) + tr(" грн"));
    totalLabel->setObjectName("orderTotalLabel"); // Ім'я для стилів
    totalLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter); // Вирівнювання по правому краю
    mainLayout->addWidget(totalLabel); // Додаємо суму

    // --- Права частина: Статус та Кнопка ---
    QVBoxLayout *statusLayout = new QVBoxLayout();
    statusLayout->setSpacing(8); // Відступ між статусом та кнопкою
    statusLayout->setAlignment(Qt::AlignRight); // Вирівнюємо вміст по правому краю

    QLabel *statusLabel = new QLabel();
    statusLabel->setObjectName("orderStatusLabel"); // Ім'я для стилів

    // Визначаємо текст та CSS-властивість статусу
    QString statusText = tr("Невідомо");
    QString statusCss = "unknown"; // Статус для CSS (має бути lowercase)
    if (!orderInfo.statuses.isEmpty()) {
        // Беремо останній статус
        statusText = orderInfo.statuses.last().status;
        // TODO: Перетворити статус з БД на відповідний CSS-клас (new, processing, shipped, delivered, cancelled)
        // Припускаємо, що вони співпадають після переведення в нижній регістр
        statusCss = statusText.toLower();
        // Приклад простого мапінгу (якщо назви відрізняються):
        // if (statusText == "В обробці") statusCss = "processing";
        // else if (statusText == "Відправлено") statusCss = "shipped";
        // ... і т.д.
    }
    statusLabel->setText(statusText);
    statusLabel->setProperty("status", statusCss); // Встановлюємо властивість для CSS
    statusLabel->ensurePolished(); // Застосовуємо стиль негайно

    // Кнопка "Деталі" (використовує стиль viewOrderDetailsButton з UI)
    QPushButton *detailsButton = new QPushButton(tr("Деталі"));
    detailsButton->setObjectName("viewOrderDetailsButton"); // Ім'я для стилів
    detailsButton->setCursor(Qt::PointingHandCursor);
    // TODO: Підключити сигнал кнопки до слота для показу деталей замовлення
    // connect(detailsButton, &QPushButton::clicked, this, [this, orderId = orderInfo.orderId]() {
    //     showOrderDetails(orderId); // Потрібно реалізувати showOrderDetails
    // });
    detailsButton->setToolTip(tr("Переглянути деталі замовлення №%1").arg(orderInfo.orderId));

    statusLayout->addWidget(statusLabel, 0, Qt::AlignRight); // Додаємо статус
    statusLayout->addWidget(detailsButton, 0, Qt::AlignRight); // Додаємо кнопку

    mainLayout->addLayout(statusLayout); // Додаємо праву частину

    orderCard->setLayout(mainLayout);
    return orderCard;
}

// Метод для відображення списку замовлень (Новий дизайн)
void MainWindow::displayOrders(const QList<OrderDisplayInfo> &orders)
{
    // Перевіряємо наявність необхідних віджетів
    if (!ui->ordersContentLayout || !ui->emptyOrdersLabel || !ui->ordersScrollArea) {
        qWarning() << "displayOrders: Required widgets (ordersContentLayout, emptyOrdersLabel, or ordersScrollArea) are null!";
        ui->statusBar->showMessage(tr("Помилка інтерфейсу: Не вдалося відобразити замовлення."), 5000);
        return;
    }

    // Очищаємо layout від попередніх карток замовлень
    clearLayout(ui->ordersContentLayout);

    bool isEmpty = orders.isEmpty();

    // Показуємо/ховаємо мітку про порожній список та область прокрутки
    // Додаємо додаткову перевірку перед використанням, щоб уникнути виключення
    if (ui->emptyOrdersLabel) {
        ui->emptyOrdersLabel->setVisible(isEmpty);
    } else {
        qWarning() << "displayOrders: emptyOrdersLabel was null during the 'if' check.";
    }
    // Аналогічна перевірка для ordersScrollArea (про всяк випадок)
    if (ui->ordersScrollArea) {
        ui->ordersScrollArea->setVisible(!isEmpty); // Ховаємо ScrollArea, якщо список порожній
    } else {
         qWarning() << "displayOrders: ordersScrollArea is unexpectedly null right before setVisible()!";
    }


    if (!isEmpty) {
        // Додаємо картки для кожного замовлення
        for (const OrderDisplayInfo &orderInfo : orders) {
            QWidget *orderCard = createOrderWidget(orderInfo); // Використовуємо оновлену функцію
            if (orderCard) {
                ui->ordersContentLayout->addWidget(orderCard);
            }
        }
        // Спейсер в кінці більше не потрібен, відступи керуються стилями карток
    }

    // Оновлюємо геометрію контейнера, щоб ScrollArea знала розмір
    ui->ordersContainerWidget->adjustSize();
    // Переконуємось, що ScrollArea оновилась, якщо вміст змінився
    QCoreApplication::processEvents(); // Даємо можливість обробити події перед прокруткою
    ui->ordersScrollArea->ensureVisible(0,0); // Прокручуємо до верху
}

// Метод для завантаження та відображення замовлень
void MainWindow::loadAndDisplayOrders()
{
    qInfo() << "Завантаження замовлень для customer ID:" << m_currentCustomerId;
    if (!m_dbManager) {
        qCritical() << "loadAndDisplayOrders: DatabaseManager is null!";
        QMessageBox::critical(this, tr("Помилка"), tr("Помилка доступу до бази даних."));
        displayOrders({}); // Показати порожній список з помилкою
        return;
    }
    if (m_currentCustomerId <= 0) {
        qWarning() << "loadAndDisplayOrders: Invalid customer ID:" << m_currentCustomerId;
        QMessageBox::warning(this, tr("Помилка"), tr("Неможливо завантажити замовлення, користувач не визначений."));
        displayOrders({}); // Показати порожній список з помилкою
        return;
    }

    // Отримуємо всі замовлення
    QList<OrderDisplayInfo> allOrders = m_dbManager->getCustomerOrdersForDisplay(m_currentCustomerId);
    qInfo() << "Завантажено" << allOrders.size() << "замовлень.";

    // Отримуємо вибраний статус та дату для фільтрації
    QString statusFilter = ui->orderStatusComboBox->currentText();
    QDate dateFilter = ui->orderDateEdit->date(); // Отримуємо дату з QDateEdit
    QList<OrderDisplayInfo> filteredOrders;

    for (const OrderDisplayInfo &order : allOrders) {
        bool statusMatch = false;
        bool dateMatch = false;

        // Перевірка статусу
        if (statusFilter == tr("Всі статуси")) {
            statusMatch = true;
        } else if (!order.statuses.isEmpty()) {
            // Порівнюємо останній статус замовлення з вибраним фільтром
            // Важливо: Переконайтесь, що рядки статусу з БД точно відповідають рядкам у ComboBox
            if (order.statuses.last().status == statusFilter) {
                statusMatch = true;
            }
        }

        // Перевірка дати (замовлення має бути створене НЕ РАНІШЕ вибраної дати)
        // Порівнюємо тільки дати, ігноруючи час
        if (!dateFilter.isNull() && order.orderDate.isValid()) {
             if (order.orderDate.date() >= dateFilter) {
                 dateMatch = true;
             }
        } else {
             // Якщо дата не вибрана (або дата замовлення невалідна), вважаємо, що дата підходить
             dateMatch = true;
        }


        // Додаємо замовлення, якщо воно відповідає обом фільтрам
        if (statusMatch && dateMatch) {
            filteredOrders.append(order);
        }
    }
    qInfo() << "Відфільтровано" << filteredOrders.size() << "замовлень за статусом:" << statusFilter << "та датою від:" << dateFilter.toString(Qt::ISODate);


    displayOrders(filteredOrders); // Відображаємо відфільтровані замовлення

    if (m_dbManager->lastError().isValid()) {
         ui->statusBar->showMessage(tr("Помилка при завантаженні замовлень: %1").arg(m_dbManager->lastError().text()), 5000);
    } else if (!filteredOrders.isEmpty()) { // Використовуємо відфільтрований список
         ui->statusBar->showMessage(tr("Замовлення успішно завантажено."), 3000);
    } else {
         // Якщо помилки не було, але замовлень 0 (після фільтрації), показуємо відповідне повідомлення
         ui->statusBar->showMessage(tr("У вас ще немає замовлень."), 3000);
    }
}


// Заповнення полів сторінки профілю даними
void MainWindow::populateProfilePanel(const CustomerProfileInfo &profileInfo)
{
    // Перевіряємо, чи вказівники на віджети існують (використовуємо нові LineEdit)
    if (!ui->profileFirstNameLineEdit || !ui->profileLastNameLineEdit || !ui->profileEmailLabel ||
        !ui->profilePhoneLineEdit || !ui->profileAddressLineEdit || !ui->profileJoinDateLabel ||
        !ui->profileLoyaltyLabel || !ui->profilePointsLabel)
    {
        qWarning() << "populateProfilePanel: One or more profile widgets are null!";
        // Не показуємо QMessageBox тут, щоб не заважати користувачу
        // Просто виходимо або встановлюємо текст помилки
        if(ui->pageProfile) { // Спробуємо показати помилку на самій сторінці
             clearLayout(ui->profilePageLayout); // Очистимо, щоб не було старих даних
             QLabel *errorLabel = new QLabel(tr("Помилка інтерфейсу: Не вдалося знайти поля для відображення профілю."), ui->pageProfile);
             errorLabel->setAlignment(Qt::AlignCenter);
             errorLabel->setWordWrap(true);
             ui->profilePageLayout->addWidget(errorLabel);
        }
        return;
    }

     // Перевіряємо, чи дані взагалі були знайдені
    if (!profileInfo.found || profileInfo.customerId <= 0) {
        // Заповнюємо поля текстом про помилку або відсутність даних (використовуємо нові LineEdit)
        const QString errorText = tr("(Помилка завантаження або дані відсутні)");
        ui->profileFirstNameLineEdit->setText("");
        ui->profileFirstNameLineEdit->setPlaceholderText(errorText);
        ui->profileFirstNameLineEdit->setEnabled(false);
        ui->profileLastNameLineEdit->setText("");
        ui->profileLastNameLineEdit->setPlaceholderText(errorText);
        ui->profileLastNameLineEdit->setEnabled(false);
        ui->profileEmailLabel->setText(errorText);
        ui->profilePhoneLineEdit->setText("");
        ui->profilePhoneLineEdit->setPlaceholderText(errorText);
        ui->profilePhoneLineEdit->setEnabled(false);
        ui->profileAddressLineEdit->setText("");
        ui->profileAddressLineEdit->setPlaceholderText(errorText);
        ui->profileAddressLineEdit->setEnabled(false);
        ui->profileJoinDateLabel->setText(errorText);
        ui->profileLoyaltyLabel->setText(errorText);
        ui->profilePointsLabel->setText("-");
        ui->saveProfileButton->setEnabled(false); // Блокуємо кнопку збереження
        return;
    }

    // Заповнюємо поля, використовуючи імена віджетів з mainwindow.ui (використовуємо нові LineEdit)
    ui->profileFirstNameLineEdit->setText(profileInfo.firstName);
    ui->profileFirstNameLineEdit->setPlaceholderText(tr("Введіть ім'я"));
    ui->profileFirstNameLineEdit->setEnabled(true);
    ui->profileLastNameLineEdit->setText(profileInfo.lastName);
    ui->profileLastNameLineEdit->setPlaceholderText(tr("Введіть прізвище"));
    ui->profileLastNameLineEdit->setEnabled(true);
    ui->profileEmailLabel->setText(profileInfo.email); // Email залишається QLabel
    ui->profilePhoneLineEdit->setText(profileInfo.phone);
    ui->profilePhoneLineEdit->setPlaceholderText(tr("Введіть номер телефону"));
    ui->profilePhoneLineEdit->setEnabled(true);
    ui->profileAddressLineEdit->setText(profileInfo.address);
    ui->profileAddressLineEdit->setPlaceholderText(tr("Введіть адресу"));
    ui->profileAddressLineEdit->setEnabled(true);
    ui->profileJoinDateLabel->setText(profileInfo.joinDate.isValid() ? profileInfo.joinDate.toString("dd.MM.yyyy") : tr("(невідомо)"));
    ui->profileLoyaltyLabel->setText(profileInfo.loyaltyProgram ? tr("Так") : tr("Ні"));
    ui->profilePointsLabel->setText(QString::number(profileInfo.loyaltyPoints));

    // Розблоковуємо кнопку збереження
    ui->saveProfileButton->setEnabled(true);

    // Поля, які не редагуються (Email, Дата реєстрації, Лояльність), можна зробити візуально неактивними
    ui->profileEmailLabel->setEnabled(false);
    ui->profileJoinDateLabel->setEnabled(false);
    ui->profileLoyaltyLabel->setEnabled(false);
    ui->profilePointsLabel->setEnabled(false);

    // Встановлюємо початковий стан редагування (зазвичай false)
    // setProfileEditingEnabled(false); // Перенесено в on_navProfileButton_clicked та конструктор
}


// Функція для ввімкнення/вимкнення режиму редагування профілю
void MainWindow::setProfileEditingEnabled(bool enabled)
{
    // Перевірка існування віджетів
    if (!ui->profileFirstNameLineEdit || !ui->profileLastNameLineEdit || !ui->profilePhoneLineEdit ||
        !ui->profileAddressLineEdit || !ui->editProfileButton || !ui->saveProfileButton)
    {
        qWarning() << "setProfileEditingEnabled: One or more profile widgets are null!";
        return;
    }

    // Вмикаємо/вимикаємо редагування полів
    ui->profileFirstNameLineEdit->setReadOnly(!enabled);
    ui->profileLastNameLineEdit->setReadOnly(!enabled);
    ui->profilePhoneLineEdit->setReadOnly(!enabled);
    ui->profileAddressLineEdit->setReadOnly(!enabled);

    // Показуємо/ховаємо кнопки
    ui->editProfileButton->setVisible(!enabled);
    ui->saveProfileButton->setVisible(enabled);

    // Змінюємо стиль полів для візуального розрізнення (опціонально)
    QString lineEditStyle = enabled
        ? "QLineEdit { background-color: white; border: 1px solid #86b7fe; }" // Стиль при редагуванні
        : "QLineEdit { background-color: #f8f9fa; border: 1px solid #dee2e6; }"; // Стиль при читанні
    ui->profileFirstNameLineEdit->setStyleSheet(lineEditStyle);
    ui->profileLastNameLineEdit->setStyleSheet(lineEditStyle);
    ui->profilePhoneLineEdit->setStyleSheet(lineEditStyle);
    ui->profileAddressLineEdit->setStyleSheet(lineEditStyle);

    // Встановлюємо фокус на перше поле при ввімкненні редагування
    if (enabled) {
        ui->profileFirstNameLineEdit->setFocus();
    }
}


// Слот для кнопки збереження змін у профілі
void MainWindow::on_saveProfileButton_clicked()
{
    qInfo() << "Attempting to save profile changes for customer ID:" << m_currentCustomerId;

    if (m_currentCustomerId <= 0) {
        QMessageBox::warning(this, tr("Збереження профілю"), tr("Неможливо зберегти зміни, користувач не визначений."));
        return;
    }
    if (!m_dbManager) {
         QMessageBox::critical(this, tr("Помилка"), tr("Помилка доступу до бази даних. Неможливо зберегти зміни."));
         return;
    }
    // Перевіряємо вказівники на нові LineEdit
    if (!ui->profileFirstNameLineEdit || !ui->profileLastNameLineEdit || !ui->profilePhoneLineEdit || !ui->profileAddressLineEdit) {
        QMessageBox::critical(this, tr("Помилка інтерфейсу"), tr("Не вдалося знайти одне або декілька полів профілю."));
        return;
    }

    // Отримуємо нові значення з полів
    QString newFirstName = ui->profileFirstNameLineEdit->text().trimmed();
    QString newLastName = ui->profileLastNameLineEdit->text().trimmed();
    QString newPhoneNumber = ui->profilePhoneLineEdit->text().trimmed();
    QString newAddress = ui->profileAddressLineEdit->text().trimmed();

    // Валідація (приклад)
    if (newFirstName.isEmpty()) {
        QMessageBox::warning(this, tr("Збереження профілю"), tr("Ім'я не може бути порожнім."));
        ui->profileFirstNameLineEdit->setFocus();
        return;
    }
    if (newLastName.isEmpty()) {
        QMessageBox::warning(this, tr("Збереження профілю"), tr("Прізвище не може бути порожнім."));
        ui->profileLastNameLineEdit->setFocus();
        return;
    }
    // TODO: Додати валідацію номера телефону та адреси

    // Викликаємо методи DatabaseManager для оновлення
    bool nameSuccess = m_dbManager->updateCustomerName(m_currentCustomerId, newFirstName, newLastName);
    bool phoneSuccess = m_dbManager->updateCustomerPhone(m_currentCustomerId, newPhoneNumber);
    bool addressSuccess = m_dbManager->updateCustomerAddress(m_currentCustomerId, newAddress);

    if (nameSuccess && phoneSuccess && addressSuccess) {
        ui->statusBar->showMessage(tr("Дані профілю успішно оновлено!"), 5000);
        qInfo() << "Profile data updated successfully for customer ID:" << m_currentCustomerId;
        setProfileEditingEnabled(false); // Вимикаємо режим редагування після успішного збереження
        // Можна перезавантажити дані, щоб переконатися, що все відображається коректно
        // CustomerProfileInfo profile = m_dbManager->getCustomerProfileInfo(m_currentCustomerId);
        // populateProfilePanel(profile); // Це оновить поля, але знову вимкне редагування
    } else {
        QString errorMessage = tr("Не вдалося оновити дані профілю:\n");
        if (!nameSuccess) errorMessage += tr("- Помилка оновлення імені/прізвища.\n");
        if (!phoneSuccess) errorMessage += tr("- Помилка оновлення телефону.\n");
        if (!addressSuccess) errorMessage += tr("- Помилка оновлення адреси.\n");
        errorMessage += tr("\nПеревірте журнал помилок.");
        QMessageBox::critical(this, tr("Помилка збереження"), errorMessage);
        qWarning() << "Failed to update profile data for customer ID:" << m_currentCustomerId << "Error:" << m_dbManager->lastError().text();
    }
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

// Метод для програмного встановлення банера
void MainWindow::setupBannerImage()
{
    if (!ui->bannerLabel) {
        qWarning() << "setupBannerImage: bannerLabel is null!";
        return;
    }

    // Будуємо шлях відносно директорії виконуваного файлу
    QString appDir = QCoreApplication::applicationDirPath();
    QString imagePath = QDir(appDir).filePath("images/banner.jpg"); // Правильний шлях до images/banner.jpg
    qInfo() << "Attempting to load banner from path:" << imagePath;
    qInfo() << "Banner label current size:" << ui->bannerLabel->size(); // Розмір віджета на момент виклику

    QPixmap bannerPixmap(imagePath);

    if (bannerPixmap.isNull()) {
        qWarning() << "Failed to load banner image. Check if the file exists at the specified path and is readable.";
        // Встановлюємо запасний варіант (наприклад, колір фону)
        ui->bannerLabel->setStyleSheet(ui->bannerLabel->styleSheet() + " background-color: #e9ecef;"); // Додаємо сірий фон
        ui->bannerLabel->setText(tr("Не вдалося завантажити банер\n(%1)").arg(imagePath)); // Показуємо шлях у повідомленні
    } else {
        qInfo() << "Banner image loaded successfully. Original size:" << bannerPixmap.size();
        // Перевіряємо розмір віджета ще раз, можливо він змінився
        QSize labelSize = ui->bannerLabel->size();
        if (!labelSize.isValid() || labelSize.width() <= 0 || labelSize.height() <= 0) {
            qWarning() << "Banner label size is invalid or zero:" << labelSize << ". Using minimum size hint:" << ui->bannerLabel->minimumSizeHint();
            labelSize = ui->bannerLabel->minimumSizeHint(); // Спробуємо використати minimumSizeHint
            if (!labelSize.isValid() || labelSize.width() <= 0 || labelSize.height() <= 0) {
                 qWarning() << "Minimum size hint is also invalid. Cannot scale pixmap correctly.";
                 ui->bannerLabel->setText(tr("Помилка розміру віджета банера"));
                 return;
            }
        }

        // Масштабуємо зображення, щоб воно заповнило QLabel, зберігаючи пропорції та обрізаючи зайве (як background-size: cover)
        QPixmap scaledPixmap = bannerPixmap.scaled(labelSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        qInfo() << "Pixmap scaled to (at least):" << scaledPixmap.size() << "based on label size:" << labelSize;

        // Обрізаємо зображення до розміру QLabel, якщо воно більше
        if (scaledPixmap.width() > labelSize.width() || scaledPixmap.height() > labelSize.height()) {
             scaledPixmap = scaledPixmap.copy(
                 (scaledPixmap.width() - labelSize.width()) / 2,
                 (scaledPixmap.height() - labelSize.height()) / 2,
                 labelSize.width(),
                 labelSize.height()
             );
        }
        ui->bannerLabel->setPixmap(scaledPixmap);
        // Переконуємось, що текст видно (стилі кольору/тіні залишаються з UI)
    }
}


// Слот для кнопки відправки коментаря
void MainWindow::on_sendCommentButton_clicked()
{
    qInfo() << "Send comment button clicked.";

    // Перевірки
    if (m_currentBookDetailsId <= 0) {
        QMessageBox::warning(this, tr("Помилка"), tr("Неможливо відправити відгук, не визначено книгу."));
        qWarning() << "Cannot send comment: m_currentBookDetailsId is invalid:" << m_currentBookDetailsId;
        return;
    }
    if (m_currentCustomerId <= 0) {
        QMessageBox::warning(this, tr("Помилка"), tr("Неможливо відправити відгук, користувач не авторизований."));
        qWarning() << "Cannot send comment: m_currentCustomerId is invalid:" << m_currentCustomerId;
        return;
    }
    if (!m_dbManager) {
        QMessageBox::critical(this, tr("Помилка"), tr("Помилка доступу до бази даних. Неможливо відправити відгук."));
        qWarning() << "Cannot send comment: m_dbManager is null.";
        return;
    }
    if (!ui->newCommentTextEdit || !ui->newCommentStarRatingWidget) {
         QMessageBox::critical(this, tr("Помилка інтерфейсу"), tr("Не знайдено поля для введення відгуку або рейтингу."));
         qWarning() << "Cannot send comment: UI elements missing.";
         return;
    }

    // Перевіряємо, чи користувач вже залишав коментар
    if (m_dbManager->hasUserCommentedOnBook(m_currentBookDetailsId, m_currentCustomerId)) {
        QMessageBox::information(this, tr("Відправка відгуку"), tr("Ви вже залишили відгук для цієї книги."));
        qWarning() << "Attempted to add a second comment for book ID:" << m_currentBookDetailsId << "by customer ID:" << m_currentCustomerId;
        // Оновлюємо UI на випадок, якщо він якось розсинхронізувався
        refreshBookComments(); // Оновлення списку може бути не потрібне, але оновимо UI додавання
        populateBookDetailsPage(m_dbManager->getBookDetails(m_currentBookDetailsId)); // Перезаповнюємо, щоб сховати поля
        return;
    }

    // Отримуємо дані з UI
    QString commentText = ui->newCommentTextEdit->text().trimmed(); // Використовуємо text() для QLineEdit
    int rating = ui->newCommentStarRatingWidget->rating();

    // Валідація
    if (commentText.isEmpty()) {
        QMessageBox::warning(this, tr("Відправка відгуку"), tr("Будь ласка, введіть текст вашого відгуку."));
        ui->newCommentTextEdit->setFocus();
        return;
    }

    // Викликаємо метод DatabaseManager
    qInfo() << "Attempting to add comment for book ID:" << m_currentBookDetailsId << "by customer ID:" << m_currentCustomerId << "Rating:" << rating;
    bool success = m_dbManager->addComment(m_currentBookDetailsId, m_currentCustomerId, commentText, rating);

    // Обробка результату
    if (success) {
        // ui->statusBar->showMessage(tr("Ваш відгук успішно додано!"), 4000); // Прибираємо повідомлення в статус-барі
        qInfo() << "Comment added successfully.";
        // Очищаємо поля введення та ховаємо їх (перезаповненням сторінки)
        // ui->newCommentTextEdit->clear(); // Більше не потрібно, бо перезаповнюємо
        // ui->newCommentStarRatingWidget->setRating(0); // Більше не потрібно
        ui->newCommentStarRatingWidget->setRating(0);
        // Оновлюємо всю сторінку деталей, щоб відобразити новий коментар,
        // оновити середній рейтинг та сховати поля для введення
        BookDetailsInfo updatedDetails = m_dbManager->getBookDetails(m_currentBookDetailsId);
        if (updatedDetails.found) {
            populateBookDetailsPage(updatedDetails);
        } else {
            // Якщо раптом книгу видалили, поки писали коментар
            qWarning() << "Book details not found after adding comment for ID:" << m_currentBookDetailsId;
            // Можна перейти на іншу сторінку або показати помилку
            ui->contentStackedWidget->setCurrentWidget(ui->booksPage);
        }

    } else {
        QMessageBox::critical(this, tr("Помилка відправки"), tr("Не вдалося додати ваш відгук. Перевірте журнал помилок або спробуйте пізніше."));
        qWarning() << "Failed to add comment. DB Error:" << m_dbManager->lastError().text();
    }
}

// Тимчасовий слот для кнопки "Деталі" замовлення
void MainWindow::showOrderDetailsPlaceholder(int orderId)
{
    qInfo() << "Details button clicked for order ID:" << orderId;
    QMessageBox::information(this,
                             tr("Деталі замовлення"),
                             tr("Тут будуть відображені деталі для замовлення №%1.").arg(orderId));
    // TODO: Реалізувати показ повноцінного діалогу з деталями замовлення
}


// --- Кінець реалізації слотів та функцій ---
