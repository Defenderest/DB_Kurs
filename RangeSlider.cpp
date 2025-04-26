#include "RangeSlider.h"
#include <QStyle>
#include <QStylePainter> // Використовуємо QStylePainter
#include <QStyleHints>   // Додано для globalStrut
#include <QApplication>  // Додано для QApplication::styleHints()
#include <QDebug>

RangeSlider::RangeSlider(Qt::Orientation orientation, QWidget *parent)
    : QWidget(parent), m_orientation(orientation)
{
    // Встановлюємо початкові значення та політику розміру
    setRange(0, 1000); // Початковий діапазон
    setLowerValue(0);
    setUpperValue(1000);
    // Політика розміру: розширюємось горизонтально, фіксована висота
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFocusPolicy(Qt::StrongFocus); // Дозволяємо отримувати фокус
    setAttribute(Qt::WA_Hover); // Вмикаємо відстеження наведення миші
    m_handleSize = 18; // Розмір повзунка (діаметр кола)
    m_grooveHeight = 6; // Висота доріжки
}

QSize RangeSlider::minimumSizeHint() const
{
    // Повертаємо мінімальний розмір, що базується на новому дизайні
    ensurePolished();
    int w = m_handleSize * 2 + 100; // Дві ручки + мінімальна довжина доріжки
    int h = m_handleSize + 4; // Висота ручки + невеликі відступи зверху/знизу
    return QSize(w, h);
}

void RangeSlider::setMinimum(int min)
{
    if (min >= m_maximum) {
        qWarning("RangeSlider::setMinimum: Minimum cannot be greater than or equal to maximum");
        min = m_maximum - 1;
    }
    if (min == m_minimum) return;

    m_minimum = min;
    if (m_lowerValue < m_minimum) setLowerValue(m_minimum);
    if (m_upperValue < m_minimum) setUpperValue(m_minimum); // Малоймовірно, але можливо

    update(); // Перемалювати
    emit rangeChanged(m_minimum, m_maximum);
}

void RangeSlider::setMaximum(int max)
{
    if (max <= m_minimum) {
        qWarning("RangeSlider::setMaximum: Maximum cannot be less than or equal to minimum");
        max = m_minimum + 1;
    }
    if (max == m_maximum) return;

    m_maximum = max;
    if (m_upperValue > m_maximum) setUpperValue(m_maximum);
    if (m_lowerValue > m_maximum) setLowerValue(m_maximum); // Малоймовірно

    update();
    emit rangeChanged(m_minimum, m_maximum);
}

void RangeSlider::setLowerValue(int lower)
{
    lower = qBound(m_minimum, lower, m_upperValue); // Обмежуємо значення
    if (lower == m_lowerValue) return;

    m_lowerValue = lower;
    update();
    emit lowerValueChanged(m_lowerValue);
    emit rangeChanged(m_lowerValue, m_upperValue); // Також випромінюємо загальний сигнал
}

void RangeSlider::setUpperValue(int upper)
{
    upper = qBound(m_lowerValue, upper, m_maximum); // Обмежуємо значення
    if (upper == m_upperValue) return;

    m_upperValue = upper;
    update();
    emit upperValueChanged(m_upperValue);
    emit rangeChanged(m_lowerValue, m_upperValue); // Також випромінюємо загальний сигнал
}

void RangeSlider::setRange(int min, int max)
{
    if (min >= max) {
        qWarning("RangeSlider::setRange: Minimum must be less than maximum");
        max = min + 1;
    }
    if (min == m_minimum && max == m_maximum) return;

    m_minimum = min;
    m_maximum = max;

    // Коригуємо поточні значення, якщо вони виходять за новий діапазон
    setLowerValue(qBound(m_minimum, m_lowerValue, m_maximum));
    setUpperValue(qBound(m_minimum, m_upperValue, m_maximum));

    update();
    emit rangeChanged(m_minimum, m_maximum); // Сигнал про зміну діапазону
}

// --- Малювання ---

