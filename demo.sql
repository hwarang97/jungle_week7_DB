CREATE TABLE members (id INT, name VARCHAR, age INT);

INSERT INTO members (name, age) VALUES (Alice, 20);
INSERT INTO members (name, age) VALUES (Bob, 31);
INSERT INTO members (id, name, age) VALUES (10, Carol, 27);

SELECT * FROM members;
SELECT id, name FROM members WHERE id = 2;
SELECT name, age FROM members WHERE name = Alice;
