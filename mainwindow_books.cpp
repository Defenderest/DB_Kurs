#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QFrame>
#include <QPushButton>
#include <QPixmap>
#include <QDebug>
#include <QMessageBox>
#include <QMouseEvent>
#include <QGridLayout>
#include <QSpacerItem>
#include <QHBoxLayout>
#include "starratingwidget.h"
#include <QLineEdit>
#include <QScrollArea> // Додано для доступу до QScrollArea

QWidget* MainWindow::createBookCardWidget(const BookDisplayInfo &bookInfo)
{
    QFrame *cardFrame = new QFrame();
    cardFrame->setFrameShape(QFrame::StyledPanel);
    cardFrame->setFrameShadow(QFrame::Raised);
    cardFrame->setLineWidth(1);
    cardFrame->setMinimumSize(200, 300);
    cardFrame->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    cardFrame->setStyleSheet("QFrame { background-color: white; border-radius: 8px; }");

    QVBoxLayout *cardLayout = new QVBoxLayout(cardFrame);
    cardLayout->setSpacing(8);
    cardLayout->setContentsMargins(10, 10, 10, 10);

    QLabel *coverLabel = new QLabel();
    coverLabel->setAlignment(Qt::AlignCenter);
    coverLabel->setMinimumHeight(150);
    coverLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QPixmap coverPixmap;
    if (!bookInfo.coverImagePath.isEmpty()) {
        coverPixmap.load(bookInfo.coverImagePath);
    }

    if (coverPixmap.isNull()) {
        coverLabel->setText(tr("Немає\nобкладинки"));
        coverLabel->setStyleSheet("QLabel { background-color: #e0e0e0; color: #555; border-radius: 4px; }");
    } else {
        coverLabel->setPixmap(coverPixmap.scaled(180, 240, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        coverLabel->setStyleSheet("");
    }
    cardLayout->addWidget(coverLabel);

    QLabel *titleLabel = new QLabel(bookInfo.title);
    titleLabel->setWordWrap(true);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("QLabel { font-weight: bold; font-size: 11pt; }");
    cardLayout->addWidget(titleLabel);

    QLabel *authorLabel = new QLabel(bookInfo.authors.isEmpty() ? tr("Невідомий автор") : bookInfo.authors);
    authorLabel->setWordWrap(true);
    authorLabel->setAlignment(Qt::AlignCenter);
    authorLabel->setStyleSheet("QLabel { color: #555; font-size: 9pt; }");
    cardLayout->addWidget(authorLabel);

    QLabel *priceLabel = new QLabel(QString::number(bookInfo.price, 'f', 2) + tr(" грн"));
    priceLabel->setAlignment(Qt::AlignCenter);
    priceLabel->setStyleSheet("QLabel { font-weight: bold; color: #007bff; font-size: 10pt; margin-top: 5px; }");
    cardLayout->addWidget(priceLabel);

    cardLayout->addStretch(1);

    QPushButton *addToCartButton = new QPushButton(tr("🛒 Додати"));
    addToCartButton->setStyleSheet("QPushButton { background-color: #28a745; color: white; border: none; border-radius: 8px; padding: 8px; font-size: 9pt; } QPushButton:hover { background-color: #218838; }");
    addToCartButton->setToolTip(tr("Додати '%1' до кошика").arg(bookInfo.title));
    addToCartButton->setProperty("bookId", bookInfo.bookId);
    connect(addToCartButton, &QPushButton::clicked, this, [this, bookId = bookInfo.bookId](){
        on_addToCartButtonClicked(bookId);
    });
    cardLayout->addWidget(addToCartButton);

    cardFrame->setProperty("bookId", bookInfo.bookId);
    cardFrame->installEventFilter(this);
    cardFrame->setCursor(Qt::PointingHandCursor);


    cardFrame->setLayout(cardLayout);
    return cardFrame;
}


void MainWindow::displayBooks(const QList<BookDisplayInfo> &books, QGridLayout *targetLayout, QWidget *parentWidgetContext)
{
    if (!targetLayout) {
        qWarning() << "displayBooks: targetLayout is null!";
        return;
    }
    if (!parentWidgetContext) {
        qWarning() << "displayBooks: parentWidgetContext is null!";
        parentWidgetContext = targetLayout->parentWidget();
        if (!parentWidgetContext) {
            qWarning() << "displayBooks: Could not determine parent widget context!";
            return;
        }
    }

    int availableWidth = 0;
    QScrollArea* scrollArea = parentWidgetContext->parentWidget() ? qobject_cast<QScrollArea*>(parentWidgetContext->parentWidget()) : nullptr;
    if (scrollArea && scrollArea->viewport()) {
        availableWidth = scrollArea->viewport()->width();
        qDebug() << "displayBooks: Using viewport width:" << availableWidth;
    } else {
        availableWidth = parentWidgetContext->width();
        qWarning() << "displayBooks: Could not get scroll area viewport width, using parentWidgetContext width:" << availableWidth;
    }
    availableWidth -= (targetLayout->contentsMargins().left() + targetLayout->contentsMargins().right());

    const int cardMinWidth = 200;
    int hSpacing = targetLayout->horizontalSpacing();
    if (hSpacing < 0) hSpacing = 10; // Default spacing if not set

    int effectiveCardWidth = cardMinWidth + hSpacing;
    int numColumns = 1;
    if (effectiveCardWidth > 0 && availableWidth >= cardMinWidth) {
        numColumns = (availableWidth + hSpacing) / effectiveCardWidth;
    }
    numColumns = qMax(1, numColumns);
    qDebug() << "displayBooks: Calculated columns:" << numColumns << "(spacing:" << hSpacing << ", cardMinWidth:" << cardMinWidth << ", availableWidth:" << availableWidth << ")";

    clearLayout(targetLayout);

    if (books.isEmpty()) {
        QLabel *noBooksLabel = new QLabel(tr("Не вдалося завантажити книги або їх немає в базі даних."), parentWidgetContext);
        noBooksLabel->setAlignment(Qt::AlignCenter);
        noBooksLabel->setWordWrap(true);
        targetLayout->addWidget(noBooksLabel, 0, 0, 1, numColumns);
        targetLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding), 1, 0, 1, numColumns); // Vertical spacer spanning all columns
        return;
    }

    int row = 0;
    int col = 0;

    for (const BookDisplayInfo &bookInfo : books) {
        QWidget *bookCard = createBookCardWidget(bookInfo);
        if (bookCard) {
            targetLayout->addWidget(bookCard, row, col);
            col++;
            if (col >= numColumns) {
                col = 0;
                row++;
            }
        }
    }

    // Add horizontal spacer to push items to the left
    if (col > 0) { // If the last row is not full
         targetLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum), row, col, 1, numColumns - col);
    }
    // Add vertical spacer to push items to the top
    targetLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding), row + (col == 0 ? 0 : 1), 0, 1, numColumns);


    parentWidgetContext->updateGeometry();
}


