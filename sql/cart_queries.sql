-- name: GetCartItemsByCustomerId
SELECT book_id, quantity FROM cart_item WHERE customer_id = :customerId;

-- name: AddOrUpdateCartItem
INSERT INTO cart_item (customer_id, book_id, quantity, added_date)
VALUES (:customerId, :bookId, :quantity, CURRENT_TIMESTAMP)
ON CONFLICT (customer_id, book_id) DO UPDATE SET
    quantity = EXCLUDED.quantity,
    added_date = CURRENT_TIMESTAMP;

-- name: RemoveCartItem
DELETE FROM cart_item WHERE customer_id = :customerId AND book_id = :bookId;

-- name: ClearCartByCustomerId
DELETE FROM cart_item WHERE customer_id = :customerId;