void RangeSlider::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing); // Вмикаємо згладжування

    // Кольори (можна винести в константи або члени класу)
    const QColor grooveBgColor = QColor(230, 230, 230); // Світло-сірий фон доріжки
    const QColor grooveFillColor = QColor(108, 117, 125); // Сірий для заповненої частини (#6c757d)
    const QColor handleBorderColor = QColor(90, 98, 104); // Темніший сірий для рамки повзунка
    const QColor handleFillColor = QColor(255, 255, 255); // Білий повзунок
    const QColor handleHoverColor = QColor(240, 240, 240); // Світліший білий при наведенні
    const QColor handlePressedColor = QColor(220, 220, 220); // Трохи темніший білий при натисканні

    // 1. Малюємо доріжку (groove)
    QRectF groove = grooveRect(); // Використовуємо QRectF для точності
    qreal grooveRadius = m_grooveHeight / 2.0;
    painter.setPen(Qt::NoPen);
    painter.setBrush(grooveBgColor);
    painter.drawRoundedRect(groove, grooveRadius, grooveRadius);

    // 2. Малюємо заповнену частину доріжки (між ручками)
    qreal lowerPos = valueToPosition(m_lowerValue);
    qreal upperPos = valueToPosition(m_upperValue);
    QRectF filledGroove = groove;
    filledGroove.setLeft(lowerPos - m_handleSize / 2.0); // Починаємо від центру нижнього повзунка
    filledGroove.setRight(upperPos + m_handleSize / 2.0); // Закінчуємо центром верхнього повзунка
    // Обрізаємо заповнену частину по краях основної доріжки
    filledGroove = filledGroove.intersected(groove);

    painter.setBrush(grooveFillColor);
    painter.drawRoundedRect(filledGroove, grooveRadius, grooveRadius);

    // 3. Малюємо нижню ручку
    QRectF lowerHandleRect = handleRect(LowerHandle);
    QColor lowerHandleCurrentFill = handleFillColor;
    if (m_pressedControl == LowerHandle) lowerHandleCurrentFill = handlePressedColor;
    else if (m_hoverControl == LowerHandle) lowerHandleCurrentFill = handleHoverColor;

    painter.setPen(QPen(handleBorderColor, 1.5)); // Рамка повзунка
    painter.setBrush(lowerHandleCurrentFill);
    painter.drawEllipse(lowerHandleRect);

    // 4. Малюємо верхню ручку
    QRectF upperHandleRect = handleRect(UpperHandle);
    QColor upperHandleCurrentFill = handleFillColor;
    if (m_pressedControl == UpperHandle) upperHandleCurrentFill = handlePressedColor;
    else if (m_hoverControl == UpperHandle) upperHandleCurrentFill = handleHoverColor;

    painter.setPen(QPen(handleBorderColor, 1.5));
    painter.setBrush(upperHandleCurrentFill);
    painter.drawEllipse(upperHandleRect);
}

// --- Обробка подій миші ---

void RangeSlider::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || m_maximum == m_minimum) {
        event->ignore();
        return;
    }
    event->accept();

    QStyleOptionSlider opt = getStyleOption();
    QPoint pos = event->pos();

    // Перевіряємо, чи клік був на ручках
    if (handleRect(UpperHandle).contains(pos)) {
        m_pressedControl = UpperHandle;
        m_clickOffset = pos.x() - handleRect(UpperHandle).left(); // Горизонтальний зсув
        m_lastPressedValue = m_upperValue;
    } else if (handleRect(LowerHandle).contains(pos)) {
        m_pressedControl = LowerHandle;
        m_clickOffset = pos.x() - handleRect(LowerHandle).left();
        m_lastPressedValue = m_lowerValue;
    } else {
        // Клік на доріжці - переміщуємо найближчу ручку
        int clickValue = positionToValue(pos.x());
        int midValue = (m_lowerValue + m_upperValue) / 2;
        if (qAbs(m_lowerValue - clickValue) < qAbs(m_upperValue - clickValue) || clickValue < midValue) {
             setLowerValue(clickValue);
             m_pressedControl = LowerHandle; // Починаємо тягнути нижню
             m_clickOffset = 0; // Зсув 0, бо ми встановили значення
             m_lastPressedValue = m_lowerValue;
        } else {
             setUpperValue(clickValue);
             m_pressedControl = UpperHandle; // Починаємо тягнути верхню
             m_clickOffset = 0;
             m_lastPressedValue = m_upperValue;
        }
        // Можна додати обробку QAbstractSlider::SliderPageStepAdd/Sub, якщо потрібно
        // triggerAction(QAbstractSlider::SliderPageStepAdd, false);
    }

    if (m_pressedControl != NoHandle) {
        update(); // Оновити вигляд (стан sunken)
    }
}

