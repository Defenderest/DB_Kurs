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
#include "starratingwidget.h"
#include <cmath> // Для std::floor
#include <QDebug> // Для відладки

const int DefaultStarSize = 20; // Розмір зірки за замовчуванням
const int Padding = 2;          // Відступ між зірками

StarRatingWidget::StarRatingWidget(QWidget *parent)
    : QWidget(parent),
      m_rating(0),
      m_maxRating(5),
      m_starColor(Qt::yellow), // Колір заповненої зірки
      m_emptyStarColor(Qt::lightGray), // Колір порожньої зірки
      m_readOnly(false),
      m_hoverRating(-1), // Спочатку немає наведення
      m_starSize(DefaultStarSize)
{
    // Вмикаємо відстеження миші, щоб отримувати mouseMoveEvent без натискання кнопки
    setMouseTracking(true);
    // Політика розміру, щоб віджет міг змінювати розмір
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    // Перераховуємо полігони зірок при створенні
    // (Це можна зробити і в paintEvent, але так ефективніше, якщо розмір не змінюється часто)
    m_stars.resize(m_maxRating);
    for (int i = 0; i < m_maxRating; ++i) {
        // Створюємо стандартний полігон зірки (можна налаштувати)
        QPolygonF starPolygon;
        starPolygon << QPointF(0.5, 0.0) << QPointF(0.618, 0.382) << QPointF(1.0, 0.382)
                    << QPointF(0.691, 0.618) << QPointF(0.809, 1.0) << QPointF(0.5, 0.764)
                    << QPointF(0.191, 1.0) << QPointF(0.309, 0.618) << QPointF(0.0, 0.382)
                    << QPointF(0.382, 0.382);
        // Масштабуємо полігон до потрібного розміру
        QTransform transform;
        transform.scale(m_starSize, m_starSize);
        m_stars[i] = transform.map(starPolygon);
    }
}

int StarRatingWidget::rating() const
{
    return m_rating;
}

// Слот для встановлення рейтингу
void StarRatingWidget::setRating(int rating)
{
    if (rating < 0) rating = 0;
    if (rating > m_maxRating) rating = m_maxRating;

    if (m_rating != rating) {
        m_rating = rating;
        emit ratingChanged(m_rating);
        update(); // Перемалювати віджет
    }
}

int StarRatingWidget::maxRating() const
{
    return m_maxRating;
}

// Встановлення максимального рейтингу (кількості зірок)
void StarRatingWidget::setMaxRating(int maxRating)
{
    if (maxRating <= 0) maxRating = 1; // Мінімум одна зірка
    if (m_maxRating != maxRating) {
        m_maxRating = maxRating;
        // Оновлюємо розмір вектора полігонів
        m_stars.resize(m_maxRating);
        for (int i = 0; i < m_maxRating; ++i) {
             // Перестворюємо полігони, якщо їх ще немає
             if (m_stars[i].isEmpty()) {
                 QPolygonF starPolygon;
                 starPolygon << QPointF(0.5, 0.0) << QPointF(0.618, 0.382) << QPointF(1.0, 0.382)
                             << QPointF(0.691, 0.618) << QPointF(0.809, 1.0) << QPointF(0.5, 0.764)
                             << QPointF(0.191, 1.0) << QPointF(0.309, 0.618) << QPointF(0.0, 0.382)
                             << QPointF(0.382, 0.382);
                 QTransform transform;
                 transform.scale(m_starSize, m_starSize);
                 m_stars[i] = transform.map(starPolygon);
             }
        }
        // Коригуємо поточний рейтинг, якщо він перевищує новий максимум
        if (m_rating > m_maxRating) {
            setRating(m_maxRating); // Це викличе update()
        } else {
            update(); // Перемалювати віджет
        }
        updateGeometry(); // Повідомити систему layout про зміну розміру
    }
}

QColor StarRatingWidget::starColor() const
{
    return m_starColor;
}

void StarRatingWidget::setStarColor(const QColor &color)
{
    if (m_starColor != color) {
        m_starColor = color;
        update();
    }
}

QColor StarRatingWidget::emptyStarColor() const
{
    return m_emptyStarColor;
}

void StarRatingWidget::setEmptyStarColor(const QColor &color)
{
    if (m_emptyStarColor != color) {
        m_emptyStarColor = color;
        update();
    }
}

bool StarRatingWidget::isReadOnly() const
{
    return m_readOnly;
}

void StarRatingWidget::setReadOnly(bool readOnly)
{
    if (m_readOnly != readOnly) {
        m_readOnly = readOnly;
        m_hoverRating = -1; // Скидаємо підсвічування при зміні режиму
        update();
    }
}

// Рекомендований розмір віджета
QSize StarRatingWidget::sizeHint() const
{
    // Ширина = кількість зірок * (розмір зірки + відступ) - відступ + 2 * горизонтальний відступ від краю
    // Висота = розмір зірки + 2 * вертикальний відступ від краю
    int width = m_maxRating * (m_starSize + Padding) - Padding + 2 * Padding;
    int height = m_starSize + 2 * Padding;
    return QSize(width, height);
}