void MainWindow::displayBooksInHorizontalLayout(const QList<BookDisplayInfo> &books, QHBoxLayout* layout)
{
    if (!layout) {
        qWarning() << "Target layout for horizontal display is null!";
        return;
    }
    clearLayout(layout);

    if (books.isEmpty()) {
        QLabel *noBooksLabel = new QLabel(tr("Для цього розділу книг не знайдено."));
        noBooksLabel->setAlignment(Qt::AlignCenter);
        noBooksLabel->setStyleSheet("QLabel { color: #777; font-style: italic; }");
        layout->addWidget(noBooksLabel, 1);
    } else {
        for (const BookDisplayInfo &bookInfo : books) {
            QWidget *bookCard = createBookCardWidget(bookInfo);
            if (bookCard) {
                bookCard->setMinimumWidth(180);
                bookCard->setMaximumWidth(220);
                layout->addWidget(bookCard);
            }
        }
        layout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum));
    }

    if (layout->parentWidget()) {
        layout->parentWidget()->updateGeometry();
    }
}

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

    BookDetailsInfo bookDetails = m_dbManager->getBookDetails(bookId);

    if (!bookDetails.found) {
        QMessageBox::warning(this, tr("Помилка"), tr("Не вдалося знайти інформацію для книги з ID %1.").arg(bookId));
        return;
    }

    populateBookDetailsPage(bookDetails);

    m_currentBookDetailsId = bookId;

    ui->contentStackedWidget->setCurrentWidget(ui->bookDetailsPage);
}

