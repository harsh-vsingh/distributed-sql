#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "ast.h"

// Resolved catalog references

struct BoundedColumnRef
{
    int tableId;
    int columnId;
    CatalogDataType type;
};

struct BoundedTableRef
{
    int tableId;
};

// Expressions
struct BoundedLiteral
{
    Literal::Value value;
    CatalogDataType type;
};


enum class BoundedBinaryOp
{
    EQ, NEQ, LT, GT, LTE, GTE,
    AND, OR,
    PLUS, MINUS, STAR, SLASH
};

enum class BoundedUnaryOp
{
    NOT,
    NEGATE
};

struct BoundedBinaryExpr;
struct BoundedUnaryExpr;
struct BoundedFunctionCall;
struct BoundedAllColumns {};

using BoundedExpr = std::variant<
    BoundedLiteral,
    BoundedColumnRef,
    BoundedAllColumns,
    std::unique_ptr<BoundedBinaryExpr>,
    std::unique_ptr<BoundedUnaryExpr>,
    std::unique_ptr<BoundedFunctionCall>
>;

struct BoundedBinaryExpr
{
    BoundedExpr left;
    BoundedBinaryOp op;
    BoundedExpr right;

    CatalogDataType type;
};

struct BoundedUnaryExpr
{
    BoundedUnaryOp op;
    BoundedExpr operand;

    CatalogDataType type;
};

enum class AggFunc
{
    COUNT,
    SUM,
    AVG,
    MIN,
    MAX
};

struct BoundedFunctionCall
{
    AggFunc func;

    bool star = false;
    std::optional<BoundedExpr> arg;

    CatalogDataType type;
};

// SELECT helpers
struct BoundedSelectColumn
{
    BoundedExpr expr;
    std::optional<std::string> alias;
};

struct BoundedTableSource
{
    int tableId;
    std::string alias;
};

enum class BoundedJoinType
{
    INNER,
    LEFT,
    RIGHT
};

struct BoundedJoinClause
{
    BoundedJoinType type;
    BoundedTableSource table;
    BoundedExpr on;
};

enum class BoundedOrderDir
{
    ASC,
    DESC
};

struct BoundedOrderByClause
{
    BoundedExpr expr;
    BoundedOrderDir dir = BoundedOrderDir::ASC;
};

// Statements
struct BoundedSelectStatement
{
    std::vector<BoundedTableSource> from;
    std::vector<BoundedJoinClause> joins;

    std::vector<BoundedSelectColumn> columns;
    bool distinct = false;

    std::optional<BoundedExpr> where;
    std::vector<BoundedExpr> groupBy;
    std::optional<BoundedExpr> having;

    std::vector<BoundedOrderByClause> orderBy;
    std::optional<int> limit;
};

struct BoundedInsertStatement
{
    int tableId;

    std::optional<std::vector<int>> columnIds;

    std::vector<std::vector<BoundedExpr>> values;
};

struct BoundedUpdateAssignment
{
    int columnId;
    BoundedExpr value;
};

struct BoundedUpdateStatement
{
    int tableId;

    std::vector<BoundedUpdateAssignment> assignments;

    std::optional<BoundedExpr> where;
};

struct BoundedDeleteStatement
{
    int tableId;

    std::optional<BoundedExpr> where;
};

struct BoundedCreateTableStatement
{
    std::string tableName;
    std::vector<ColumnDef> columns;
};

struct BoundedCreateIndexStatement
{
    std::string indexName;
    int tableId;
    int columnId;
};

struct BoundedDropTableStatement
{
    int tableId;
};

struct BoundedDropIndexStatement
{
    int indexId;
    int columnId;
    int tableId;
};

struct BoundedBeginStatement {};
struct BoundedCommitStatement {};
struct BoundedRollbackStatement {};

// Top level
using BoundStatement = std::variant<
    BoundedSelectStatement,
    BoundedInsertStatement,
    BoundedUpdateStatement,
    BoundedDeleteStatement,
    BoundedCreateTableStatement,
    BoundedCreateIndexStatement,
    BoundedDropTableStatement,
    BoundedDropIndexStatement,
    BoundedBeginStatement,
    BoundedCommitStatement,
    BoundedRollbackStatement
>;