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

QWidget* MainWindow::createAuthorCardWidget(const AuthorDisplayInfo &authorInfo)
{
    QFrame *cardFrame = new QFrame();
    cardFrame->setFrameShape(QFrame::StyledPanel);
    cardFrame->setFrameShadow(QFrame::Raised);
    cardFrame->setLineWidth(1);
    cardFrame->setMinimumSize(180, 250);
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
    const int maxColumns = 5;

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
        ui->authorsContainerLayout->addWidget(noAuthorsLabel, 0, 0, 1, maxColumns);
        return;
    }

    int row = 0;
    int col = 0;

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

    for (int c = 0; c < maxColumns; ++c) {
        ui->authorsContainerLayout->setColumnStretch(c, 1);
    }
    ui->authorsContainerLayout->setColumnStretch(maxColumns, 99);

    QLayoutItem* itemV = ui->authorsContainerLayout->itemAtPosition(row + 1, 0);
    if (itemV && itemV->spacerItem()) { delete ui->authorsContainerLayout->takeAt(ui->authorsContainerLayout->indexOf(itemV)); }
    QLayoutItem* itemH = ui->authorsContainerLayout->itemAtPosition(0, maxColumns);
    if (itemH && itemH->spacerItem()) { delete ui->authorsContainerLayout->takeAt(ui->authorsContainerLayout->indexOf(itemH)); }

    ui->authorsContainerLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum), 0, maxColumns);
    ui->authorsContainerLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding), row + 1, 0, 1, maxColumns);

    ui->authorsContainerWidget->updateGeometry();
}

void MainWindow::loadAndDisplayAuthors()
{
    if (!m_dbManager || !m_dbManager->isConnected()) {
        qWarning() << "loadAndDisplayAuthors: Database is not connected.";
        QLabel *errorLabel = new QLabel(tr("Не вдалося підключитися до бази даних для завантаження авторів."), ui->authorsContainerWidget);
        clearLayout(ui->authorsContainerLayout);
        ui->authorsContainerLayout->addWidget(errorLabel);
        return;
    }

    QList<AuthorDisplayInfo> authors = m_dbManager->getAllAuthorsForDisplay();

    if (authors.isEmpty()) {
         qInfo() << "No authors found in the database or failed to load.";
    }

    displayAuthors(authors);
}
