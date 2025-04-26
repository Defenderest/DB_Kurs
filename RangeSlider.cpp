#include "RangeSlider.h"
#include <QStyle>
#include <QStylePainter> // Використовуємо QStylePainter
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
}

QSize RangeSlider::minimumSizeHint() const
{
    // Повертаємо мінімальний розмір, що базується на стилі
    ensurePolished(); // Переконуємось, що стиль застосовано
    int w = 0, h = 0;
    QStyleOptionSlider opt = getStyleOption();

    // Отримуємо розмір ручки зі стилю
    opt.subControls = QStyle::SC_SliderHandle;
    QSize handleSize = style()->sizeFromContents(QStyle::CT_Slider, &opt, QSize(), this);

    // Отримуємо розмір канавки
    opt.subControls = QStyle::SC_SliderGroove;
    QSize grooveSize = style()->sizeFromContents(QStyle::CT_Slider, &opt, QSize(), this);

    if (m_orientation == Qt::Horizontal) {
        w = handleSize.width() * 2 + 100; // Приблизна ширина: дві ручки + мінімальна довжина доріжки
        h = qMax(handleSize.height(), grooveSize.height());
        m_handleWidth = handleSize.width(); // Оновлюємо ширину ручки зі стилю
        m_grooveHeight = grooveSize.height(); // Оновлюємо висоту доріжки зі стилю
    } else {
        // Аналогічно для вертикальної орієнтації (не реалізовано повністю)
        w = qMax(handleSize.width(), grooveSize.width());
        h = handleSize.height() * 2 + 100;
    }
    return style()->sizeFromContents(QStyle::CT_Slider, &opt, QSize(w, h), this).expandedTo(QApplication::globalStrut());
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
    QStylePainter painter(this); // Використовуємо QStylePainter для малювання зі стилем
    QStyleOptionSlider opt;

    // 1. Малюємо доріжку (groove)
    opt = getStyleOption();
    opt.subControls = QStyle::SC_SliderGroove;
    painter.drawComplexControl(QStyle::CC_Slider, opt);

    // 2. Малюємо заповнену частину доріжки (між ручками)
    opt = getStyleOption();
    opt.subControls = QStyle::SC_SliderGroove; // Базуємось на доріжці
    QRect filledGroove = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove, this);
    int lowerPos = valueToPosition(m_lowerValue);
    int upperPos = valueToPosition(m_upperValue);

    if (m_orientation == Qt::Horizontal) {
        filledGroove.setLeft(lowerPos);
        filledGroove.setWidth(upperPos - lowerPos);
    } else {
        // Для вертикальної орієнтації (не реалізовано)
    }
    // Малюємо заповнену частину (можна використовувати інший стиль або колір)
    // Наприклад, заповнюємо прямокутник певним кольором
    painter.fillRect(filledGroove, palette().color(QPalette::Highlight)); // Використовуємо колір виділення

    // 3. Малюємо нижню ручку
    opt = getStyleOption(LowerHandle);
    opt.subControls = QStyle::SC_SliderHandle;
    opt.rect = handleRect(LowerHandle);
    if (m_hoverControl == LowerHandle || m_pressedControl == LowerHandle) {
        opt.state |= QStyle::State_Sunken; // Стан "натиснуто" або "наведено"
    }
    painter.drawComplexControl(QStyle::CC_Slider, opt);

    // 4. Малюємо верхню ручку
    opt = getStyleOption(UpperHandle);
    opt.subControls = QStyle::SC_SliderHandle;
    opt.rect = handleRect(UpperHandle);
     if (m_hoverControl == UpperHandle || m_pressedControl == UpperHandle) {
        opt.state |= QStyle::State_Sunken;
    }
    painter.drawComplexControl(QStyle::CC_Slider, opt);
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

QRect RangeSlider::grooveRect() const
{
    QStyleOptionSlider opt = getStyleOption();
    return style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove, this);
}

QRect RangeSlider::handleRect(Handle handle) const
{
    QStyleOptionSlider opt = getStyleOption(handle);
    opt.sliderPosition = (handle == LowerHandle) ? valueToPosition(m_lowerValue) : valueToPosition(m_upperValue);
    // Використовуємо SC_SliderHandle для отримання прямокутника ручки
    return style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);
}


int RangeSlider::positionToValue(int pos) const
{
    QRect gr = grooveRect();
    int grooveLen = gr.width();
    if (grooveLen <= 0) return m_minimum;

    // Використовуємо логіку QStyle для перетворення позиції в значення
    return QStyle::sliderValueFromPosition(m_minimum, m_maximum, pos - gr.left(), grooveLen, false); // false = не інвертовано
}

int RangeSlider::valueToPosition(int val) const
{
    QRect gr = grooveRect();
    int grooveLen = gr.width();
    if (grooveLen <= 0) return gr.left();

    // Використовуємо логіку QStyle для перетворення значення в позицію
    return QStyle::sliderPositionFromValue(m_minimum, m_maximum, val, grooveLen, false) + gr.left();
}

void RangeSlider::updateHoverControl(const QPoint& pos)
{
    Handle oldHover = m_hoverControl;
    m_hoverControl = NoHandle; // Скидаємо
    if (handleRect(UpperHandle).contains(pos)) {
        m_hoverControl = UpperHandle;
    } else if (handleRect(LowerHandle).contains(pos)) {
        m_hoverControl = LowerHandle;
    }

    if (m_hoverControl != oldHover) {
        update(); // Перемалювати, якщо стан наведення змінився
    }
}

// Створює QStyleOptionSlider для малювання
QStyleOptionSlider RangeSlider::getStyleOption(Handle handle) const
{
    QStyleOptionSlider opt;
    opt.initFrom(this); // Ініціалізуємо стандартними значеннями віджета
    opt.orientation = m_orientation;
    opt.minimum = m_minimum;
    opt.maximum = m_maximum;
    opt.sliderPosition = (handle == LowerHandle) ? m_lowerValue : m_upperValue; // Позиція для конкретної ручки
    opt.sliderValue = opt.sliderPosition; // Значення = позиція (для простоти)
    opt.singleStep = 1; // Крок
    opt.pageStep = (m_maximum - m_minimum) / 10; // Крок сторінки
    opt.tickPosition = QSlider::TicksBelow; // Позиція позначок (можна налаштувати)
    opt.tickInterval = (m_maximum - m_minimum) / 10; // Інтервал позначок
    if (m_orientation == Qt::Horizontal) {
        opt.state |= QStyle::State_Horizontal;
    }
    // Додаємо стан фокусу, якщо віджет у фокусі
    if (hasFocus()) {
        opt.activeSubControls = QStyle::SC_SliderHandle; // Активна ручка при фокусі
        opt.state |= QStyle::State_HasFocus;
    } else {
         opt.activeSubControls = QStyle::SC_None;
    }
    // Додаємо стан наведення
    if (m_hoverControl != NoHandle && m_pressedControl == NoHandle) { // Тільки якщо не натиснуто
        opt.state |= QStyle::State_MouseOver;
        if (m_hoverControl == LowerHandle && handle == LowerHandle) {
             opt.activeSubControls = QStyle::SC_SliderHandle;
        } else if (m_hoverControl == UpperHandle && handle == UpperHandle) {
             opt.activeSubControls = QStyle::SC_SliderHandle;
        }
    }

    return opt;
}
