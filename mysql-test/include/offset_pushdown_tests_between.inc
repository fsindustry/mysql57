#
# Offset Pushdown tests
#

SET optimizer_trace="enabled=on";

CREATE DATABASE optest;
USE optest;

--echo #
--echo # Primary key index scan and primary key column is int
--echo #

CREATE TABLE t1 (
        c1 int,
        c2 int,
        c3 char(10),
        PRIMARY KEY (c1),
        KEY(c2)
);

INSERT INTO t1 VALUES (1, 6, '3');
INSERT INTO t1 VALUES (2, 3, '4');
INSERT INTO t1 VALUES (3, 4, '1');
INSERT INTO t1 VALUES (4, 1, '6');
INSERT INTO t1 VALUES (5, 2, '2');
INSERT INTO t1 VALUES (6, 5, '5');
INSERT INTO t1 VALUES (7, 8, '9');
INSERT INTO t1 VALUES (8, 9, '7');
INSERT INTO t1 VALUES (9, 7, '8');

ANALYZE TABLE t1;

--echo #
--echo # Test 1: SELECT primary_key_column
--echo #

--echo # Covering index range scan
SELECT c1 FROM t1 WHERE c1 BETWEEN 3 AND 8 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT c1 FROM t1 USE INDEX(PRIMARY) WHERE (c1 BETWEEN 3 AND 3) OR (c1 > 6) LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo #
--echo # Test 2: SELECT *
--echo #

--echo # Index range scan
SELECT * FROM t1 WHERE c1 BETWEEN 3 AND 8 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
# todo can be optimized to support offset pushdown.
SELECT * FROM t1 WHERE (c1 BETWEEN 3 AND 8) OR (c1 < 5) LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo #
--echo # Test 5: order by primary key
--echo #

--echo # Index range scan
SELECT * FROM t1 WHERE c1 BETWEEN 3 AND 8 ORDER BY c1 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
# todo can be optimized to support offset pushdown.
SELECT * FROM t1 WHERE (c1 BETWEEN 3 AND 8) OR (c1 < 5) ORDER BY c1 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo #
--echo # Test 6: order by primary key desc
--echo #

--echo # Index range scan
SELECT * FROM t1 WHERE c1 BETWEEN 3 AND 8 ORDER BY c1 DESC LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
# todo can be optimized to support offset pushdown.
SELECT * FROM t1 WHERE (c1 BETWEEN 3 AND 8) OR (c1 < 5) ORDER BY c1 DESC LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

DROP TABLE t1;


--echo #
--echo # Primary key index scan and primary key column is char(10)
--echo #

CREATE TABLE t1 (
        c1 varchar(3),
        c2 int,
        c3 char(10),
        PRIMARY KEY (c1),
        KEY(c2)
);

INSERT INTO t1 VALUES ('001', 6, '3');
INSERT INTO t1 VALUES ('102', 3, '4');
INSERT INTO t1 VALUES ('003', 4, '1');
INSERT INTO t1 VALUES ('104', 1, '6');
INSERT INTO t1 VALUES ('005', 2, '2');
INSERT INTO t1 VALUES ('106', 5, '5');
INSERT INTO t1 VALUES ('007', 8, '9');
INSERT INTO t1 VALUES ('108', 9, '7');
INSERT INTO t1 VALUES ('009', 7, '8');
INSERT INTO t1 VALUES ('110', 10, '13');
INSERT INTO t1 VALUES ('011', 13, '12');
INSERT INTO t1 VALUES ('112', 11, '14');
INSERT INTO t1 VALUES ('013', 14, '11');
INSERT INTO t1 VALUES ('114', 12, '15');
INSERT INTO t1 VALUES ('015', 15, '10');

ANALYZE TABLE t1;

--echo #
--echo # Test 1: SELECT primary_key_column
--echo #

SELECT c1 FROM t1 WHERE c1 BETWEEN '008' AND '013' ORDER BY c1 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT c1 FROM t1 WHERE c1 BETWEEN '110' AND '110' ORDER BY c1 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo #
--echo # Test 2: SELECT *
--echo #

--echo # Index range scan
SELECT * FROM t1 WHERE c1 BETWEEN '008' AND '013' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c1 BETWEEN '110' AND '110' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo #
--echo # Test 5: order by primary key
--echo #

--echo # Index range scan
SELECT * FROM t1 WHERE c1 BETWEEN '008' AND '013' ORDER BY c1 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c1 BETWEEN '110' AND '110' ORDER BY c1 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo #
--echo # Test 6: order by primary key desc
--echo #

--echo # Index range scan
SELECT * FROM t1 WHERE c1 BETWEEN '008' AND '013' ORDER BY c1 DESC LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c1 BETWEEN '110' AND '110' ORDER BY c1 DESC LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;


--echo #
--echo # Test 7: order by another secondary index
--echo #
--echo # the plan has a sort operator
SELECT * FROM t1 WHERE c1 BETWEEN '008' AND '013' ORDER BY c2 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c1 BETWEEN '110' AND '110' ORDER BY c2 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo #
--echo # Test 8: the where condition contains two indexes
--echo #
--echo # the all index condition pushed down if enable icp
SELECT * FROM t1 WHERE ( c1 BETWEEN '100' AND '109' ) AND (c2 > 5 OR c2 < 4) LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

DROP TABLE t1;


--echo #
--echo # Secondary index scan and only select secondary index column,
--echo # namely no need to read the primary key by the secondary index record
--echo #

CREATE TABLE t1 (
        c1 int,
        c2 int,
        c3 char(10),
        c4 int,
        PRIMARY KEY (c1),
        KEY(c2),
        KEY(c3)
);

INSERT INTO t1 VALUES (1, 6, '003', 5);
INSERT INTO t1 VALUES (2, 3, '104', 4);
INSERT INTO t1 VALUES (3, 4, '001', 6);
INSERT INTO t1 VALUES (4, 1, '106', 3);
INSERT INTO t1 VALUES (5, 2, '002', 7);
INSERT INTO t1 VALUES (6, 5, '105', 1);
INSERT INTO t1 VALUES (7, 8, '009', 2);
INSERT INTO t1 VALUES (8, 9, '107', 8);
INSERT INTO t1 VALUES (9, 7, '008', 9);
INSERT INTO t1 VALUES (10, 10, '110', 11);
INSERT INTO t1 VALUES (11, 14, '011', 10);
INSERT INTO t1 VALUES (12, 12, '112', 12);
INSERT INTO t1 VALUES (13, 13, '013', 14);
INSERT INTO t1 VALUES (14, 11, '114', 13);

ANALYZE TABLE t1;

--echo #
--echo # Test 1: offset is greater than the total number of rows
--echo #

--echo # Covering index scan
SELECT c3 FROM t1 WHERE c3 BETWEEN '011' AND '115' LIMIT 9, 3;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo #
--echo # Test 2: limit + offset is greater than the total number of rows
--echo #

