#pragma once

#include <string>
#include <vector>
#include <optional>
#include <variant>
#include <memory>

// Data types
struct VarCharType { int length; };
struct IntType {};
struct FloatType {};
struct BoolType {};
struct TimestampType {};

using DataType = std::variant<
    IntType,
    FloatType,
    BoolType,
    TimestampType,
    VarCharType
>;

// Expressions
struct Literal
{
    using Value = std::variant<int64_t, double, bool, std::string, std::nullptr_t>;
    Value value;
};

struct ColumnRef
{
    std::string table;
    std::string column;
};

enum class BinaryOp
{
    EQ, NEQ, LT, GT, LTE, GTE,
    AND, OR,
    PLUS, MINUS, STAR, SLASH
};

enum class UnaryOp { NOT, NEGATE };

struct AllColumns {};

struct BinaryExpr;
struct UnaryExpr;
struct FunctionCall;

using Expr = std::variant<
    Literal,
    ColumnRef,
    AllColumns,
    std::unique_ptr<BinaryExpr>,
    std::unique_ptr<UnaryExpr>,
    std::unique_ptr<FunctionCall>
>;

struct BinaryExpr
{
    Expr left;
    BinaryOp op;
    Expr right;
};

struct UnaryExpr
{
    UnaryOp op;
    Expr operand;
};

enum class AggFunc { COUNT, SUM, AVG, MIN, MAX };

struct FunctionCall
{
    AggFunc func;
    bool star;
    std::optional<Expr> arg;
};

// Select helpers
struct SelectColumn
{
    Expr expr;
    std::optional<std::string> alias;
};

struct TableSource
{
    std::string tableName;
    std::optional<std::string> alias;
};

enum class JoinType { INNER, LEFT, RIGHT, OUTER };

struct JoinClause
{
    JoinType type;
    TableSource table;
    Expr on;
};

enum class OrderDir { ASC, DESC };

struct OrderByClause
{
    Expr expr;
    OrderDir dir = OrderDir::ASC;
};

// Column definition
struct ColumnDef
{
    std::string name;
    DataType type;
    bool nullable = true;
    bool primaryKey = false;
    bool unique = false;
};

// Statements
struct SelectStatement
{
    std::vector<SelectColumn> columns;
    bool distinct = false;
    std::vector<TableSource> from;
    std::vector<JoinClause> joins;
    std::optional<Expr> where;
    std::vector<Expr> groupBy;
    std::optional<Expr> having;
    std::vector<OrderByClause> orderBy;
    std::optional<int> limit;
};

struct InsertStatement
{
    std::string tableName;
    std::optional<std::vector<std::string>> columns;
    std::vector<std::vector<Expr>> values;
};

struct UpdateStatement
{
    std::string tableName;
    std::vector<std::pair<std::string, Expr>> assignments;
    std::optional<Expr> where;
};

struct DeleteStatement
{
    std::string tableName;
    std::optional<Expr> where;
};

struct CreateTableStatement
{
    std::string tableName;
    std::vector<ColumnDef> columns;
};

struct CreateIndexStatement
{
    std::string indexName;
    std::string tableName;
    std::string column;
};

struct DropTableStatement  { std::string tableName; };
struct DropIndexStatement  { std::string indexName; };
struct BeginStatement      {};
struct CommitStatement     {};
struct RollbackStatement   {};

// Top level
using Statement = std::variant<
    SelectStatement,
    InsertStatement,
    UpdateStatement,
    DeleteStatement,
    CreateTableStatement,
    CreateIndexStatement,
    DropTableStatement,
    DropIndexStatement,
    BeginStatement,
    CommitStatement,
    RollbackStatement
>;