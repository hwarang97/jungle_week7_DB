CREATE TABLE users_idx_demo (id INT, name VARCHAR, age INT);

INSERT INTO users_idx_demo (id, name, age) VALUES (1, 'jiun', 25);
INSERT INTO users_idx_demo (id, name, age) VALUES (2, 'minsuk', 24);
INSERT INTO users_idx_demo (id, name, age) VALUES (3, 'seokje', 26);

SELECT * FROM users_idx_demo WHERE id = 2;
SELECT * FROM users_idx_demo WHERE name = 'minsuk';