--echo # Covering index scan
SELECT c3 FROM t1 WHERE c3 BETWEEN '011' AND '115' LIMIT 3, 3;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo #
--echo # Test 3: normal select
--echo #

--echo # Covering index range scan
SELECT c3 FROM t1 WHERE c3 BETWEEN '011' AND '115' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT c3 FROM t1 WHERE c3 LIKE '00%' OR (c3 BETWEEN '005' AND '120') LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo #
--echo # Test 4: order by secondary index
--echo #

--echo # Covering index range scan
SELECT c3 FROM t1 USE INDEX(c3) WHERE c3 BETWEEN '100' AND '109' ORDER BY c3 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
# todo can be optimized to support offset pushdown.
SELECT c3 FROM t1 WHERE c3 LIKE '00%' OR (c3 BETWEEN '005' AND '120') ORDER BY c3 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo #
--echo # Test 5: order by secondary index desc
--echo #

--echo # Covering index range scan
SELECT c3 FROM t1 USE INDEX(c3) WHERE c3 BETWEEN '100' AND '109' ORDER BY c3 DESC LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
# todo can be optimized to support offset pushdown.
SELECT c3 FROM t1 WHERE c3 LIKE '00%' OR (c3 BETWEEN '005' AND '120') ORDER BY c3 DESC LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo #
--echo # Test 6: order by another index, normaly can not pushdown offset
--echo #
SELECT c3 FROM t1 USE INDEX(c3) WHERE c3 BETWEEN '100' AND '109' ORDER BY c2 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo #
--echo # Test 7: the where condition contains two secondary indexes,
--echo #         normaly can not pushdown offset
--echo #
SELECT * FROM t1 WHERE c2 > 10 AND c3 BETWEEN '011' AND '115' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c2 > 10 AND ( c3 BETWEEN '011' AND '115' ) OR c3 < '120' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

DROP TABLE t1;


--echo #
--echo # Secondary index scan and select all columns,
--echo # namely need to read the primary key by the secondary index record
--echo #

CREATE TABLE t1 (
        c1 int,
        c2 int,
        c3 char(10),
        c4 int,
        PRIMARY KEY (c1),
        KEY(c2),
        KEY(c3)
);

INSERT INTO t1 VALUES (1, 6, '003', 5);
INSERT INTO t1 VALUES (2, 3, '104', 4);
INSERT INTO t1 VALUES (3, 4, '001', 6);
INSERT INTO t1 VALUES (4, 1, '106', 3);
INSERT INTO t1 VALUES (5, 2, '002', 7);
INSERT INTO t1 VALUES (6, 5, '105', 1);
INSERT INTO t1 VALUES (7, 8, '009', 2);
INSERT INTO t1 VALUES (8, 9, '107', 8);
INSERT INTO t1 VALUES (9, 7, '008', 9);
INSERT INTO t1 VALUES (10, 10, '110', 11);
INSERT INTO t1 VALUES (11, 14, '011', 10);
INSERT INTO t1 VALUES (12, 12, '112', 12);
INSERT INTO t1 VALUES (13, 13, '013', 14);
INSERT INTO t1 VALUES (14, 11, '114', 13);

ANALYZE TABLE t1;

--echo #
--echo # Test 1: explicit primary key
--echo #

--echo #
--echo # Test 1.1: offset is greater than the total number of rows
--echo #
SELECT * FROM t1 WHERE c3 BETWEEN '011' AND '115' LIMIT 9, 3;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo #
--echo # Test 1.2: limit + offset is greater than the total number of rows
--echo #
SELECT * FROM t1 WHERE c3 BETWEEN '011' AND '115' LIMIT 3, 3;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo #
--echo # Test 1.3: normal select
--echo #

--echo # If ICP is disabled, these statements will use Table scan, otherwise Index range scan
SELECT * FROM t1 WHERE c3 BETWEEN '011' AND '115' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c3 LIKE '00%' OR (c3 BETWEEN '005' AND '120') LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo #
--echo # Test 1.4: order by secondary index
--echo #

--echo # Index range scan
SELECT * FROM t1 USE INDEX(c3) WHERE c3 BETWEEN '100' AND '109' ORDER BY c3 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
# todo can be optimized to support offset pushdown.
SELECT * FROM t1 WHERE c3 LIKE '00%' OR (c3 BETWEEN '005' AND '120') ORDER BY c3 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo #
--echo # Test 1.5: order by secondary index desc
--echo #

--echo # Index range scan
SELECT * FROM t1 USE INDEX(c3) WHERE c3 BETWEEN '100' AND '109' ORDER BY c3 DESC LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
# todo can be optimized to support offset pushdown.
SELECT * FROM t1 WHERE c3 LIKE '00%' OR (c3 BETWEEN '005' AND '120') ORDER BY c3 DESC LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo #
--echo # Test 1.6: order by another index, namely can not pushdown offset
--echo #
SELECT * FROM t1 USE INDEX(c3) WHERE c3 BETWEEN '100' AND '109' ORDER BY c2 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c3 LIKE '00%' OR (c3 BETWEEN '005' AND '120') ORDER BY c2 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo #
--echo # Test 2: implicit primary key
--echo #

ALTER TABLE t1 DROP PRIMARY KEY;
ANALYZE TABLE t1;

--echo #
--echo # Test 2.1: offset is greater than the total number of rows
--echo #
SELECT * FROM t1 WHERE c3 BETWEEN '011' AND '115' LIMIT 9, 3;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo #
--echo # Test 2.2: limit + offset is greater than the total number of rows
--echo #
SELECT * FROM t1 WHERE c3 BETWEEN '011' AND '115' LIMIT 3, 3;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo #
--echo # Test 2.3: normal select
--echo #

--echo # If ICP is disabled, these statements will use Table scan, otherwise Index range scan
SELECT * FROM t1 WHERE c3 BETWEEN '011' AND '115' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c3 LIKE '00%' OR (c3 BETWEEN '005' AND '120') LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo #
--echo # Test 2.4: order by secondary index
--echo #

--echo # Index range scan
SELECT * FROM t1 USE INDEX(c3) WHERE c3 BETWEEN '100' AND '109' ORDER BY c3 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
# todo can be optimized to support offset pushdown.
SELECT * FROM t1 WHERE c3 LIKE '00%' OR (c3 BETWEEN '005' AND '120') ORDER BY c3 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo #
--echo # Test 2.5: order by secondary index desc
--echo #

--echo # Index range scan
SELECT * FROM t1 USE INDEX(c3) WHERE c3 BETWEEN '100' AND '109' ORDER BY c3 DESC LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
# todo can be optimized to support offset pushdown.
SELECT * FROM t1 WHERE c3 LIKE '00%' OR (c3 BETWEEN '005' AND '120') ORDER BY c3 DESC LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo #
--echo # Test 2.6: order by another index, namely can not pushdown offset
--echo #
SELECT * FROM t1 USE INDEX(c3) WHERE c3 BETWEEN '100' AND '109' ORDER BY c2 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c3 LIKE '00%' OR (c3 BETWEEN '005' AND '120') ORDER BY c2 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