void RangeSlider::mouseMoveEvent(QMouseEvent *event)
{
    if (m_pressedControl == NoHandle || !(event->buttons() & Qt::LeftButton)) {
        updateHoverControl(event->pos()); // Оновлюємо стан наведення, якщо не тягнемо
        event->ignore();
        return;
    }
    event->accept();

    int newPos = event->pos().x() - m_clickOffset;
    int newValue = positionToValue(newPos);

    if (m_pressedControl == LowerHandle) {
        // Переконуємось, що не виходимо за межі та не перетинаємо верхню ручку
        newValue = qBound(m_minimum, newValue, m_upperValue);
        if (newValue != m_lowerValue) {
            setLowerValue(newValue);
            // Сигнал lowerValueChanged вже випромінюється в setLowerValue
        }
    } else if (m_pressedControl == UpperHandle) {
        newValue = qBound(m_lowerValue, newValue, m_maximum);
        if (newValue != m_upperValue) {
            setUpperValue(newValue);
            // Сигнал upperValueChanged вже випромінюється в setUpperValue
        }
    }
}

void RangeSlider::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || m_pressedControl == NoHandle) {
        event->ignore();
        return;
    }
    event->accept();

    Handle releasedControl = m_pressedControl;
    m_pressedControl = NoHandle;
    updateHoverControl(event->pos()); // Оновлюємо стан наведення
    update(); // Оновити вигляд (прибрати sunken)

    // Випромінюємо сигнал, якщо значення змінилося з моменту натискання
    if (releasedControl == LowerHandle && m_lowerValue != m_lastPressedValue) {
        // Сигнал вже був випромінений у setLowerValue під час руху
        // Можна додати окремий сигнал sliderReleased, якщо потрібно
    } else if (releasedControl == UpperHandle && m_upperValue != m_lastPressedValue) {
        // Аналогічно
    }
}

void RangeSlider::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::StyleChange || event->type() == QEvent::FontChange) {
        // Оновлюємо кешовані значення розмірів зі стилю
        minimumSizeHint(); // Цей метод оновлює m_handleWidth та m_grooveHeight
        update();
    }
    QWidget::changeEvent(event);
}


// --- Допоміжні функції ---

QRectF RangeSlider::grooveRect() const
{
    // Розраховуємо прямокутник доріжки вручну
    QRectF content = contentsRect().adjusted(m_handleSize / 2.0, 0, -m_handleSize / 2.0, 0); // Залишаємо місце для половини повзунка з боків
    qreal grooveY = (height() - m_grooveHeight) / 2.0;
    return QRectF(content.left(), grooveY, content.width(), m_grooveHeight);
}

QRectF RangeSlider::handleRect(Handle handle) const
{
    // Розраховуємо прямокутник повзунка вручну
    qreal centerPos = valueToPosition( (handle == LowerHandle) ? m_lowerValue : m_upperValue );
    qreal handleX = centerPos - m_handleSize / 2.0;
    qreal handleY = (height() - m_handleSize) / 2.0; // Центруємо вертикально
    return QRectF(handleX, handleY, m_handleSize, m_handleSize);
}


int RangeSlider::positionToValue(int pos) const
{
    QRectF gr = grooveRect();
    qreal grooveLen = gr.width();
    if (grooveLen <= 0) return m_minimum;

    // Перетворюємо позицію в значення лінійно
    qreal relativePos = qBound(0.0, (qreal)pos - gr.left(), grooveLen);
    int value = m_minimum + static_cast<int>( (relativePos / grooveLen) * (m_maximum - m_minimum) + 0.5 ); // +0.5 для округлення
    return qBound(m_minimum, value, m_maximum);
}

int RangeSlider::valueToPosition(int val) const
{
    QRectF gr = grooveRect();
    qreal grooveLen = gr.width();
    if (grooveLen <= 0 || m_maximum == m_minimum) return static_cast<int>(gr.left());

    // Перетворюємо значення в позицію лінійно
    qreal relativeValue = qBound(0.0, (qreal)(val - m_minimum) / (m_maximum - m_minimum), 1.0);
    return static_cast<int>(gr.left() + relativeValue * grooveLen);
}

void RangeSlider::updateHoverControl(const QPoint& pos)
{
    Handle oldHover = m_hoverControl;
    m_hoverControl = NoHandle; // Скидаємо
    // Використовуємо QRectF::contains
    if (handleRect(UpperHandle).contains(QPointF(pos))) {
        m_hoverControl = UpperHandle;
    } else if (handleRect(LowerHandle).contains(QPointF(pos))) {
        m_hoverControl = LowerHandle;
    }

    if (m_hoverControl != oldHover) {
        setCursor(m_hoverControl != NoHandle ? Qt::PointingHandCursor : Qt::ArrowCursor); // Змінюємо курсор
        update(); // Перемалювати, якщо стан наведення змінився
    }
}

// Видаляємо getStyleOption, оскільки більше не використовуємо QStyleOptionSlider для малювання
// QStyleOptionSlider RangeSlider::getStyleOption(Handle handle) const { ... }
