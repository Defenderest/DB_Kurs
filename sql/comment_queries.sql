-- name: CheckUserCommentExists
SELECT COUNT(*)
FROM comment
WHERE book_id = :bookId AND customer_id = :customerId;

-- name: AddComment
INSERT INTO comment (book_id, customer_id, comment_text, comment_date, rating)
VALUES (:book_id, :customer_id, :comment_text, CURRENT_TIMESTAMP, :rating);

-- name: GetBookCommentsByBookId
SELECT
    c.comment_text,
    c.comment_date,
    c.rating,
    cust.first_name || ' ' || cust.last_name AS author_name
FROM comment c
JOIN customer cust ON c.customer_id = cust.customer_id
WHERE c.book_id = :bookId
ORDER BY c.comment_date DESC;