DROP TABLE t1;


--echo #
--echo # Multiple-column indexes scan and only select index column,
--echo # namely no need to read the primary key by the secondary index record
--echo #

CREATE TABLE t1 (
        c1 int,
        c2 int,
        c3 char(10),
        c4 int,
        c5 int,
        PRIMARY KEY (c1),
        KEY(c2, c3, c4),
        KEY(c5)
);

INSERT INTO t1 VALUES (1, 6, '003', 5, 3);
INSERT INTO t1 VALUES (2, 3, '104', 4, 1);
INSERT INTO t1 VALUES (3, 4, '001', 6, 2);
INSERT INTO t1 VALUES (4, 1, '106', 3, 4);
INSERT INTO t1 VALUES (5, 2, '002', 7, 8);
INSERT INTO t1 VALUES (6, 5, '105', 1, 5);
INSERT INTO t1 VALUES (7, 8, '009', 2, 6);
INSERT INTO t1 VALUES (8, 9, '107', 8, 7);
INSERT INTO t1 VALUES (9, 7, '008', 9, 9);
INSERT INTO t1 VALUES (10, 10, '110', 11, 10);
INSERT INTO t1 VALUES (11, 14, '011', 10, 12);
INSERT INTO t1 VALUES (12, 12, '112', 12, 11);
INSERT INTO t1 VALUES (13, 13, '013', 14, 13);
INSERT INTO t1 VALUES (14, 11, '114', 13, 13);

ANALYZE TABLE t1;

--echo #
--echo # Test 1: offset is greater than the total number of rows
--echo #
SELECT c4 FROM t1 WHERE c2 > 10 AND ( c3 BETWEEN '011' AND '115' ) AND c4 > 2 LIMIT 9, 3;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo #
--echo # Test 2: limit + offset is greater than the total number of rows
--echo #
SELECT c4 FROM t1 WHERE c2 > 10 AND ( c3 BETWEEN '011' AND '115' ) LIMIT 3, 3;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo #
--echo # Test 3: normal select
--echo #

--echo # Covering index range scan


SELECT c2, c3, c4 FROM t1 WHERE c3 BETWEEN '011' AND '115' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT c2, c3, c4 FROM t1 WHERE c2 > 10 AND ( c3 BETWEEN '011' AND '115' ) LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT c2, c3, c4 FROM t1 WHERE c2 > 10 AND ( c3 BETWEEN '011' AND '115' ) AND c4 > 1 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT c2, c3, c4 FROM t1 WHERE ( c3 BETWEEN '011' AND '115' ) AND c4 > 2 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo #
--echo # Test 4: order by multiple-column indexes
--echo #

--echo # Covering index range scan
SELECT c2, c3, c4 FROM t1 WHERE c3 BETWEEN '011' AND '115' ORDER BY c4 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT c2, c3, c4 FROM t1 WHERE c2 > 10 AND ( c3 BETWEEN '011' AND '115' ) ORDER BY c2, c3 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT c2, c3, c4 FROM t1 WHERE c2 > 10 AND ( c3 BETWEEN '011' AND '115' ) AND c4 > 1 ORDER BY c2, c3, c4 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT c2, c3, c4 FROM t1 WHERE ( c3 BETWEEN '011' AND '115' ) AND c4 > 2 ORDER BY c3, c4 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo #
--echo # Test 5: order by multiple-column indexes desc
--echo #

--echo # Covering index range scan
SELECT c2, c3, c4 FROM t1 WHERE c3 BETWEEN '011' AND '115' ORDER BY c4 DESC LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT c2, c3, c4 FROM t1 WHERE c2 > 10 AND ( c3 BETWEEN '011' AND '115' ) ORDER BY c2, c3 DESC LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT c2, c3, c4 FROM t1 WHERE c2 > 10 AND ( c3 BETWEEN '011' AND '115' ) AND c4 > 1 ORDER BY c2, c3, c4 DESC LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT c2, c3, c4 FROM t1 WHERE ( c3 BETWEEN '011' AND '115' ) AND c4 > 2 ORDER BY c3, c4 DESC LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo #
--echo # Test 6: order by another index, namely can not pushdown offset
--echo #
SELECT c2, c3, c4 FROM t1 WHERE c2 > 10 AND ( c3 BETWEEN '011' AND '115' ) ORDER BY c5 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo #
--echo # Test 7: the where condition contains two secondary indexes,
--echo #         namely can not pushdown offset
--echo #
SELECT c2, c3, c4 FROM t1 WHERE c2 > 10 AND ( c3 BETWEEN '011' AND '115' ) AND c5 < 13 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

DROP TABLE t1;


--echo #
--echo # Multiple-column indexes scan and select all columns,
--echo # namely need to read the primary key by the secondary index record
--echo #

CREATE TABLE t1 (
        c1 int,
        c2 int,
        c3 char(10),
        c4 int,
        c5 int,
        PRIMARY KEY (c1),
        KEY(c2, c3, c4),
        KEY(c5)
);

INSERT INTO t1 VALUES (1, 6, '003', 5, 3);
INSERT INTO t1 VALUES (2, 3, '104', 4, 1);
INSERT INTO t1 VALUES (3, 4, '001', 6, 2);
INSERT INTO t1 VALUES (4, 1, '106', 3, 4);
INSERT INTO t1 VALUES (5, 2, '002', 7, 8);
INSERT INTO t1 VALUES (6, 5, '105', 1, 5);
INSERT INTO t1 VALUES (7, 8, '009', 2, 6);
INSERT INTO t1 VALUES (8, 9, '107', 8, 7);
INSERT INTO t1 VALUES (9, 7, '008', 9, 9);
INSERT INTO t1 VALUES (10, 10, '110', 11, 10);
INSERT INTO t1 VALUES (11, 14, '011', 10, 12);
INSERT INTO t1 VALUES (12, 12, '112', 12, 11);
INSERT INTO t1 VALUES (13, 13, '013', 14, 13);
INSERT INTO t1 VALUES (14, 11, '114', 13, 13);

ANALYZE TABLE t1;

--echo #
--echo # Test 1: explicit primary key
--echo #

--echo #
--echo # Test 1.1: offset is greater than the total number of rows
--echo #
SELECT * FROM t1 WHERE c2 > 10 AND ( c3 BETWEEN '011' AND '115' ) AND c4 > 2 LIMIT 9, 3;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo #
--echo # Test 1.2: limit + offset is greater than the total number of rows
--echo #
SELECT * FROM t1 WHERE c2 > 10 AND ( c3 BETWEEN '011' AND '115' ) LIMIT 3, 3;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo #
--echo # Test 1.3: normal select
--echo #

