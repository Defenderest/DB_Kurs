#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QFrame>
#include <QPushButton>
#include <QPixmap>
#include <QDebug>
#include <QMessageBox>
#include <QMouseEvent> // Для eventFilter
#include <QGridLayout> // Для displayBooks
#include <QSpacerItem> // Для displayBooks
#include <QHBoxLayout> // Для displayBooksInHorizontalLayout
#include "starratingwidget.h" // Для populateBookDetailsPage
#include <QLineEdit> // Для populateBookDetailsPage (newCommentTextEdit)

// Метод для створення віджету картки книги
QWidget* MainWindow::createBookCardWidget(const BookDisplayInfo &bookInfo)
{
    // Основний віджет картки (використовуємо QFrame для рамки)
    QFrame *cardFrame = new QFrame();
    cardFrame->setFrameShape(QFrame::StyledPanel); // Додає рамку
    cardFrame->setFrameShadow(QFrame::Raised);     // Додає тінь
    cardFrame->setLineWidth(1);
    cardFrame->setMinimumSize(200, 300); // Мінімальний розмір картки (повернено)
    // cardFrame->setMaximumSize(250, 350); // Максимальний розмір можна прибрати або залишити
    cardFrame->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred); // Дозволяємо розтягуватись в комірці
    cardFrame->setStyleSheet("QFrame { background-color: white; border-radius: 8px; }"); // Повернено початковий стиль картки

    // Вертикальний layout для вмісту картки
    QVBoxLayout *cardLayout = new QVBoxLayout(cardFrame);
    cardLayout->setSpacing(8); // Повернено початковий spacing
    cardLayout->setContentsMargins(10, 10, 10, 10); // Повернено початкові margins

    // 1. Обкладинка книги (QLabel)
    QLabel *coverLabel = new QLabel();
    coverLabel->setAlignment(Qt::AlignCenter);
    coverLabel->setMinimumHeight(150); // Повернено початкову мінімальну висоту
    coverLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding); // Розтягувати
    QPixmap coverPixmap; // Створюємо Pixmap
    // Завантажуємо, тільки якщо шлях не порожній
    if (!bookInfo.coverImagePath.isEmpty()) {
        coverPixmap.load(bookInfo.coverImagePath);
    }

    if (coverPixmap.isNull()) { // Перевіряємо, чи завантажилось (або шлях був порожній)
        // Якщо зображення не завантажилось, показуємо плейсхолдер
        coverLabel->setText(tr("Немає\nобкладинки"));
        coverLabel->setStyleSheet("QLabel { background-color: #e0e0e0; color: #555; border-radius: 4px; }"); // Повернено стиль плейсхолдера
    } else {
        // Масштабуємо завантажене зображення, зберігаючи пропорції
        coverLabel->setPixmap(coverPixmap.scaled(180, 240, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        coverLabel->setStyleSheet(""); // Прибираємо зайвий стиль
    }
    cardLayout->addWidget(coverLabel);

    // 2. Назва книги (QLabel)
    QLabel *titleLabel = new QLabel(bookInfo.title);
    titleLabel->setWordWrap(true); // Переносити текст
    titleLabel->setAlignment(Qt::AlignCenter); // Повернено центрування
    titleLabel->setStyleSheet("QLabel { font-weight: bold; font-size: 11pt; }"); // Повернено початковий стиль
    cardLayout->addWidget(titleLabel);

    // 3. Автор(и) (QLabel)
    QLabel *authorLabel = new QLabel(bookInfo.authors.isEmpty() ? tr("Невідомий автор") : bookInfo.authors);
    authorLabel->setWordWrap(true);
    authorLabel->setAlignment(Qt::AlignCenter); // Повернено центрування
    authorLabel->setStyleSheet("QLabel { color: #555; font-size: 9pt; }"); // Повернено початковий стиль
    cardLayout->addWidget(authorLabel);

    // 4. Ціна (QLabel)
    QLabel *priceLabel = new QLabel(QString::number(bookInfo.price, 'f', 2) + tr(" грн"));
    priceLabel->setAlignment(Qt::AlignCenter); // Повернено центрування
    priceLabel->setStyleSheet("QLabel { font-weight: bold; color: #007bff; font-size: 10pt; margin-top: 5px; }"); // Повернено початковий стиль
    cardLayout->addWidget(priceLabel);

    // Додаємо розтягувач, щоб притиснути кнопку вниз
    cardLayout->addStretch(1);

    // 5. Кнопка "Додати в кошик" (QPushButton - повернено початковий вигляд)
    QPushButton *addToCartButton = new QPushButton(tr("🛒 Додати")); // Повернено текст
    addToCartButton->setStyleSheet("QPushButton { background-color: #28a745; color: white; border: none; border-radius: 8px; padding: 8px; font-size: 9pt; } QPushButton:hover { background-color: #218838; }"); // Повернено стиль
    addToCartButton->setToolTip(tr("Додати '%1' до кошика").arg(bookInfo.title));
    // Зберігаємо bookId як властивість кнопки для легкого доступу в слоті
    addToCartButton->setProperty("bookId", bookInfo.bookId);
    // Підключаємо сигнал кнопки до слота on_addToCartButtonClicked
    connect(addToCartButton, &QPushButton::clicked, this, [this, bookId = bookInfo.bookId](){
        on_addToCartButtonClicked(bookId);
    });
    cardLayout->addWidget(addToCartButton); // Додаємо кнопку напряму в cardLayout

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
void MainWindow::displayBooks(const QList<BookDisplayInfo> &books, QGridLayout *targetLayout, QWidget *parentWidgetContext)
{
    // Перевіряємо валідність вказівників
    if (!targetLayout) {
        qWarning() << "displayBooks: targetLayout is null!";
        return;
    }
    if (!parentWidgetContext) {
        qWarning() << "displayBooks: parentWidgetContext is null!";
        // Можна спробувати отримати батьківський віджет з layout, але це менш надійно
        parentWidgetContext = targetLayout->parentWidget();
        if (!parentWidgetContext) {
            qWarning() << "displayBooks: Could not determine parent widget context!";
            return;
        }
    }

    // Очищаємо попередні віджети з цільового layout
    clearLayout(targetLayout);

    if (books.isEmpty()) {
        QLabel *noBooksLabel = new QLabel(tr("Не вдалося завантажити книги або їх немає в базі даних."), parentWidgetContext);
        noBooksLabel->setAlignment(Qt::AlignCenter);
        noBooksLabel->setWordWrap(true);
        // Додаємо мітку безпосередньо в layout
        targetLayout->addWidget(noBooksLabel, 0, 0, 1, maxColumns); // Розтягнути на кілька колонок
        // Додаємо спейсери, щоб мітка була по центру і не розтягувала все
        targetLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding), 1, 0); // Вертикальний
        targetLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum), 0, maxColumns); // Горизонтальний
        return; // Виходимо, якщо книг немає
    }

    int row = 0;
    int col = 0;

    for (const BookDisplayInfo &bookInfo : books) {
        QWidget *bookCard = createBookCardWidget(bookInfo);
        if (bookCard) {
            targetLayout->addWidget(bookCard, row, col);
            col++;
            if (col >= maxColumns) {
                col = 0;
                row++;
            }
        }
    }

    // Встановлюємо однакове розтягування для колонок з картками
    for (int c = 0; c < maxColumns; ++c) {
        targetLayout->setColumnStretch(c, 1);
    }
    // Встановлюємо розтягування для колонки після карток (де горизонтальний спейсер)
    targetLayout->setColumnStretch(maxColumns, 99); // Велике значення, щоб забрати весь зайвий простір

    // Видаляємо попередні розширювачі, якщо вони були додані раніше (про всяк випадок)
    // (Це може бути не потрібно, якщо clearLayout працює коректно, але залишаємо для надійності)
    QLayoutItem* item;
    // Видаляємо вертикальний розширювач знизу (якщо він є)
    item = targetLayout->itemAtPosition(row + 1, 0);
    if (item && item->spacerItem()) {
        targetLayout->removeItem(item);
        delete item;
    }
     // Видаляємо горизонтальний розширювач справа (якщо він є)
    item = targetLayout->itemAtPosition(0, maxColumns);
     if (item && item->spacerItem()) {
        targetLayout->removeItem(item);
        delete item;
    }

    // Додаємо горизонтальний розширювач в першому рядку після останньої колонки карток,
    // щоб притиснути картки вліво.
    targetLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum), 0, maxColumns);
    // Додаємо вертикальний розширювач під останнім рядком карток,
    // щоб притиснути картки вгору.
    targetLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding), row + 1, 0, 1, maxColumns);


    // Переконуємося, що контейнер оновився
    parentWidgetContext->updateGeometry();
    // Якщо parentWidgetContext знаходиться всередині ScrollArea, можливо, знадобиться оновити і її
    // QScrollArea *scrollArea = qobject_cast<QScrollArea*>(parentWidgetContext->parentWidget());
    // if (scrollArea) scrollArea->updateGeometry();
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
    // Підключаємо сигнал кнопки до слота додавання в кошик
    // Спочатку від'єднуємо попередні з'єднання, щоб уникнути дублікатів
    disconnect(ui->bookDetailAddToCartButton, &QPushButton::clicked, nullptr, nullptr);
    connect(ui->bookDetailAddToCartButton, &QPushButton::clicked, this, [this, bookId = details.bookId](){
        on_addToCartButtonClicked(bookId);
    });

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

    // 7. Схожі книги (додано)
    // Перевіряємо наявність віджетів для схожих книг
    if (ui->similarBooksWidget && ui->similarBooksLayout) { // Перевірка залишається корисною
        if (!details.genre.isEmpty() && m_dbManager) {
            QList<BookDisplayInfo> similarBooks = m_dbManager->getSimilarBooks(details.bookId, details.genre, 5); // Отримуємо до 5 схожих книг

            if (!similarBooks.isEmpty()) {
                // Очищаємо попередній вміст layout для схожих книг
                clearLayout(ui->similarBooksLayout);

                // Додаємо заголовок секції (можна створити QLabel програмно або використати існуючий)
                // Наприклад, якщо є ui->similarBooksTitleLabel:
                // ui->similarBooksTitleLabel->setText(tr("Схожі книги"));
                // ui->similarBooksTitleLabel->setVisible(true);

                // Відображаємо книги у горизонтальному layout
                displayBooksInHorizontalLayout(similarBooks, ui->similarBooksLayout);
                ui->similarBooksWidget->setVisible(true); // Показуємо контейнер
                qInfo() << "Displayed" << similarBooks.count() << "similar books.";
            } else {
                // Якщо схожих книг немає, ховаємо секцію
                clearLayout(ui->similarBooksLayout); // Очищаємо layout на випадок, якщо там було повідомлення
                ui->similarBooksWidget->setVisible(false);
                qInfo() << "No similar books found for genre:" << details.genre;
            }
        } else {
            // Якщо жанр невідомий або немає dbManager, ховаємо секцію
            clearLayout(ui->similarBooksLayout);
            ui->similarBooksWidget->setVisible(false);
            qWarning() << "Cannot fetch similar books: Genre is empty or dbManager is null.";
        }
    } else {
        // Цей блок тепер менш ймовірний, але залишаємо попередження про всяк випадок
        qWarning() << "populateBookDetailsPage: similarBooksWidget or similarBooksLayout pointers are null! Cannot display similar books.";
        // Якщо сам віджет існує, але вказівник чомусь null, спробуємо його сховати
        if(ui->similarBooksWidget) ui->similarBooksWidget->setVisible(false);
    }

    qInfo() << "Book details page populated for:" << details.title;
}
