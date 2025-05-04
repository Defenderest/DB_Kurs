-- name: GetCustomerLoginInfoByEmail
SELECT customer_id, password_hash
FROM customer
WHERE email = :email;

-- name: GetCustomerProfileInfoById
SELECT
    customer_id, first_name, last_name, email, phone, address,
    join_date, loyalty_program, loyalty_points
FROM customer
WHERE customer_id = :customerId;

-- name: RegisterCustomer
INSERT INTO customer (first_name, last_name, email, password_hash, join_date, loyalty_program, loyalty_points)
VALUES (:first_name, :last_name, :email, :password_hash, CURRENT_DATE, FALSE, 0)
RETURNING customer_id;

-- name: UpdateCustomerName
UPDATE customer
SET first_name = :firstName, last_name = :lastName
WHERE customer_id = :customerId;

-- name: UpdateCustomerAddress
UPDATE customer
SET address = :address
WHERE customer_id = :customerId;

-- name: AddLoyaltyPoints
UPDATE customer
SET loyalty_points = loyalty_points + :pointsToAdd,
    loyalty_program = TRUE -- Вмикаємо програму лояльності при нарахуванні балів
WHERE customer_id = :customerId;

-- name: UpdateCustomerPhone
UPDATE customer
SET phone = :phone
WHERE customer_id = :customerId;

-- name: CheckCustomerExistsById
SELECT 1 FROM customer WHERE customer_id = :customerId;