--echo # If ICP is disabled, these statements will use Table scan, otherwise Index range scan
SELECT * FROM t1 WHERE c3 BETWEEN '011' AND '115' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c2 > 10 AND ( c3 BETWEEN '011' AND '115' ) LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c2 > 10 AND ( c3 BETWEEN '011' AND '115' ) AND c4 > 1 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE ( c3 BETWEEN '011' AND '115' ) AND c4 > 2 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo #
--echo # Test 1.4: order by multiple-column indexes
--echo #

--echo # Index range scan
SELECT * FROM t1 WHERE c3 BETWEEN '011' AND '115' ORDER BY c4 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c2 > 10 AND ( c3 BETWEEN '011' AND '115' ) ORDER BY c2, c3 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c2 > 10 AND ( c3 BETWEEN '011' AND '115' ) AND c4 > 1 ORDER BY c2, c3, c4 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE ( c3 BETWEEN '011' AND '115' ) AND c4 > 2 ORDER BY c3, c4 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo #
--echo # Test 1.5: order by multiple-column indexes desc
--echo #
--echo # Index range scan
SELECT * FROM t1 WHERE c3 BETWEEN '011' AND '115' ORDER BY c4 DESC LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c2 > 10 AND ( c3 BETWEEN '011' AND '115' ) ORDER BY c2, c3 DESC LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c2 > 10 AND ( c3 BETWEEN '011' AND '115' ) AND c4 > 1 ORDER BY c2, c3, c4 DESC LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE ( c3 BETWEEN '011' AND '115' ) AND c4 > 2 ORDER BY c3, c4 DESC LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo #
--echo # Test 1.6: order by another index, namely can not pushdown offset
--echo #
SELECT * FROM t1 WHERE c2 > 10 AND ( c3 BETWEEN '011' AND '115' ) ORDER BY c5 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo #
--echo # Test 1.7: the where condition contains two secondary indexes,
--echo #         namely can not pushdown offset
--echo #
SELECT * FROM t1 WHERE c2 > 10 AND ( c3 BETWEEN '011' AND '115' ) AND c5 < 13 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo #
--echo # Test 2: implicit primary key
--echo #

ALTER TABLE t1 DROP PRIMARY KEY;
ANALYZE TABLE t1;

--echo #
--echo # Test 2.1: offset is greater than the total number of rows
--echo #
SELECT * FROM t1 WHERE c2 > 10 AND ( c3 BETWEEN '011' AND '115' ) AND c4 > 2 LIMIT 9, 3;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo #
--echo # Test 2.2: limit + offset is greater than the total number of rows
--echo #
SELECT * FROM t1 WHERE c2 > 10 AND ( c3 BETWEEN '011' AND '115' ) LIMIT 3, 3;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo #
--echo # Test 2.3: normal select
--echo #

--echo # If ICP is disabled, these statements will use Table scan, otherwise Index range scan
SELECT * FROM t1 WHERE c3 BETWEEN '011' AND '115' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c2 > 10 AND ( c3 BETWEEN '011' AND '115' ) LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c2 > 10 AND ( c3 BETWEEN '011' AND '115' ) AND c4 > 1 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE ( c3 BETWEEN '011' AND '115' ) AND c4 > 2 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo #
--echo # Test 2.4: order by multiple-column indexes
--echo #

--echo # Index range scan
SELECT * FROM t1 WHERE c3 BETWEEN '011' AND '115' ORDER BY c4 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c2 > 10 AND ( c3 BETWEEN '011' AND '115' ) ORDER BY c2, c3 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c2 > 10 AND ( c3 BETWEEN '011' AND '115' ) AND c4 > 1 ORDER BY c2, c3, c4 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE ( c3 BETWEEN '011' AND '115' ) AND c4 > 2 ORDER BY c3, c4 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo #
--echo # Test 2.5: order by multiple-column indexes desc
--echo #
--echo # Index range scan
SELECT * FROM t1 WHERE c3 BETWEEN '011' AND '115' ORDER BY c4 DESC LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c2 > 10 AND ( c3 BETWEEN '011' AND '115' ) ORDER BY c2, c3 DESC LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c2 > 10 AND ( c3 BETWEEN '011' AND '115' ) AND c4 > 1 ORDER BY c2, c3, c4 DESC LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE ( c3 BETWEEN '011' AND '115' ) AND c4 > 2 ORDER BY c3, c4 DESC LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo #
--echo # Test 2.6: order by another index, namely can not pushdown offset
--echo #
SELECT * FROM t1 WHERE c2 > 10 AND ( c3 BETWEEN '011' AND '115' ) ORDER BY c5 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo #
--echo # Test 2.7: the where condition contains two secondary indexes,
--echo #         namely can not pushdown offset
--echo #
SELECT * FROM t1 WHERE c2 > 10 AND ( c3 BETWEEN '011' AND '115' ) AND c5 < 13 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

DROP TABLE t1;

--echo #
--echo # Test 3: The data type in the WHERE condition
--echo #

--echo #
--echo # Test 3.1: INT/INTEGER/MEDIUMINT/SMALLINT/TINYINT
--echo #

CREATE TABLE `t1` (
	`id` int NOT NULL,
	`c1` int NOT NULL,
	`c2` integer DEFAULT NULL,
	`c3` bigint NOT NULL,
	`c4` bigint DEFAULT NULL,
	`c5` mediumint NOT NULL,
	`c6` mediumint DEFAULT NULL,
	`c7` smallint NOT NULL,
	`c8` smallint DEFAULT NULL,
	`c9` tinyint NOT NULL,
	`c10` tinyint DEFAULT NULL,
	PRIMARY KEY (`id`),
	KEY `idx_c1` (`c1`),
	KEY `idx_c2` (`c2`),
	KEY `idx_c3` (`c3`),
	KEY `idx_c4` (`c4`),
	KEY `idx_c5` (`c5`),
	KEY `idx_c6` (`c6`),
	KEY `idx_c7` (`c7`),
	KEY `idx_c8` (`c8`),
	KEY `idx_c9` (`c9`),
	KEY `idx_c10` (`c10`)
) ENGINE = InnoDB;

INSERT INTO t1 VALUES (1, 1, 3, 1, 9, 4, 7, 2, 1, 7, 6);
INSERT INTO t1 VALUES (2, 9, 2, 3, 3, 5, 1, 1, 7, 3, 1);
INSERT INTO t1 VALUES (3, 2, 1, 4, 2, 8, 8, 4, 4, 1, 3);
INSERT INTO t1 VALUES (4, 4, 7, 8, 8, 3, 2, 7, 3, 4, 8);
INSERT INTO t1 VALUES (5, 7, 8, 5, 1, 6, 3, 8, 8, 6, 7);
INSERT INTO t1 VALUES (6, 8, 6, 6, 7, 2, 5, 5, 2, 9, 9);
INSERT INTO t1 VALUES (7, 3, 5, 7, 4, 1, 4, 6, 5, 5, 2);
INSERT INTO t1 VALUES (8, 6, 4, 9, 6, 7, 9, 9, 6, 4, 4);
INSERT INTO t1 VALUES (9, 5, 9, 2, 5, 9, 6, 3, 9, 2, 5);
INSERT INTO t1 VALUES (12, 12, 12, 5294967295, 5294967295, 12, 12, 12, 12, 12, 12);
INSERT INTO t1 VALUES (11, 11, 11, 6294967295, 6294967295, 11, 11, 11, 11, 11, 11);
INSERT INTO t1 VALUES (13, 13, 13, 7294967295, 7294967295, 13, 13, 13, 13, 13, 13);
INSERT INTO t1 VALUES (10, 10, 10, 8294967295, 8294967295, 10, 10, 10, 10, 10, 10);

