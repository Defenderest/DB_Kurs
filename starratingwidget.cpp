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
