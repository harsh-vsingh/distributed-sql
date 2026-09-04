#pragma once

#include <string>

enum class TokenType
{
    // literals
    INTEGER,
    FLOAT,
    STRING,
    TRUE_LIT,
    FALSE_LIT,
    NULL_LIT,

    // identifier
    IDENTIFIER,

    // DDL keywords
    CREATE,
    TABLE,
    INDEX,
    ON,
    DROP,

    // DML keywords
    SELECT,
    INSERT,
    INTO,
    VALUES,
    UPDATE,
    SET,
    DELETE,
    FROM,

    // query clauses
    WHERE,
    AND,
    OR,
    NOT,
    AS,
    DISTINCT,
    JOIN,
    INNER,
    LEFT,
    RIGHT,
    OUTER,
    GROUP,
    BY,
    ORDER,
    HAVING,
    LIMIT,
    ASC,
    DESC,

    // Aggregate functions
    COUNT,
    SUM,
    AVG,
    MIN,
    MAX,

    // transaction keywords
    BEGIN,
    COMMIT,
    ROLLBACK,

    // data types
    INT,
    BOOL,
    FLOAT_TYPE,
    VARCHAR,
    TIMESTAMP,

    // constraints
    PRIMARY,
    KEY,
    FOREIGN,
    REFERENCES,
    UNIQUE,

    // comparison operators
    EQ,
    NEQ,
    LT,
    GT,
    LTE,
    GTE,

    // arithmetic operators
    PLUS,
    MINUS,
    STAR,
    SLASH,

    // punctuation
    LPAREN,
    RPAREN,
    COMMA,
    SEMICOLON,
    DOT,

    // special
    END_OF_FILE
};

struct Token
{
    TokenType type;
    std::string value;
    int line;
    int col;
};