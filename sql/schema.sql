-- name: DropOrderStatusTable
DROP TABLE IF EXISTS order_status CASCADE;

-- name: DropOrderItemTable
DROP TABLE IF EXISTS order_item CASCADE;

-- name: DropCommentTable
DROP TABLE IF EXISTS comment CASCADE;

-- name: DropBookAuthorTable
DROP TABLE IF EXISTS book_author CASCADE;

-- name: DropOrderTable
DROP TABLE IF EXISTS "order" CASCADE;

-- name: DropBookTable
DROP TABLE IF EXISTS book CASCADE;

-- name: DropAuthorTable
DROP TABLE IF EXISTS author CASCADE;

-- name: DropPublisherTable
DROP TABLE IF EXISTS publisher CASCADE;

-- name: DropCartItemTable
DROP TABLE IF EXISTS cart_item CASCADE;

-- name: DropCustomerTable
DROP TABLE IF EXISTS customer CASCADE;

-- name: CreateCustomerTable
CREATE TABLE customer (
    customer_id SERIAL PRIMARY KEY, first_name VARCHAR(100) NOT NULL, last_name VARCHAR(100) NOT NULL,
    email VARCHAR(255) UNIQUE NOT NULL, phone VARCHAR(30), address TEXT,
    password_hash VARCHAR(64) NOT NULL, -- Додано поле для хешу пароля (SHA-256 = 64 hex chars)
    loyalty_program BOOLEAN DEFAULT FALSE, join_date DATE NOT NULL DEFAULT CURRENT_DATE,
    loyalty_points INTEGER DEFAULT 0 CHECK (loyalty_points >= 0)
);

-- name: CreatePublisherTable
CREATE TABLE publisher (
    publisher_id SERIAL PRIMARY KEY, name VARCHAR(255) NOT NULL UNIQUE, contact_info TEXT
);

-- name: CreateAuthorTable
CREATE TABLE author (
    author_id SERIAL PRIMARY KEY, first_name VARCHAR(100) NOT NULL, last_name VARCHAR(100) NOT NULL,
    birth_date DATE, nationality VARCHAR(100), image_path VARCHAR(512), biography TEXT
);

-- name: CreateBookTable
CREATE TABLE book (
    book_id SERIAL PRIMARY KEY, title VARCHAR(255) NOT NULL, isbn VARCHAR(20) UNIQUE,
    publication_date DATE, publisher_id INTEGER, price NUMERIC(10, 2) CHECK (price >= 0),
    stock_quantity INTEGER DEFAULT 0 CHECK (stock_quantity >= 0), description TEXT, language VARCHAR(50),
    page_count INTEGER CHECK (page_count > 0),
    cover_image_path VARCHAR(512),
    genre VARCHAR(100),
    CONSTRAINT fk_publisher FOREIGN KEY (publisher_id) REFERENCES publisher(publisher_id) ON DELETE SET NULL
);

-- name: CreateOrderTable
CREATE TABLE "order" (
    order_id SERIAL PRIMARY KEY, customer_id INTEGER,
    order_date TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP, total_amount NUMERIC(12, 2) CHECK (total_amount >= 0),
    shipping_address TEXT NOT NULL, payment_method VARCHAR(50),
    CONSTRAINT fk_customer FOREIGN KEY (customer_id) REFERENCES customer(customer_id) ON DELETE SET NULL
);

-- name: CreateBookAuthorTable
CREATE TABLE book_author (
    book_id INTEGER NOT NULL, author_id INTEGER NOT NULL, role VARCHAR(100),
    PRIMARY KEY (book_id, author_id),
    CONSTRAINT fk_book FOREIGN KEY (book_id) REFERENCES book(book_id) ON DELETE CASCADE,
    CONSTRAINT fk_author FOREIGN KEY (author_id) REFERENCES author(author_id) ON DELETE CASCADE
);

-- name: CreateOrderItemTable
CREATE TABLE order_item (
    order_item_id SERIAL PRIMARY KEY, order_id INTEGER NOT NULL, book_id INTEGER NOT NULL,
    quantity INTEGER NOT NULL CHECK (quantity > 0), price_per_unit NUMERIC(10, 2) NOT NULL CHECK (price_per_unit >= 0),
    CONSTRAINT fk_order FOREIGN KEY (order_id) REFERENCES "order"(order_id) ON DELETE CASCADE,
    CONSTRAINT fk_book FOREIGN KEY (book_id) REFERENCES book(book_id) ON DELETE RESTRICT
);

-- name: CreateOrderStatusTable
CREATE TABLE order_status (
    order_status_id SERIAL PRIMARY KEY, order_id INTEGER NOT NULL, status VARCHAR(50) NOT NULL,
    status_date TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP, tracking_number VARCHAR(100),
    CONSTRAINT fk_order FOREIGN KEY (order_id) REFERENCES "order"(order_id) ON DELETE CASCADE
);

-- name: CreateCommentTable
CREATE TABLE comment (
    comment_id SERIAL PRIMARY KEY,
    book_id INTEGER NOT NULL,
    customer_id INTEGER NOT NULL,
    comment_text TEXT NOT NULL,
    comment_date TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
    rating INTEGER CHECK (rating >= 0 AND rating <= 5), -- 0 = no rating, 1-5 stars
    CONSTRAINT fk_book_comment FOREIGN KEY (book_id) REFERENCES book(book_id) ON DELETE CASCADE,
    CONSTRAINT fk_customer_comment FOREIGN KEY (customer_id) REFERENCES customer(customer_id) ON DELETE CASCADE
);

-- name: CreateCartItemTable
CREATE TABLE cart_item (
    customer_id INTEGER NOT NULL,
    book_id INTEGER NOT NULL,
    quantity INTEGER NOT NULL CHECK (quantity > 0),
    added_date TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (customer_id, book_id), -- Комбінований первинний ключ
    CONSTRAINT fk_customer_cart FOREIGN KEY (customer_id) REFERENCES customer(customer_id) ON DELETE CASCADE,
    CONSTRAINT fk_book_cart FOREIGN KEY (book_id) REFERENCES book(book_id) ON DELETE CASCADE
);