ANALYZE TABLE t1;

--echo # INT/INTEGER
SELECT * FROM t1 WHERE c1 BETWEEN 2 AND 9 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c1 BETWEEN '2' AND '9' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c1 BETWEEN 2.3 AND 9.400011 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c2 BETWEEN 2 AND 9 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c2 BETWEEN '2' AND '9' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c2 BETWEEN 2.3 AND 9.400011 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo # BIGINT
SELECT * FROM t1 WHERE c3 BETWEEN 5294967294 AND 8294967295 LIMIT 2, 2; # Truncated to INT
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c3 BETWEEN 5294967294.3 AND 8294967295.400011 LIMIT 2, 2; # Truncated to INT
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c3 BETWEEN '5294967294.3' AND '8294967295.400011' LIMIT 2, 2; # Truncated to INT
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c3 BETWEEN 5294967294 AND 8294967295 LIMIT 2, 2; # Truncated to INT
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c4 BETWEEN 5294967294.3 AND 8294967295.400011 LIMIT 2, 2; # Truncated to INT
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c4 BETWEEN '5294967294.3' AND '8294967295.400011' LIMIT 2, 2; # Truncated to INT
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo # MEDIUMINT
SELECT * FROM t1 WHERE c5 BETWEEN 3 AND 8 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c5 BETWEEN '3' AND '8' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c5 BETWEEN 3.186 AND 8.400011 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c6 BETWEEN 2 AND 7 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c6 BETWEEN '2' AND '7' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c6 BETWEEN 2.186 AND 7.400011 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo # SMALLINT
SELECT * FROM t1 WHERE c7 BETWEEN 2 AND 9 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c7 BETWEEN '2' AND '9' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c7 BETWEEN 2.186 AND 9.400011 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c8 BETWEEN 2 AND 9 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c8 BETWEEN '2' AND '9' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c8 BETWEEN 2.186 AND 9.400011 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo # TINYINT
SELECT * FROM t1 WHERE c9 BETWEEN 2 AND 9 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c9 BETWEEN '2' AND '9' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c9 BETWEEN 2.186 AND 9.400011 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c10 BETWEEN 2 AND 9 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c10 BETWEEN '2' AND '9' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c10 BETWEEN 2.186 AND 9.400011 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

DROP TABLE t1;

--echo #
--echo # Test 3.2: FLOAT/DOUBLE/DECIMAL
--echo #

CREATE TABLE `t1` (
	`id` int NOT NULL,
	`c1` float NOT NULL,
	`c2` float DEFAULT NULL,
	`c3` double NOT NULL,
	`c4` double DEFAULT NULL,
	`c5` decimal(10, 6) NOT NULL,
	`c6` decimal(10, 6) DEFAULT NULL,
	PRIMARY KEY (`id`),
	KEY `idx_c1` (`c1`),
	KEY `idx_c2` (`c2`),
	KEY `idx_c3` (`c3`),
	KEY `idx_c4` (`c4`),
	KEY `idx_c5` (`c5`),
	KEY `idx_c6` (`c6`)
) ENGINE = InnoDB;

INSERT INTO t1 VALUES (1, 1.186, 3.186, 1.186, 1.186, 1.186, 7.186);
INSERT INTO t1 VALUES (2, 4.186, 1.186, 9.186, 4.186, 7.186, 3.186);
INSERT INTO t1 VALUES (3, 9.186, 7.186, 2.186, 2.186, 8.186, 2.186);
INSERT INTO t1 VALUES (4, 3.186, 9.186, 7.186, 5.186, 2.186, 1.186);
INSERT INTO t1 VALUES (5, 2.186, 2.186, 8.186, 6.186, 9.186, 9.186);
INSERT INTO t1 VALUES (6, 7.186, 6.186, 6.186, 9.186, 3.186, 5.186);
INSERT INTO t1 VALUES (7, 8.186, 2.186, 3.186, 8.186, 5.186, 4.186);
INSERT INTO t1 VALUES (8, 6.186, 5.186, 5.186, 7.186, 6.186, 6.186);
INSERT INTO t1 VALUES (9, 5.186, 4.186, 4.186, 3.186, 4.186, 8.186);

ANALYZE TABLE t1;

--echo # FLOAT
SELECT * FROM t1 WHERE c1 BETWEEN 3 AND 8 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c1 BETWEEN '3.186' AND '8.1186' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c1 BETWEEN 3.186 AND 8.1186 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c2 BETWEEN 2 AND 7 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c2 BETWEEN '2.186' AND '7.186' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c2 BETWEEN 2.186 AND 7.186 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo # DOUBLE
SELECT * FROM t1 WHERE c3 BETWEEN 2 AND 8 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c3 BETWEEN '2.186' AND '8.186' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c3 BETWEEN 2.186 AND 8.186 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c4 BETWEEN 2 AND 7 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c4 BETWEEN '2.186' AND '7.186' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c4 BETWEEN 2.186 AND 7.186 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo # DECIMAL
SELECT * FROM t1 WHERE c5 BETWEEN 2 AND 8 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c5 BETWEEN '2.186' AND '8.186' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c5 BETWEEN 2.186 AND 8.186 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c6 BETWEEN 2 AND 7 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c6 BETWEEN '2.186' AND '7.186' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c6 BETWEEN 2.186 AND 7.186 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

DROP TABLE t1;

--echo # enhanced test for DECIMAL

CREATE TABLE td1 (
   id int(11) NOT NULL auto_increment,
   c1 decimal(30,10) DEFAULT '0.0000000000' NOT NULL,
   c2 decimal(30,10) DEFAULT NULL,
   PRIMARY KEY (id),
   KEY `idx_c1` (c1),
   KEY `idx_c2` (c2)
);

