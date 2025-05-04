-- name: GetOrderHeaderById
SELECT order_id, order_date::text, total_amount, shipping_address, payment_method
FROM "order"
WHERE order_id = :orderId;

-- name: GetOrderItemsByOrderId
SELECT oi.quantity, oi.price_per_unit, b.title
FROM order_item oi
JOIN book b ON oi.book_id = b.book_id
WHERE oi.order_id = :orderId;

-- name: GetOrderStatusesByOrderId
SELECT status, status_date, tracking_number
FROM order_status
WHERE order_id = :orderId
ORDER BY status_date ASC;

-- name: InsertOrderHeader
INSERT INTO "order" (customer_id, order_date, total_amount, shipping_address, payment_method)
VALUES (:customer_id, CURRENT_TIMESTAMP, 0.0, :shipping_address, :payment_method)
RETURNING order_id;

-- name: GetBookPriceAndStockForUpdate
SELECT price, stock_quantity FROM book WHERE book_id = :book_id FOR UPDATE;

-- name: UpdateBookStock
UPDATE book SET stock_quantity = stock_quantity - :quantity WHERE book_id = :book_id AND stock_quantity >= :quantity;

-- name: InsertOrderItem
INSERT INTO order_item (order_id, book_id, quantity, price_per_unit)
VALUES (:order_id, :book_id, :quantity, :price_per_unit);

-- name: UpdateOrderTotalAmount
UPDATE "order" SET total_amount = :total WHERE order_id = :order_id;

-- name: InsertOrderStatus
INSERT INTO order_status (order_id, status, status_date)
VALUES (:order_id, :status, CURRENT_TIMESTAMP);

-- name: GetCustomerOrderHeadersByCustomerId
SELECT order_id, order_date::text, total_amount, shipping_address, payment_method
FROM "order"
WHERE customer_id = :customerId
ORDER BY order_date DESC;
