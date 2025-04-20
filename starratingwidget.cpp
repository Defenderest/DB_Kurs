#include "starratingwidget.h"
#include <QHBoxLayout>
#include <QPainter> // Може знадобитися для складнішого малювання, але поки що використовуємо QLabel

StarRatingWidget::StarRatingWidget(QWidget *parent)
    : QWidget(parent), m_currentRating(0)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0); // Без відступів всередині віджета
    layout->setSpacing(2); // Невеликий відступ між зірками

    // Створюємо QLabel для кожної зірки
    for (int i = 0; i < MAX_RATING; ++i) {
        QLabel *starLabel = new QLabel(STAR_EMPTY);
        starLabel->setAlignment(Qt::AlignCenter);
        // Встановлюємо стиль через styleSheet для легкого керування кольором та розміром
        starLabel->setStyleSheet(QString("font-size: %1pt; color: %2;")
                                     .arg(STAR_SIZE)
                                     .arg(STAR_COLOR_EMPTY));
        starLabel->setMinimumSize(STAR_SIZE + 4, STAR_SIZE + 4); // Мінімальний розмір для клікабельності
        m_starLabels.append(starLabel);
        layout->addWidget(starLabel);
    }

    setLayout(layout);
    setMouseTracking(true); // Вмикаємо відстеження руху миші без натискання кнопки
    setToolTip(tr("Натисніть, щоб встановити рейтинг (0-5 зірок)"));
}

int StarRatingWidget::rating() const
{
    return m_currentRating;
}

void StarRatingWidget::setRating(int rating)
{
    if (rating < 0 || rating > MAX_RATING) {
        rating = 0; // Виправляємо некоректне значення
    }

    if (m_currentRating != rating) {
        m_currentRating = rating;
        updateStars(); // Оновлюємо вигляд без підсвітки наведення
        emit ratingChanged(m_currentRating); // Випромінюємо сигнал
    }
}
QSize StarRatingWidget::sizeHint() const
{
    // Розраховуємо приблизний розмір на основі кількості зірок та їх розміру
    int width = (STAR_SIZE + 4) * MAX_RATING + layout()->spacing() * (MAX_RATING - 1);
    int height = STAR_SIZE + 4;
    return QSize(width, height);
}

void StarRatingWidget::mouseMoveEvent(QMouseEvent *event)
{
    // Визначаємо, над якою зіркою зараз курсор
    int hoverRating = starAtPosition(event->position().x());
    if (hoverRating != m_hoverRating) {
        m_hoverRating = hoverRating;
        updateStars(m_hoverRating); // Оновлюємо з підсвіткою наведення
    }
    QWidget::mouseMoveEvent(event);
}

void StarRatingWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        int clickedRating = starAtPosition(event->position().x());
        // Якщо клікнули на ту саму зірку, що вже вибрана, скидаємо рейтинг на 0
        if (clickedRating == m_currentRating) {
            setRating(0);
        } else {
            setRating(clickedRating); // Встановлюємо новий рейтинг
        }
    }
    QWidget::mousePressEvent(event);
}

void StarRatingWidget::leaveEvent(QEvent *event)
{
    // Коли миша покидає віджет, прибираємо підсвітку наведення
    if (m_hoverRating != -1) {
        m_hoverRating = -1;
        updateStars(); // Оновлюємо без підсвітки
    }
    QWidget::leaveEvent(event);
}

void StarRatingWidget::updateStars(int hoverRating)
{
    // Визначаємо, скільки зірок підсвічувати (або вибраних, або під курсором)
    int starsToHighlight = (hoverRating != -1) ? hoverRating : m_currentRating;

    for (int i = 0; i < MAX_RATING; ++i) {
        if (i < starsToHighlight) {
            // Зірка заповнена (або вибрана, або під курсором)
            m_starLabels[i]->setText(STAR_FULL);
            QString color = (hoverRating != -1 && i < hoverRating) ? STAR_COLOR_HOVER : STAR_COLOR_SELECTED;
            m_starLabels[i]->setStyleSheet(QString("font-size: %1pt; color: %2;")
                                               .arg(STAR_SIZE)
                                               .arg(color));
        } else {
            // Зірка порожня
            m_starLabels[i]->setText(STAR_EMPTY);
            m_starLabels[i]->setStyleSheet(QString("font-size: %1pt; color: %2;")
                                               .arg(STAR_SIZE)
                                               .arg(STAR_COLOR_EMPTY));
        }
    }
}
int StarRatingWidget::starAtPosition(int x)
{
    // Знаходимо, над якою зіркою знаходиться координата x
    // Простий розрахунок, припускаючи однакову ширину міток
    int starWidth = m_starLabels.isEmpty() ? (STAR_SIZE + 4) : m_starLabels[0]->width();
    int spacing = layout()->spacing();
    int rating = 0;
    int currentX = 0;

    for (int i = 0; i < MAX_RATING; ++i) {
        currentX += starWidth;
        if (x < currentX) {
            rating = i + 1;
            break;
        }
        currentX += spacing; // Додаємо відступ для наступної зірки
    }

    // Якщо x за межами останньої зірки, вважаємо, що це остання
    if (rating == 0 && x >= currentX - spacing) { // Перевірка, чи ми не за межами взагалі
        rating = MAX_RATING;
    }

    return rating;
}