INSERT INTO td1 VALUES ( 1, '0.0001000000', '0.0000200000');
INSERT INTO td1 VALUES ( 2, '1.0003000000', '1.0000400000');
INSERT INTO td1 VALUES ( 3, '2.0006000000', '49.0005000000');
INSERT INTO td1 VALUES ( 4, '0.0000000000', '0.0000000000');
INSERT INTO td1 VALUES ( 5, '1.0007000000', '1.0008000000');
INSERT INTO td1 VALUES ( 6, '2.0009000000', '2.0007000000');
INSERT INTO td1 VALUES ( 7, '3.0000300000', '3.0006000000');
INSERT INTO td1 VALUES ( 8, '6.0000200000', '6.0005000000');
INSERT INTO td1 VALUES ( 9, '2.0000100000', '2.0004000000');
INSERT INTO td1 VALUES ( 10, '-35.0004000000', '-5.0000030000');
INSERT INTO td1 VALUES ( 11, '3.0000000001', '3.0000200008');
INSERT INTO td1 VALUES ( 12, '21.0000000002', '21.0000000007');
INSERT INTO td1 VALUES ( 13, '13.0000000003', '13.0000000006');
INSERT INTO td1 VALUES ( 14, '22.0000000004', '22.0000000005');
INSERT INTO td1 VALUES ( 15, '33.0000000010', '33.0000000030');
INSERT INTO td1 VALUES ( 16, '23.0000000020', '23.0000000040');
INSERT INTO td1 VALUES ( 37, '335.0000000300', '335.0000000500');
INSERT INTO td1 VALUES ( 18, '134.0000000400', '134.0000000600');
INSERT INTO td1 VALUES ( 19, '133.0000005000', '133.0000700000');
INSERT INTO td1 VALUES ( 20, '135.0000006000', '135.0000800000');

ANALYZE TABLE td1;

--echo # include the boundary
SELECT * FROM td1 WHERE c1 BETWEEN 1 AND 2 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM td1 WHERE c1 BETWEEN '1.0003' AND '2.0009' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM td1 WHERE c1 BETWEEN 1.0003 AND 2.0009  LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM td1 WHERE c2 BETWEEN 2 AND 3 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM td1 WHERE c2 BETWEEN '2.00009' AND '3.0006' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM td1 WHERE c2 BETWEEN 2.00009 AND 3.0006 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

DROP TABLE td1;

--echo #
--echo # Test 3.3: CHAR/VARCHAR/BINARY/VARBINARY
--echo #

CREATE TABLE `t1` (
	`id` int NOT NULL,
	`c1` char(10) NOT NULL,
	`c2` char(10) DEFAULT NULL,
	`c3` varchar(20) NOT NULL,
	`c4` varchar(20) DEFAULT NULL,
	`c5` binary(10) NOT NULL,
	`c6` binary(10) DEFAULT NULL,
	`c7` varbinary(20) NOT NULL,
	`c8` varbinary(20) DEFAULT NULL,
	PRIMARY KEY (`id`),
	KEY `idx_c1` (`c1`),
	KEY `idx_c2` (`c2`),
	KEY `idx_c3` (`c3`),
	KEY `idx_c4` (`c4`),
	KEY `idx_c5` (`c5`),
	KEY `idx_c6` (`c6`),
	KEY `idx_c7` (`c7`),
	KEY `idx_c8` (`c8`)
) ENGINE = InnoDB;

INSERT INTO t1 VALUES (1, '004', '004', '003', '007', '003', '007', '008', '006');
INSERT INTO t1 VALUES (2, '001', '009', '006', '003', '006', '003', '002', '004');
INSERT INTO t1 VALUES (3, '003', '003', '008', '001', '008', '001', '005', '001');
INSERT INTO t1 VALUES (4, '009', '001', '002', '008', '002', '008', '001', '007');
INSERT INTO t1 VALUES (5, '007', '007', '009', '002', '009', '009', '009', '009');
INSERT INTO t1 VALUES (6, '002', '002', '001', '006', '001', '001', '003', '002');
INSERT INTO t1 VALUES (7, '005', '005', '005', '005', '005', '005', '007', '003');
INSERT INTO t1 VALUES (8, '006', '006', '004', '004', '006', '004', '004', '008');
INSERT INTO t1 VALUES (9, '008', '008', '007', '009', '008', '007', '005', '005');

ANALYZE TABLE t1;

--echo # CHAR
SELECT * FROM t1 WHERE c1 BETWEEN 0x303032 AND '008' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c2 BETWEEN '001' AND 0x303038 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo # VARCHAR
SELECT * FROM t1 WHERE c3 BETWEEN 0x303032 AND '008' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c4 BETWEEN '001' AND 0x303038 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo # BINARY
SELECT * FROM t1 WHERE c5 BETWEEN 0x303032 AND '008' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c6 BETWEEN '001' AND 0x303038 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo # VARBINARY
SELECT * FROM t1 WHERE c7 BETWEEN 0x303032 AND '008' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c8 BETWEEN '001' AND 0x303038 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

DROP TABLE t1;

--echo #
--echo # Test 3.4: BLOB/TEXT
--echo #

# TODO: If the WHERE condition contains BLOB or TEXT types, pushing down the offset clause is not currently supported.

--echo #
--echo # Test 3.4: DATE/TIME/YEAR/DATETIME/TIMESTAMP
--echo #

CREATE TABLE t1 (
	id int PRIMARY KEY,
	c1 year NOT NULL,
	c2 year(4) DEFAULT NULL,
	c3 date NOT NULL,
	c4 date DEFAULT NULL,
	c5 time NOT NULL,
	c6 time DEFAULT NULL,
	c7 datetime NOT NULL,
	c8 datetime DEFAULT NULL,
	c9 timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
	c10 timestamp DEFAULT CURRENT_TIMESTAMP,
  KEY `idx_c1` (c1),
  KEY `idx_c2` (c2),
  KEY `idx_c3` (c3),
  KEY `idx_c4` (c4),
  KEY `idx_c5` (c5),
  KEY `idx_c6` (c6),
  KEY `idx_c7` (c7),
  KEY `idx_c8` (c8),
  KEY `idx_c9` (c9),
  KEY `idx_c10` (c10)
);

