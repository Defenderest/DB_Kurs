-- Функція, яка буде викликатися тригером
CREATE OR REPLACE FUNCTION award_loyalty_points_on_order_completion()
RETURNS TRIGGER AS $$
DECLARE
    points_to_add INT;
BEGIN
    -- Перевіряємо, чи total_amount було оновлено до позитивного значення
    -- OLD.total_amount може бути NULL або 0 від початкової вставки
    IF NEW.total_amount IS NOT NULL AND NEW.total_amount > 0 AND (OLD.total_amount IS NULL OR OLD.total_amount <> NEW.total_amount) THEN
        -- Розраховуємо бали: 1 бал за кожні 10 одиниць валюти, заокруглено вниз.
        -- Змініть '10.0' на інше значення, якщо потрібно.
        points_to_add := FLOOR(NEW.total_amount / 10.0);

        IF points_to_add > 0 THEN
            UPDATE Customers
            SET loyalty_points = loyalty_points + points_to_add,
                loyalty_program = TRUE -- Активуємо програму лояльності, якщо ще не активна
            WHERE customer_id = NEW.customer_id;
        END IF;
    END IF;
    RETURN NEW; -- Повертаємо NEW для UPDATE тригерів
END;
$$ LANGUAGE plpgsql;

-- Тригер, що спрацьовує ПІСЛЯ оновлення поля total_amount в таблиці Orders
CREATE TRIGGER trg_award_loyalty_points_on_order_completion
AFTER UPDATE OF total_amount ON Orders
FOR EACH ROW -- Тригер спрацьовує для кожного зміненого рядка
EXECUTE FUNCTION award_loyalty_points_on_order_completion();

-- Примітка:
-- Цей тригер спрацьовує, коли total_amount в таблиці Orders оновлюється.
-- Ваша функція createOrder спочатку вставляє замовлення (можливо, з total_amount = 0 або NULL),
-- а потім оновлює total_amount до розрахованого значення. Саме на це оновлення і розрахований тригер.
-- Якщо total_amount може оновлюватися кілька разів для одного замовлення з інших причин,
-- цей тригер може спрацювати повторно. Якщо це небажано, логіку тригера, можливо,
-- доведеться ускладнити (наприклад, додати прапорець "бали нараховано" до таблиці Orders).
