#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QLocale>
#include <QDateTime>
#include <QDebug>
#include <QMessageBox>
#include <QLineEdit> // Для on_sendCommentButton_clicked
#include <QPushButton> // Для on_sendCommentButton_clicked
#include "starratingwidget.h" // Для createCommentWidget, on_sendCommentButton_clicked
#include <QSpacerItem> // Для displayComments

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