INSERT INTO t1 VALUES (1, 0, 0, '1990-01-01', '19900101', '18:00:00', '18:00:00.78', '19900101T000000', '19900101T000000', '19900101T000000', '19900101T000000');
INSERT INTO t1 VALUES (2, 1999, 1999, '1999-01-01', '19990101', '12:12:00', '12:12:00.66', '1999-01-01T12:12:00', '1999-01-01T12:12:00', '1999-01-01T12:12:00', '1999-01-01T12:12:00');
INSERT INTO t1 VALUES (3, 2010, 2010, '2010-01-01', '20100101', '19:12:00', '19:12:00.66', '2010-1-1T19:12:01', '2010-1-1T19:12:01', '2010-1-1T19:12:01', '2010-1-1T19:12:01');
INSERT INTO t1 VALUES (4, 99, 99, '1999-12-01', '19991201', '13:13:00', '13:13:00.66', '1999-12-01 13:13:00', '1999-12-01 13:13:00', '1999-12-01 13:13:00', '1999-12-01 13:13:00');
INSERT INTO t1 VALUES (5, 1, 1, '2001-02-01', '20010201', '11:14:00', '11:14:00.66', '2001-02-01 11:14:00', '2001-02-01 11:14:00', '2001-02-01 11:14:00', '2001-02-01 11:14:00');
INSERT INTO t1 VALUES (6, 2, 2, '2003-03-01', '20030301', '17:17:00', '17:17:00.66', '20030301T171715', '20030301T171715', '20030301T171715', '20030301T171715');
INSERT INTO t1 VALUES (7, 2009, 2009, '2009-04-01', '20090401', '21:21:00', '21:21:00.66', '2009-04-01 21:21:00', '2009-04-01 21:21:00', '2009-04-01 21:21:00', '2009-04-01 21:21:00');
INSERT INTO t1 VALUES (8, 2007, 2007, '2007-05-01', '20070501', '14:14:00', '14:14:00.66', '2007-05-01 14:14:00', '2007-05-01 14:14:00', '2007-05-01 14:14:00', '2007-05-01 14:14:00');
INSERT INTO t1 VALUES (9, 2005, 2005, '2005-06-01', '20050601', '16:16:00', '16:16:00.66', '2005-06-01 16:16:00', '2005-06-01 16:16:00', '2005-06-01 16:16:00', '2005-06-01 16:16:00');
INSERT INTO t1 VALUES (10, 0, 0, '1989-02-01', '19890201', '15:15:00', '15:15:00.66', '1989-02-01 15:15:00', '1989-02-01 15:15:00', '1989-02-01 15:15:00', '1989-02-01 15:15:00');
INSERT INTO t1 VALUES (11, 2006, 2006, '2006-05-01', '20060501', '20:20:00', '20:20:00.66', '2006-05-01 20:20:00', '2006-05-01 20:20:00', '2006-05-01 20:20:00', '2006-05-01 20:20:00');
INSERT INTO t1 VALUES (12, 2004, 2004, '2004-04-01', '20040401', '12:02:00', '12:02:00.66', '2004-04-01 12:02:00', '2004-04-01 12:02:00', '2004-04-01 12:02:00', '2004-04-01 12:02:00');
INSERT INTO t1 VALUES (13, 2008, 2008, '2008-03-11', '20080311', '15:15:00', '15:15:00.66', '2008-03-11 15:15:00', '2008-03-11 15:15:00', '2008-03-11 15:15:00', '2008-03-11 15:15:00');
INSERT INTO t1 VALUES (14, 2003, 2003, '2003-02-21', '20030221', '10:10:00', '10:10:00.66', '2003-02-21 10:10:00', '2003-02-21 10:10:00', '2003-02-21 10:10:00', '2003-02-21 10:10:00');
INSERT INTO t1 VALUES (15, 2000, 2000, '2000-01-08', '20000108', '09:09:00', '09:09:00.66', '2000-01-08 09:09:00', '2000-01-08 09:09:00', '2000-01-08 09:09:00', '2000-01-08 09:09:00');

ANALYZE TABLE t1;

--echo #
--echo # Test 3.4.1: YEAR
--echo #

--echo # normal
SELECT * FROM t1 WHERE c1 BETWEEN '1990' AND '2010' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c2 BETWEEN '1999' AND '2010' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c1 BETWEEN '90' AND '10' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c2 BETWEEN '99' AND '10' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c1 BETWEEN 1990.001 AND 2010.001 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c2 BETWEEN 1999.001 AND 2010.001 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c1 BETWEEN 1990 AND 2010 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c2 BETWEEN 1999 AND 2010 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c1 BETWEEN 1990 AND '2010' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c2 BETWEEN 1999 AND '2010' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c1 BETWEEN 1990.001 AND '2010' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c2 BETWEEN 1999.001 AND '2010' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c1 BETWEEN 1990 AND 2010.001 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c2 BETWEEN 1999 AND 2010.001 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo # need truncate
SELECT * FROM t1 WHERE c1 BETWEEN '1990-01' AND '2010-01' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c2 BETWEEN '1999-01' AND '2010-01' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c1 BETWEEN 1990-01 AND 2010-01 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c2 BETWEEN 1999-01 AND 2010-01 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c1 BETWEEN '1990-01-01' AND '2010-01-01' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c2 BETWEEN '1999-01-01' AND '2010-01-01' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c1 BETWEEN 1990-01-01 AND 2010-01-01 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c2 BETWEEN 1999-01-01 AND 2010-01-01 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo #
--echo # Test 3.4.2: DATE
--echo #

--echo # normal
SELECT * FROM t1 WHERE c3 BETWEEN '19900101' AND '20101212' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c4 BETWEEN '19990101' AND '20101212' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c3 BETWEEN 19900101 AND 20101212 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c4 BETWEEN 19990101 AND 20101212 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c3 BETWEEN 19900101.0123 AND 20101212.0123 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c4 BETWEEN 19990101.0123 AND 20101212.0123 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c3 BETWEEN '1990-01-01' AND '2010-12-12' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c4 BETWEEN '1999-01-01' AND '2010-12-12' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo # need truncate
SELECT * FROM t1 WHERE c3 BETWEEN '1990-01-01 01:02:03' AND '2010-12-12 13:14:15' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c4 BETWEEN '1999-01-01 01:02:03.123456' AND '2010-12-12 13:14:15.12345678' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo # special cases which would pushdown offset
SELECT * FROM t1 WHERE c3 BETWEEN '90-01-01 01:02:03' AND '10-12-12 13:14:15' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c4 BETWEEN '99-01-01 01:02:03.123456' AND '10-12-12 13:14:15.12345678' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c3 BETWEEN '19900101.010203' AND '20101212.131415' AND c3 > '1990-01-01' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c4 BETWEEN '19990101.010203.123456' AND '20101212.131415.12345678' AND c4 < '2010-12-12' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c3 BETWEEN 19900101.010203 AND 20101212.131415 AND c3 > 1990-01-01 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c3 BETWEEN 19900101.010203 AND 20101212.131415 AND c3 > '1990-01-01' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo #
--echo # Test 3.4.3: TIME
--echo #

--echo # normal
SELECT * FROM t1 WHERE c5 BETWEEN '12:12:00' AND '19:12:00' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c6 BETWEEN '12:12:01' AND '15:15:01' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c5 BETWEEN '12:12:00.0123' AND '19:12:00.0123' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c6 BETWEEN '12:12:01.0123' AND '15:15:01.0123' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

# start by fzx @20240103 about: test case can't not support truncate '2010-12-12 19:12:00' to time in mysql 5.7
#--echo # need truncate
#SELECT * FROM t1 WHERE c5 >= '1990-01-01 12:12:00' AND c5 < '2010-12-12 19:12:00' LIMIT 2, 2;
#SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
#SELECT * FROM t1 WHERE c6 >= '1999-01-01 12:12:01.123456' AND c6 <= '2010-12-12 15:15:01.12345678' LIMIT 2, 2;
#SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

