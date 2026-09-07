#pragma once

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "ast.h"
#include "bound_ast.h"
#include "catalog.h"

class Binder
{
    struct TableScope
    {
        std::unordered_map<std::string, TableData> tables;
    };

    Catalog& catalog;

    //Locking
    void acquireSharedLocks(const std::vector<std::string>& tableNames, 
        const std::string& txnId);
    void acquireExclusiveLocks(const std::vector<std::string>& tableNames, 
        const std::string& txnId);

    // Catalog access
    CatalogDataType getExpressionType(const BoundedExpr& expr) const;
    TableData getTable(const std::string& tableName) const;

    // Scope management
    void addTableToScope(TableScope& scope, const TableSource& tableSource) const;

    //Expression binding
    BoundedExpr bindExpression(const Expr& expr, const TableScope& scope) const;
    BoundedExpr bindLiteral(const Literal& literal) const;
    BoundedExpr bindColumnRef(const ColumnRef& columnRef, const TableScope& scope) const;
    BoundedExpr bindBinaryExpr(const BinaryExpr& binaryExpr, const TableScope& scope) const;
    BoundedExpr bindUnaryExpr(const UnaryExpr& unaryExpr, const TableScope& scope) const;
    BoundedExpr bindFunctionCall(const FunctionCall& functionCall, const TableScope& scope) const;

    CatalogDataType getLiteralType(const Literal& literal) const;
    CatalogDataType getBinaryResultType(BinaryOp op, 
        const BoundedExpr& left, const BoundedExpr& right) const;

    CatalogDataType getUnaryResultType(UnaryOp op, const BoundedExpr& operand ) const;
    CatalogDataType getFunctionResultType(const FunctionCall& functionCall, 
        const std::optional<BoundedExpr>& arg) const;

    bool isNumeric(CatalogDataType type) const;

    // DML Statement binding
    void BoundSelectStatement(const SelectStatement& selectStmt, 
        BoundedSelectStatement& boundedSelectStmt, 
        const std::string& txnId);
    void BoundInsertStatement(const InsertStatement& insertStmt, 
        BoundedInsertStatement& boundedInsertStmt, 
        const std::string& txnId);
    void BoundUpdateStatement(const UpdateStatement& updateStmt, 
        BoundedUpdateStatement& boundedUpdateStmt, 
        const std::string& txnId);
    void BoundDeleteStatement(const DeleteStatement& deleteStmt, 
        BoundedDeleteStatement& boundedDeleteStmt, 
        const std::string& txnId);
 
    // DDL Statement binding
    void BoundCreateTableStatement(const CreateTableStatement& createStmt, 
        BoundedCreateTableStatement& boundedCreateStmt, 
        const std::string& txnId);
    void BoundCreateIndexStatement(const CreateIndexStatement& createStmt, 
        BoundedCreateIndexStatement& boundedCreateIndexStmt, 
        const std::string& txnId);
    void BoundDropTableStatement(const DropTableStatement& dropStmt, 
        BoundedDropTableStatement& boundedDropTableStmt, 
        const std::string& txnId);
    void BoundDropIndexStatement(const DropIndexStatement& dropStmt, 
        BoundedDropIndexStatement& boundedDropIndexStmt, 
        const std::string& txnId);

    // Transaction Statement binding
    void BoundBeginStatement(const BeginStatement& beginStmt, 
        BoundedBeginStatement& boundedBeginStmt, 
        const std::string& txnId);
    void BoundCommitStatement(const CommitStatement& commitStmt, 
        BoundedCommitStatement& boundedCommitStmt, 
        const std::string& txnId);
    void BoundRollbackStatement(const RollbackStatement& rollbackStmt, 
        BoundedRollbackStatement& boundedRollbackStmt, 
        const std::string& txnId);

public:
    explicit Binder(Catalog& catalog) : catalog(catalog) {}

    BoundStatement bind(const Statement& statement,const std::string& txnId);
};