// Мінімальний розмір віджета (такий самий, як рекомендований)
QSize StarRatingWidget::minimumSizeHint() const
{
    return sizeHint();
}

// Малювання віджета
void StarRatingWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true); // Згладжування

    // Малюємо кожну зірку
    for (int i = 0; i < m_maxRating; ++i) {
        // Визначаємо прямокутник для поточної зірки
        QRect starRect(i * (m_starSize + Padding) + Padding, Padding, m_starSize, m_starSize);

        // Визначаємо, чи повинна зірка бути заповненою
        // Враховуємо поточний рейтинг та рейтинг під курсором (якщо не readOnly)
        bool filled = false;
        if (m_readOnly) {
            filled = (i < m_rating);
        } else {
            // Якщо є hoverRating, використовуємо його, інакше - поточний рейтинг
            filled = (i < ((m_hoverRating >= 0) ? m_hoverRating : m_rating));
        }

        // Малюємо зірку
        paintStar(&painter, starRect, filled);
    }
}

// Малювання однієї зірки
void StarRatingWidget::paintStar(QPainter *painter, const QRect &rect, bool filled)
{
    painter->save(); // Зберігаємо стан painter

    // Встановлюємо колір та стиль пера/пензля
    QColor color = filled ? m_starColor : m_emptyStarColor;
    painter->setPen(color); // Колір контуру
    painter->setBrush(filled ? QBrush(color) : Qt::NoBrush); // Заливка або без заливки

    // Переміщуємо систему координат до початку прямокутника зірки
    painter->translate(rect.topLeft());

    // Малюємо полігон зірки (вже масштабований)
    if (m_stars.size() > 0) { // Перевірка, чи є полігони
         painter->drawPolygon(m_stars[0]); // Використовуємо перший полігон як шаблон
    }


    painter->restore(); // Відновлюємо стан painter
}

// Обробка натискання кнопки миші
void StarRatingWidget::mousePressEvent(QMouseEvent *event)
{
    if (m_readOnly) {
        QWidget::mousePressEvent(event); // Передаємо подію далі, якщо тільки для читання
        return;
    }

    if (event->button() == Qt::LeftButton) {
        int star = starAtPosition(event->position().x());
        if (star != -1) {
            // Якщо клікнули на ту саму зірку, що вже вибрана, скидаємо рейтинг на 0
            // інакше встановлюємо новий рейтинг
            setRating((star + 1 == m_rating) ? 0 : star + 1);
            // qInfo() << "Mouse Press - Star:" << star << "New Rating:" << m_rating;
        }
    } else {
        QWidget::mousePressEvent(event); // Обробка інших кнопок
    }
}

// Обробка руху миші
void StarRatingWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_readOnly) {
        QWidget::mouseMoveEvent(event);
        return;
    }

    // Визначаємо, над якою зіркою знаходиться курсор
    int star = starAtPosition(event->position().x());
    int newHoverRating = (star != -1) ? (star + 1) : -1; // +1, бо рейтинг від 1 до maxRating

    // Оновлюємо, тільки якщо hoverRating змінився
    if (m_hoverRating != newHoverRating) {
        m_hoverRating = newHoverRating;
        // qInfo() << "Mouse Move - Hover Star:" << star << "Hover Rating:" << m_hoverRating;
        update(); // Перемалювати для підсвічування
    }

    // Якщо ліва кнопка натиснута під час руху (drag), оновлюємо рейтинг
    if (event->buttons() & Qt::LeftButton && star != -1) {
         setRating(star + 1);
         // qInfo() << "Mouse Drag - Star:" << star << "New Rating:" << m_rating;
    }

    QWidget::mouseMoveEvent(event);
}

// Обробка відпускання кнопки миші (для скидання hover)
void StarRatingWidget::mouseReleaseEvent(QMouseEvent *event)
{
     if (m_readOnly) {
        QWidget::mouseReleaseEvent(event);
        return;
    }
    // Можна додати логіку, якщо потрібно щось зробити при відпусканні,
    // але основна логіка зміни рейтингу вже в press/move.
    // Скидання hoverRating відбувається в mouseMoveEvent, коли курсор виходить за межі зірок.
    QWidget::mouseReleaseEvent(event);
}


// Визначення індексу зірки за позицією X
int StarRatingWidget::starAtPosition(int x)
{
    // Перебираємо можливі позиції зірок
    for (int i = 0; i < m_maxRating; ++i) {
        int starLeft = i * (m_starSize + Padding) + Padding;
        int starRight = starLeft + m_starSize;
        // Перевіряємо, чи координата X знаходиться в межах поточної зірки
        if (x >= starLeft && x < starRight) {
            return i; // Повертаємо індекс зірки (0 до maxRating-1)
        }
    }
    return -1; // Курсор не над жодною зіркою
}