#--echo # special cases which would pushdown offset
#SELECT * FROM t1 WHERE c5 >= '1990-01-01 12:12:00' AND c5 < '2010-12-12 19:12:00' AND c5 < '19:12:00.123456' LIMIT 2, 2;
#SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
#SELECT * FROM t1 WHERE c6 >= '1999-01-01 12:12:01.123456' AND c6 <= '2010-12-12 15:15:01.12345678' AND c6 <= '15:15:01' LIMIT 2, 2;
#SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
# end by fzx @20240103 about: test case can't not support truncate '2010-12-12 19:12:00' to time in mysql 5.7


--echo #
--echo # Test 3.4.4: DATETIME
--echo #

--echo # normal
SELECT * FROM t1 WHERE c7 BETWEEN '2005-01-01 01:02:03' AND '2008-03-11 15:15:00' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c8 BETWEEN '1999-12-01 13:13:00.123456' AND '2010-12-12 13:14:15.12345678' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c7 BETWEEN '2005-01-01T01:02:03' AND '2008-03-11T15:15:00' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c8 BETWEEN '1999-12-01T13:13:00.123456' AND '2010-12-12T13:14:15.12345678' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c7 BETWEEN '2005-01-01 01:02:03' AND '2008-03-11T15:15:00' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c8 BETWEEN '1999-12-01 13:13:00.123456' AND '2010-12-12T13:14:15.12345678' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo # the unspecified time is all zeros
SELECT * FROM t1 WHERE c7 BETWEEN '2005-01-01 01' AND '2008-03-11 15:15' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c8 BETWEEN '1999-12-01 13:13' AND '2010-12-12 13:14:15.12345678' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c7 BETWEEN '2005-01-01T01' AND '2008-03-11T15:15' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c8 BETWEEN '1999-12-01T13:13' AND '2010-12-12T13:14:15.12345678' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c7 BETWEEN '2005-01-01 01' AND '2008-03-11T15:15' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c8 BETWEEN '1999-12-01 13:13' AND '2010-12-12T13:14:15.12345678' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c7 BETWEEN '1990-01-01 01' AND '2010-12-12 12:13' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c8 BETWEEN '1999-01-01 01:13' AND '2010-12-12 12:00' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c7 BETWEEN '1990-01-01T01' AND '2010-12-12T12:13' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c8 BETWEEN '1999-01-01T01:13' AND '2010-12-12T12:00' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c7 BETWEEN '1990-01-01 01' AND '2010-12-12T12:13' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c8 BETWEEN '1999-01-01 01:13' AND '2010-12-12T12:00' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo # special cases which would pushdown offset
SELECT * FROM t1 WHERE c7 BETWEEN '90-01-01 01:02:03' AND '10-12-12 13:14:15' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c8 BETWEEN '99-01-01 01:02:03.123456' AND '10-12-12 13:14:15.12345678' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c7 BETWEEN '90-01-01T01:02:03' AND '10-12-12T13:14:15' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c8 BETWEEN '99-01-01T01:02:03.123456' AND '10-12-12T13:14:15.12345678' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c7 BETWEEN '90-01-01 01:02:03' AND '10-12-12T13:14:15' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c8 BETWEEN '99-01-01 01:02:03.123456' AND '10-12-12T13:14:15.12345678' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c7 BETWEEN '19900101.010203' AND '20101212.131415' AND c7 > '1990-01-01' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c8 BETWEEN '19990101.010203.123456' AND '20101212.131415.12345678' AND c8 < '2010-12-12' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c7 BETWEEN 19900101.010203 AND 20101212.131415 AND c7 > 1990-01-01 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c7 BETWEEN 19900101.010203 AND 20101212.131415 AND c7 > '1990-01-01' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c7 BETWEEN '19900101.010203' AND '20101212.131415' AND c7 > 1990-01-01 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c8 BETWEEN '19990101.010203.123456' AND '20101212.131415.12345678' AND c8 < 2010-12-12 LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo #
--echo # Test 3.4.5: TIMESTAMP
--echo #

--echo # normal
SELECT * FROM t1 WHERE c9 BETWEEN '2005-01-01 01:02:03' AND '2008-03-11 15:15:00' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c10 BETWEEN '1999-12-01 13:13:00.123456' AND '2010-12-12 13:14:15.12345678' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c9 BETWEEN '2005-01-01T01:02:03' AND '2008-03-11T15:15:00' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c10 BETWEEN '1999-12-01T13:13:00.123456' AND '2010-12-12T13:14:15.12345678' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c9 BETWEEN '2005-01-01 01:02:03' AND '2008-03-11T15:15:00' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c10 BETWEEN '1999-12-01 13:13:00.123456' AND '2010-12-12T13:14:15.12345678' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo # the unspecified time is all zeros
SELECT * FROM t1 WHERE c9 BETWEEN '2005-01-01 01' AND '2008-03-11 15:15' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c10 BETWEEN '1999-12-01 13:13' AND '2010-12-12 13:14:15.12345678' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c9 BETWEEN '2005-01-01T01' AND '2008-03-11T15:15' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c10 BETWEEN '1999-12-01T13:13' AND '2010-12-12T13:14:15.12345678' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c9 BETWEEN '2005-01-01 01' AND '2008-03-11T15:15' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c10 BETWEEN '1999-12-01 13:13' AND '2010-12-12T13:14:15.12345678' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c9 BETWEEN '1990-01-01 01' AND '2010-12-12 12:13' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c10 BETWEEN '1999-01-01 01:13' AND '2010-12-12 12:00' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c9 BETWEEN '1990-01-01T01' AND '2010-12-12T12:13' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c10 BETWEEN '1999-01-01T01:13' AND '2010-12-12T12:00' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c9 BETWEEN '1990-01-01 01' AND '2010-12-12T12:13' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c10 BETWEEN '1999-01-01 01:13' AND '2010-12-12T12:00' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo # special cases which would pushdown offset
SELECT * FROM t1 WHERE c9 BETWEEN '90-01-01 01:02:03' AND '10-12-12 13:14:15' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c10 BETWEEN '99-01-01 01:02:03.123456' AND '10-12-12 13:14:15.12345678' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c9 BETWEEN '90-01-01T01:02:03' AND '10-12-12T13:14:15' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c10 BETWEEN '99-01-01T01:02:03.123456' AND '10-12-12T13:14:15.12345678' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c9 BETWEEN '90-01-01 01:02:03' AND '10-12-12T13:14:15' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c10 BETWEEN '99-01-01 01:02:03.123456' AND '10-12-12T13:14:15.12345678' LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

--echo # special cases which would not pushdown offset
SELECT * FROM t1 WHERE c9 BETWEEN from_unixtime('631126923') AND from_unixtime('1292127194') LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;
SELECT * FROM t1 WHERE c10 BETWEEN from_unixtime('944025180') AND from_unixtime('1292127194') LIMIT 2, 2;
SELECT json_extract(trace, '$.steps[*].join_optimization.steps[*].refine_plan[*]') AS trace FROM information_schema.optimizer_trace;

DROP TABLE t1;
DROP DATABASE optest;

SET optimizer_trace="enabled=off";
