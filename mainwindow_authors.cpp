#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QFrame>
#include <QVBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QBitmap>
#include <QPainter>
#include <QPushButton>
#include <QDebug>
#include <QGridLayout>
#include <QSpacerItem>
#include <QScrollArea> // Додано для доступу до QScrollArea

QWidget* MainWindow::createAuthorCardWidget(const AuthorDisplayInfo &authorInfo)
{
    QFrame *cardFrame = new QFrame();
    cardFrame->setFrameShape(QFrame::StyledPanel);
    cardFrame->setFrameShadow(QFrame::Raised);
    cardFrame->setLineWidth(1);
    cardFrame->setMinimumSize(180, 250); // Використовуємо цю мінімальну ширину для розрахунків
    cardFrame->setMaximumSize(220, 280);
    cardFrame->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    cardFrame->setStyleSheet("QFrame { background-color: white; border-radius: 8px; }");

    QVBoxLayout *cardLayout = new QVBoxLayout(cardFrame);
    cardLayout->setSpacing(6);
    cardLayout->setContentsMargins(10, 10, 10, 10);

    QLabel *photoLabel = new QLabel();
    photoLabel->setAlignment(Qt::AlignCenter);
    photoLabel->setMinimumSize(150, 150);
    photoLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    QPixmap photoPixmap(authorInfo.imagePath);
    if (photoPixmap.isNull() || authorInfo.imagePath.isEmpty()) {
        photoLabel->setText(tr("👤"));
        photoLabel->setStyleSheet("QLabel { background-color: #e0e0e0; color: #555; border-radius: 75px; font-size: 80pt; qproperty-alignment: AlignCenter; }");
    } else {
        QPixmap scaledPixmap = photoPixmap.scaled(150, 150, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        QBitmap mask(scaledPixmap.size());
        mask.fill(Qt::color0);
        QPainter painter(&mask);
        painter.setBrush(Qt::color1);
        painter.drawEllipse(0, 0, scaledPixmap.width(), scaledPixmap.height());
        painter.end();
        scaledPixmap.setMask(mask);
        photoLabel->setPixmap(scaledPixmap);
        photoLabel->setStyleSheet("QLabel { border-radius: 75px; }");
    }
    cardLayout->addWidget(photoLabel, 0, Qt::AlignHCenter);

    QLabel *nameLabel = new QLabel(authorInfo.firstName + " " + authorInfo.lastName);
    nameLabel->setWordWrap(true);
    nameLabel->setAlignment(Qt::AlignCenter);
    nameLabel->setStyleSheet("QLabel { font-weight: bold; font-size: 11pt; margin-top: 5px; }");
    cardLayout->addWidget(nameLabel);

    if (!authorInfo.nationality.isEmpty()) {
        QLabel *nationalityLabel = new QLabel(authorInfo.nationality);
        nationalityLabel->setAlignment(Qt::AlignCenter);
        nationalityLabel->setStyleSheet("QLabel { color: #777; font-size: 9pt; }");
        cardLayout->addWidget(nationalityLabel);
    }

    cardLayout->addStretch(1);

    QPushButton *viewBooksButton = new QPushButton(tr("Переглянути книги"));
    viewBooksButton->setStyleSheet("QPushButton { background-color: #0078d4; color: white; border: none; border-radius: 8px; padding: 6px; font-size: 9pt; } QPushButton:hover { background-color: #106ebe; }");
    viewBooksButton->setToolTip(tr("Переглянути книги автора %1 %2").arg(authorInfo.firstName, authorInfo.lastName));

    cardLayout->addWidget(viewBooksButton);

    cardFrame->setProperty("authorId", authorInfo.authorId);
    cardFrame->installEventFilter(this);
    cardFrame->setCursor(Qt::PointingHandCursor);


    cardFrame->setLayout(cardLayout);
    return cardFrame;
}

void MainWindow::displayAuthors(const QList<AuthorDisplayInfo> &authors)
{
    if (!ui->authorsContainerLayout || !ui->authorsContainerWidget) {
        qWarning() << "displayAuthors: authorsContainerLayout or authorsContainerWidget is null!";
        if (ui->authorsContainerWidget) {
             QLabel *errorLabel = new QLabel(tr("Помилка: Не вдалося знайти область для відображення авторів."), ui->authorsContainerWidget);
             errorLabel->setAlignment(Qt::AlignCenter);
             if (!ui->authorsContainerWidget->layout()) {
                 ui->authorsContainerWidget->setLayout(new QVBoxLayout());
             }
             clearLayout(ui->authorsContainerWidget->layout());
             ui->authorsContainerWidget->layout()->addWidget(errorLabel);
        }
        return;
    }

    clearLayout(ui->authorsContainerLayout);

    int availableWidth = 0;
    QScrollArea* scrollArea = ui->authorsContainerWidget->parentWidget() ? qobject_cast<QScrollArea*>(ui->authorsContainerWidget->parentWidget()) : nullptr;
    if (scrollArea && scrollArea->viewport()) {
        availableWidth = scrollArea->viewport()->width();
        qDebug() << "displayAuthors: Using viewport width:" << availableWidth;
    } else {
        availableWidth = ui->authorsContainerWidget->width();
        qWarning() << "displayAuthors: Could not get scroll area viewport width, using authorsContainerWidget width:" << availableWidth;
    }
    availableWidth -= (ui->authorsContainerLayout->contentsMargins().left() + ui->authorsContainerLayout->contentsMargins().right());

    const int cardMinWidth = 180; // Використовуємо мінімальну ширину картки автора
    int hSpacing = ui->authorsContainerLayout->horizontalSpacing();
    if (hSpacing < 0) hSpacing = 10; // Default spacing if not set

    int effectiveCardWidth = cardMinWidth + hSpacing;
    int numColumns = 1;
    if (effectiveCardWidth > 0 && availableWidth >= cardMinWidth) {
        numColumns = (availableWidth + hSpacing) / effectiveCardWidth;
    }
    numColumns = qMax(1, numColumns);
    qDebug() << "displayAuthors: Calculated columns:" << numColumns << "(spacing:" << hSpacing << ", cardMinWidth:" << cardMinWidth << ", availableWidth:" << availableWidth << ")";


    if (authors.isEmpty()) {
        QLabel *noAuthorsLabel = new QLabel(tr("Не вдалося завантажити авторів або їх немає в базі даних."), ui->authorsContainerWidget);
        noAuthorsLabel->setAlignment(Qt::AlignCenter);
        noAuthorsLabel->setWordWrap(true);
        ui->authorsContainerLayout->addWidget(noAuthorsLabel, 0, 0, 1, numColumns);
        ui->authorsContainerLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding), 1, 0, 1, numColumns); // Vertical spacer spanning all columns
        return;
    }

    int row = 0;
    int col = 0;

    for (const AuthorDisplayInfo &authorInfo : authors) {
        QWidget *authorCard = createAuthorCardWidget(authorInfo);
        if (authorCard) {
            ui->authorsContainerLayout->addWidget(authorCard, row, col);
            col++;
            if (col >= numColumns) {
                col = 0;
                row++;
            }
        }
    }

    // Remove old stretch settings
    for (int c = 0; c < ui->authorsContainerLayout->columnCount(); ++c) {
        ui->authorsContainerLayout->setColumnStretch(c, 0);
    }

    // Add horizontal spacer to push items to the left
    if (col > 0) { // If the last row is not full
         ui->authorsContainerLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum), row, col, 1, numColumns - col);
    }
    // Add vertical spacer to push items to the top
    ui->authorsContainerLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding), row + (col == 0 ? 0 : 1), 0, 1, numColumns);


    ui->authorsContainerWidget->updateGeometry();
}

void MainWindow::loadAndDisplayAuthors()
{
    if (!m_dbManager || !m_dbManager->isConnected()) {
        qWarning() << "loadAndDisplayAuthors: Database is not connected.";
        if (ui->authorsContainerWidget && ui->authorsContainerLayout) {
             QLabel *errorLabel = new QLabel(tr("Не вдалося підключитися до бази даних для завантаження авторів."), ui->authorsContainerWidget);
             clearLayout(ui->authorsContainerLayout);
             ui->authorsContainerLayout->addWidget(errorLabel);
        } else {
             qWarning() << "loadAndDisplayAuthors: Cannot display error, authorsContainerWidget or authorsContainerLayout is null.";
        }
        return;
    }

    QList<AuthorDisplayInfo> authors = m_dbManager->getAllAuthorsForDisplay();

    if (authors.isEmpty()) {
         qInfo() << "No authors found in the database or failed to load.";
    }

    displayAuthors(authors);
}