void MainWindow::populateBookDetailsPage(const BookDetailsInfo &details)
{
    if (!ui->bookDetailCoverLabel || !ui->bookDetailTitleLabel || !ui->bookDetailAuthorLabel ||
        !ui->bookDetailGenreLabel || !ui->bookDetailPublisherLabel || !ui->bookDetailYearLabel ||
        !ui->bookDetailPagesLabel || !ui->bookDetailIsbnLabel || !ui->bookDetailPriceLabel ||
        !ui->bookDetailDescriptionLabel || !ui->bookDetailAddToCartButton || !ui->bookDetailStarRatingWidget)
    {
        qWarning() << "populateBookDetailsPage: One or more detail page widgets are null!";
        if(ui->bookDetailsPageLayout) {
            clearLayout(ui->bookDetailsPageLayout);
            QLabel *errorLabel = new QLabel(tr("Помилка інтерфейсу: Не вдалося відобразити деталі книги."), ui->bookDetailsPage);
            ui->bookDetailsPageLayout->addWidget(errorLabel);
        }
        return;
    }

    QPixmap coverPixmap(details.coverImagePath);
    if (coverPixmap.isNull() || details.coverImagePath.isEmpty()) {
        ui->bookDetailCoverLabel->setText(tr("Немає\nобкладинки"));
        ui->bookDetailCoverLabel->setStyleSheet("QLabel { background-color: #e0e0e0; color: #555; border: 1px solid #ccc; border-radius: 4px; }");
    } else {
        ui->bookDetailCoverLabel->setPixmap(coverPixmap.scaled(ui->bookDetailCoverLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        ui->bookDetailCoverLabel->setStyleSheet("QLabel { background-color: transparent; border: 1px solid #ccc; border-radius: 4px; }");
    }

    ui->bookDetailTitleLabel->setText(details.title.isEmpty() ? tr("(Без назви)") : details.title);
    ui->bookDetailAuthorLabel->setText(details.authors.isEmpty() ? tr("(Автор невідомий)") : details.authors);
    ui->bookDetailGenreLabel->setText(tr("Жанр: %1").arg(details.genre.isEmpty() ? "-" : details.genre));
    ui->bookDetailPublisherLabel->setText(tr("Видавництво: %1").arg(details.publisherName.isEmpty() ? "-" : details.publisherName));
    ui->bookDetailYearLabel->setText(tr("Рік видання: %1").arg(details.publicationDate.isValid() ? QString::number(details.publicationDate.year()) : "-"));
    ui->bookDetailPagesLabel->setText(tr("Сторінок: %1").arg(details.pageCount > 0 ? QString::number(details.pageCount) : "-"));
    ui->bookDetailIsbnLabel->setText(tr("ISBN: %1").arg(details.isbn.isEmpty() ? "-" : details.isbn));
    ui->bookDetailPriceLabel->setText(QString::number(details.price, 'f', 2) + tr(" грн"));
    ui->bookDetailDescriptionLabel->setText(details.description.isEmpty() ? tr("(Опис відсутній)") : details.description);

    ui->bookDetailAddToCartButton->setEnabled(details.stockQuantity > 0);
    ui->bookDetailAddToCartButton->setToolTip(details.stockQuantity > 0 ? tr("Додати '%1' до кошика").arg(details.title) : tr("Немає в наявності"));
    disconnect(ui->bookDetailAddToCartButton, &QPushButton::clicked, nullptr, nullptr);
    connect(ui->bookDetailAddToCartButton, &QPushButton::clicked, this, [this, bookId = details.bookId](){
        on_addToCartButtonClicked(bookId);
    });

    int averageRating = 0;
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
            averageRating = qRound(totalRating / ratedCount);
        }
    }
    ui->bookDetailStarRatingWidget->setRating(averageRating);
    ui->bookDetailStarRatingWidget->setToolTip(tr("Середній рейтинг: %1 з 5 (%2 відгуків)")
                                                 .arg(averageRating)
                                                 .arg(ratedCount));

    displayComments(details.comments);

    bool userHasCommented = false;
    bool canComment = (m_currentCustomerId > 0);

    if (canComment && m_dbManager) {
        userHasCommented = m_dbManager->hasUserCommentedOnBook(details.bookId, m_currentCustomerId);
    }

    QLineEdit *commentEdit = ui->newCommentTextEdit;
    StarRatingWidget *ratingWidget = ui->newCommentStarRatingWidget;
    QPushButton *sendButton = ui->sendCommentButton;
    QLabel *alreadyCommentedLabel = ui->alreadyCommentedLabel;

    if (commentEdit && ratingWidget && sendButton && alreadyCommentedLabel) {
        if (!canComment) {
            commentEdit->setVisible(false);
            ratingWidget->setVisible(false);
            sendButton->setVisible(false);
            alreadyCommentedLabel->setText(tr("Будь ласка, увійдіть, щоб залишити відгук."));
            alreadyCommentedLabel->setVisible(true);
        } else if (userHasCommented) {
            commentEdit->setVisible(false);
            ratingWidget->setVisible(false);
            sendButton->setVisible(false);
            alreadyCommentedLabel->setText(tr("Ви вже залишили відгук для цієї книги."));
            alreadyCommentedLabel->setVisible(true);
        } else {
            commentEdit->clear();
            ratingWidget->setRating(0);
            commentEdit->setVisible(true);
            ratingWidget->setVisible(true);
            sendButton->setVisible(true);
            alreadyCommentedLabel->setVisible(false);
        }
    } else {
        qWarning() << "populateBookDetailsPage: Could not find all comment input widgets!";
    }

    if (ui->similarBooksWidget && ui->similarBooksLayout) {
        if (!details.genre.isEmpty() && m_dbManager) {
            QList<BookDisplayInfo> similarBooks = m_dbManager->getSimilarBooks(details.bookId, details.genre, 5);

            if (!similarBooks.isEmpty()) {
                clearLayout(ui->similarBooksLayout);
                displayBooksInHorizontalLayout(similarBooks, ui->similarBooksLayout);
                ui->similarBooksWidget->setVisible(true);
                qInfo() << "Displayed" << similarBooks.count() << "similar books.";
            } else {
                clearLayout(ui->similarBooksLayout);
                ui->similarBooksWidget->setVisible(false);
                qInfo() << "No similar books found for genre:" << details.genre;
            }
        } else {
            clearLayout(ui->similarBooksLayout);
            ui->similarBooksWidget->setVisible(false);
            qWarning() << "Cannot fetch similar books: Genre is empty or dbManager is null.";
        }
    } else {
        qWarning() << "populateBookDetailsPage: similarBooksWidget or similarBooksLayout pointers are null! Cannot display similar books.";
        if(ui->similarBooksWidget) ui->similarBooksWidget->setVisible(false);
    }

    qInfo() << "Book details page populated for:" << details.title;
}
