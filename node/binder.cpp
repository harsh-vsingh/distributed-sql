#include "binder.h"

void Binder::acquireSharedLocks(const std::vector<std::string>& tableNames, const std::string& txnId)
{
    catalog.acquireSharedLock(tableNames, txnId);
}

void Binder::acquireExclusiveLocks(const std::vector<std::string>& tableNames, const std::string& txnId)
{
    catalog.acquireExclusiveLock(tableNames, txnId);
}

TableData Binder::getTable(const std::string& tableName) const
{
    auto table = catalog.getTable(tableName);

    if(!table.has_value())
    {
        throw std::runtime_error("Table '" + tableName + "' does not exist");
    }

    return table.value();
}

CatalogDataType Binder::getExpressionType(const BoundedExpr& expr) const
{
    if(std::holds_alternative<BoundedLiteral>(expr))
    {
        return std::get<BoundedLiteral>(expr).type;
    }

    if(std::holds_alternative<BoundedColumnRef>(expr))
    {
        return std::get<BoundedColumnRef>(expr).type;
    }

    if(std::holds_alternative<std::unique_ptr<BoundedBinaryExpr>>(expr))
    {
        return std::get<std::unique_ptr<BoundedBinaryExpr>>(expr)->type;
    }

    if(std::holds_alternative<std::unique_ptr<BoundedUnaryExpr>>(expr))
    {
        return std::get<std::unique_ptr<BoundedUnaryExpr>>(expr)->type;
    }

    if(std::holds_alternative<std::unique_ptr<BoundedFunctionCall>>(expr))
    {
        return std::get<std::unique_ptr<BoundedFunctionCall>>(expr)->type;
    }

    throw std::runtime_error("AllColumns does not have a scalar expression type");
}

void Binder::addTableToScope(TableScope& scope, const TableSource& tableSource) const
{
    TableData table = getTable(tableSource.tableName);

    std::string qualifier = tableSource.alias.value_or(tableSource.tableName);

    if(scope.tables.find(qualifier) != scope.tables.end())
        throw std::runtime_error("Duplicate table alias '" + qualifier + "'");

    scope.tables.emplace(qualifier, std::move(table));
}

CatalogDataType Binder::getLiteralType(const Literal& literal) const
{
    if(std::holds_alternative<std::int64_t>(literal.value))
        return CatalogDataType::INT;

    if(std::holds_alternative<double>(literal.value))
        return CatalogDataType::FLOAT;

    if(std::holds_alternative<bool>(literal.value))
        return CatalogDataType::BOOL;

    if(std::holds_alternative<std::string>(literal.value))
        return CatalogDataType::VARCHAR;

    throw std::runtime_error("NULL does not have a concrete type");
}

bool Binder::isNumeric(CatalogDataType type) const
{
    return type == CatalogDataType::INT || type == CatalogDataType::FLOAT;
}

BoundedExpr Binder::bindLiteral(const Literal& literal) const
{
    BoundedLiteral boundedLiteral;
    boundedLiteral.value = literal.value;

    if(std::holds_alternative<std::nullptr_t>(literal.value))
        throw std::runtime_error("NULL type resolution is not implemented");

    boundedLiteral.type = getLiteralType(literal);

    return boundedLiteral;
}

BoundedExpr Binder::bindColumnRef(const ColumnRef& columnRef, const TableScope& scope) const
{
    if(!columnRef.table.empty())
    {
        auto tableIt = scope.tables.find(columnRef.table);

        if(tableIt == scope.tables.end())
            throw std::runtime_error("Unknown table or alias '" + columnRef.table + "'");

        auto column = tableIt->second.getColumn(columnRef.column);

        if(!column.has_value())
            throw std::runtime_error("Column '" + columnRef.column + 
                "' does not exist in table '" + columnRef.table + "'");

        BoundedColumnRef boundedColumn;
        boundedColumn.tableId = tableIt->second.tableId;
        boundedColumn.columnId = column->ColumnId;
        boundedColumn.type = column->type;

        return boundedColumn;
    }

    const Column* foundColumn = nullptr;
    const TableData* foundTable = nullptr;

    for(const auto& [qualifier, table] : scope.tables)
    {
        auto column = table.getColumn(columnRef.column);

        if(!column.has_value())
            continue;

        if(foundColumn != nullptr)
            throw std::runtime_error("Ambiguous column '" + columnRef.column + "'");

        foundColumn = &column.value();

        foundTable = &table;
    }

    if(foundColumn == nullptr)
        throw std::runtime_error("Column '" + columnRef.column + "' does not exist");

    BoundedColumnRef boundedColumn;
    boundedColumn.tableId = foundTable->tableId;
    boundedColumn.columnId = foundColumn->ColumnId;
    boundedColumn.type = foundColumn->type;

    return boundedColumn;
}

CatalogDataType Binder::getBinaryResultType(BinaryOp op, 
    const BoundedExpr& left, const BoundedExpr& right) const
{
    CatalogDataType leftType = getExpressionType(left);

    CatalogDataType rightType = getExpressionType(right);

    if(op == BinaryOp::AND || op == BinaryOp::OR)
    {
        if(leftType != CatalogDataType::BOOL || rightType != CatalogDataType::BOOL)
            throw std::runtime_error("Logical operators require BOOL operands");

        return CatalogDataType::BOOL;
    }

    if(op == BinaryOp::PLUS || op == BinaryOp::MINUS || 
        op == BinaryOp::STAR || op == BinaryOp::SLASH)
    {
        if(!isNumeric(leftType) || !isNumeric(rightType))
            throw std::runtime_error("Arithmetic operators require numeric operands");

        if(leftType == CatalogDataType::FLOAT || rightType == CatalogDataType::FLOAT)
            return CatalogDataType::FLOAT;

        return CatalogDataType::INT;
    }

    if(op == BinaryOp::EQ || op == BinaryOp::NEQ || op == BinaryOp::LT || 
        op == BinaryOp::GT || op == BinaryOp::LTE || op == BinaryOp::GTE)
    {
        if(isNumeric(leftType) && isNumeric(rightType))
            return CatalogDataType::BOOL;

        if(leftType == rightType)
            return CatalogDataType::BOOL;

        throw std::runtime_error("Incompatible comparison operands");
    }

    throw std::runtime_error("Unknown binary operator");
}

BoundedExpr Binder::bindBinaryExpr(const BinaryExpr& binaryExpr, const TableScope& scope) const
{
    BoundedExpr left = bindExpression(binaryExpr.left, scope);

    BoundedExpr right = bindExpression(binaryExpr.right, scope);

    BoundedBinaryExpr boundedBinary;
    boundedBinary.left = std::move(left);
    boundedBinary.op = static_cast<BoundedBinaryOp>(binaryExpr.op);
    boundedBinary.right = std::move(right);
    boundedBinary.type = getBinaryResultType(binaryExpr.op, boundedBinary.left, boundedBinary.right);

    return std::make_unique<BoundedBinaryExpr>(std::move(boundedBinary));
}

CatalogDataType Binder::getUnaryResultType(UnaryOp op, const BoundedExpr& operand) const
{
    CatalogDataType operandType = getExpressionType(operand);

    if(op == UnaryOp::NOT)
    {
        if(operandType != CatalogDataType::BOOL)
            throw std::runtime_error("NOT requires a BOOL operand");

        return CatalogDataType::BOOL;
    }

    if(op == UnaryOp::NEGATE)
    {
        if(!isNumeric(operandType))
            throw std::runtime_error("Unary '-' requires a numeric operand");

        return operandType;
    }

    throw std::runtime_error("Unknown unary operator");
}

BoundedExpr Binder::bindUnaryExpr(const UnaryExpr& unaryExpr, const TableScope& scope) const
{
    BoundedExpr operand = bindExpression(unaryExpr.operand, scope);

    BoundedUnaryExpr boundedUnary;
    boundedUnary.op = static_cast<BoundedUnaryOp>(unaryExpr.op);
    boundedUnary.operand = std::move(operand);
    boundedUnary.type = getUnaryResultType(unaryExpr.op, boundedUnary.operand);

    return std::make_unique<BoundedUnaryExpr>(std::move(boundedUnary));
}

CatalogDataType Binder::getFunctionResultType(
    const FunctionCall& functionCall,
    const std::optional<BoundedExpr>& arg) const
{
    if(functionCall.func == AggFunc::COUNT)
        return CatalogDataType::INT;

    if(!arg.has_value())
        throw std::runtime_error("Aggregate function requires an argument");

    CatalogDataType argType = getExpressionType(arg.value());

    if(functionCall.func == AggFunc::AVG || functionCall.func == AggFunc::SUM)
    {
        if(!isNumeric(argType))
            throw std::runtime_error("Aggregate function requires a numeric argument");

        return CatalogDataType::FLOAT;
    }

    if(functionCall.func == AggFunc::MIN || functionCall.func == AggFunc::MAX)
        return argType;

    throw std::runtime_error("Unknown aggregate function");
}

BoundedExpr Binder::bindFunctionCall(
    const FunctionCall& functionCall,
    const TableScope& scope) const
{
    BoundedFunctionCall boundedFunction;
    boundedFunction.func = functionCall.func;
    boundedFunction.star = functionCall.star;
    if(functionCall.star)
    {
        if(functionCall.func != AggFunc::COUNT)
            throw std::runtime_error("Only COUNT supports '*'");

        boundedFunction.type = CatalogDataType::INT;
        return std::make_unique<BoundedFunctionCall>(std::move(boundedFunction));
    }

    if(!functionCall.arg.has_value())
        throw std::runtime_error("Aggregate function requires an argument");

    BoundedExpr argument = bindExpression(functionCall.arg.value(), scope);
    boundedFunction.arg = std::move(argument);
    boundedFunction.type = getFunctionResultType(functionCall, boundedFunction.arg);

    return std::make_unique<BoundedFunctionCall>(std::move(boundedFunction));
}

BoundedExpr Binder::bindExpression(
    const Expr& expr,
    const TableScope& scope) const
{
    switch (expr.index())
    {
        case 0:
            return bindLiteral(std::get<Literal>(expr));

        case 1:
            return bindColumnRef(std::get<ColumnRef>(expr), scope);

        case 2:
            return BoundedAllColumns{};

        case 3:
            return bindBinaryExpr(*std::get<std::unique_ptr<BinaryExpr>>(expr), scope);

        case 4:
            return bindUnaryExpr(*std::get<std::unique_ptr<UnaryExpr>>(expr), scope);

        case 5:
            return bindFunctionCall(*std::get<std::unique_ptr<FunctionCall>>(expr), scope);

        default:
            throw std::runtime_error("Unknown expression type");
    }
}

void Binder::BoundSelectStatement(
    const SelectStatement& selectStmt,
    BoundedSelectStatement& boundedSelectStmt,
    const std::string& txnId)
{
    std::vector<std::string> tableNames;
    for(const auto& table : selectStmt.from)
    {
        tableNames.push_back(table.tableName);
    }

    for(const auto& join : selectStmt.joins)
    {
        tableNames.push_back(join.table.tableName);
    }

    acquireSharedLocks(tableNames, txnId);

    TableScope scope;

    for(const auto& table : selectStmt.from)
    {
        addTableToScope(scope, table);
    }

    for(const auto& join : selectStmt.joins)
    {
        addTableToScope(scope, join.table);
    }

    for(const auto& table : selectStmt.from)
    {
        std::string qualifier = table.alias.value_or(table.tableName);

        BoundedTableSource boundedTable;
        boundedTable.tableId = scope.tables.at(qualifier).tableId;
        boundedTable.alias = qualifier;

        boundedSelectStmt.from.push_back(std::move(boundedTable));
    }

    for(const auto& join : selectStmt.joins)
    {
        BoundedJoinClause boundedJoin;

        std::string qualifier = join.table.alias.value_or(join.table.tableName);

        boundedJoin.type = static_cast<BoundedJoinType>(join.type);

        boundedJoin.table.tableId = scope.tables.at(qualifier).tableId;

        boundedJoin.table.alias = qualifier;

        boundedJoin.on = bindExpression(join.on, scope);

        boundedSelectStmt.joins.push_back(std::move(boundedJoin));
    }

    boundedSelectStmt.distinct = selectStmt.distinct;

    for(const auto& column : selectStmt.columns)
    {
        BoundedSelectColumn boundedColumn;

        boundedColumn.expr = bindExpression(column.expr, scope);

        boundedColumn.alias = column.alias;

        boundedSelectStmt.columns.push_back(std::move(boundedColumn));
    }

    if(selectStmt.where.has_value())
        boundedSelectStmt.where = bindExpression(selectStmt.where.value(), scope);

    for(const auto& expression : selectStmt.groupBy)
    {
        boundedSelectStmt.groupBy.push_back(bindExpression(expression, scope));
    }

    if(selectStmt.having.has_value())
        boundedSelectStmt.having = bindExpression(selectStmt.having.value(), scope);

    for(const auto& order : selectStmt.orderBy)
    {
        BoundedOrderByClause boundedOrder;

        boundedOrder.expr = bindExpression(order.expr, scope);

        boundedOrder.dir = static_cast<BoundedOrderDir>(order.dir);

        boundedSelectStmt.orderBy.push_back(std::move(boundedOrder));
    }

    boundedSelectStmt.limit = selectStmt.limit;
}

void Binder::BoundInsertStatement(
    const InsertStatement& insertStmt,
    BoundedInsertStatement& boundedInsertStmt,
    const std::string& txnId)
{
    acquireSharedLocks({insertStmt.tableName}, txnId);

    TableData table = getTable(insertStmt.tableName);

    boundedInsertStmt.tableId = table.tableId;

    TableScope scope;

    scope.tables.emplace(insertStmt.tableName, table);

    if(insertStmt.columns.has_value())
    {
        std::vector<int> columnIds;

        for(const auto& columnName : insertStmt.columns.value())
        {
            auto column = table.getColumn(columnName);

            if(!column.has_value())
            {
                throw std::runtime_error("Column '" + columnName + 
                    "' does not exist in table '" + 
                    insertStmt.tableName + "'");
            }

            columnIds.push_back(column->ColumnId);
        }

        boundedInsertStmt.columnIds = std::move(columnIds);
    }

    for(const auto& row : insertStmt.values)
    {
        std::vector<BoundedExpr> boundedRow;

        for(const auto& expression : row)
        {
            boundedRow.push_back(bindExpression(expression, scope));
        }

        boundedInsertStmt.values.push_back(std::move(boundedRow));
    }
}

void Binder::BoundUpdateStatement(
    const UpdateStatement& updateStmt,
    BoundedUpdateStatement& boundedUpdateStmt,
    const std::string& txnId)
{
    acquireSharedLocks({updateStmt.tableName}, txnId);

    TableData table = getTable(updateStmt.tableName);

    boundedUpdateStmt.tableId = table.tableId;

    TableScope scope;

    scope.tables.emplace(updateStmt.tableName, table);

    for(const auto& assignment : updateStmt.assignments)
    {
        auto column = table.getColumn(assignment.first);

        if(!column.has_value())
        {
            throw std::runtime_error("Column '" + assignment.first + 
                "' does not exist in table '" + updateStmt.tableName + "'");
        }

        BoundedUpdateAssignment boundedAssignment;

        boundedAssignment.columnId = column->ColumnId;

        boundedAssignment.value = bindExpression(assignment.second, scope);

        boundedUpdateStmt.assignments.push_back(std::move(boundedAssignment));
    }

    if(updateStmt.where.has_value())
    {
        boundedUpdateStmt.where = bindExpression(updateStmt.where.value(), scope);
    }
}

void Binder::BoundDeleteStatement(
    const DeleteStatement& deleteStmt,
    BoundedDeleteStatement& boundedDeleteStmt,
    const std::string& txnId)
{
    acquireSharedLocks({deleteStmt.tableName}, txnId);

    TableData table = getTable(deleteStmt.tableName);

    boundedDeleteStmt.tableId = table.tableId;

    if(deleteStmt.where.has_value())
    {
        TableScope scope;

        scope.tables.emplace(deleteStmt.tableName, table);

        boundedDeleteStmt.where = bindExpression(deleteStmt.where.value(), scope);
    }
}

void Binder::BoundCreateTableStatement(
    const CreateTableStatement& createStmt,
    BoundedCreateTableStatement& boundedCreateStmt,
    const std::string& txnId)
{
    acquireExclusiveLocks({createStmt.tableName}, txnId);

    if(catalog.tableExists(createStmt.tableName) != -1)
    {
        throw std::runtime_error("Table '" + createStmt.tableName + "' already exists");
    }

    boundedCreateStmt.tableName = createStmt.tableName;
    boundedCreateStmt.columns = createStmt.columns;
}

void Binder::BoundCreateIndexStatement(
    const CreateIndexStatement& createStmt,
    BoundedCreateIndexStatement& boundedCreateIndexStmt,
    const std::string& txnId)
{
    acquireExclusiveLocks({createStmt.tableName},txnId);

    if(catalog.indexExists(createStmt.indexName) != -1)
    {
        throw std::runtime_error("Index '" + createStmt.indexName + "' already exists");
    }

    int tableId = catalog.tableExists(createStmt.tableName);

    if(tableId == -1)
    {
        throw std::runtime_error("Table '" + createStmt.tableName + "' does not exist");
    }

    int columnId = catalog.columnExists(createStmt.tableName, createStmt.column);

    if(columnId == -1)
    {
        throw std::runtime_error("Column '" + createStmt.column + 
            "' does not exist in table '" + createStmt.tableName + "'");
    }

    boundedCreateIndexStmt.indexName = createStmt.indexName;
    boundedCreateIndexStmt.tableId = tableId;
    boundedCreateIndexStmt.columnId = columnId;
}

void Binder::BoundDropTableStatement(
    const DropTableStatement& dropStmt,
    BoundedDropTableStatement& boundedDropTableStmt,
    const std::string& txnId)
{
    acquireExclusiveLocks({dropStmt.tableName}, txnId);
    int tableId = catalog.tableExists(dropStmt.tableName);

    if(tableId == -1)
    {
        throw std::runtime_error("Table '" + dropStmt.tableName + "' does not exist");
    }

    boundedDropTableStmt.tableId = tableId;
}

void Binder::BoundDropIndexStatement(
    const DropIndexStatement& dropStmt,
    BoundedDropIndexStatement& boundedDropIndexStmt,
    const std::string& txnId)
{
    auto index = catalog.getIndex(dropStmt.indexName);

    if(!index.has_value())
    {
        throw std::runtime_error("Index '" + dropStmt.indexName + "' does not exist");
    }

    acquireExclusiveLocks({index->tableName}, txnId);

    boundedDropIndexStmt.indexId = index->indexId;
    boundedDropIndexStmt.tableId = index->tableId;
    boundedDropIndexStmt.columnId = index->columnId;
}

void Binder::BoundBeginStatement(
    const BeginStatement& beginStmt,
    BoundedBeginStatement& boundedBeginStmt,
    const std::string& txnId)
{
}

void Binder::BoundCommitStatement(
    const CommitStatement& commitStmt,
    BoundedCommitStatement& boundedCommitStmt,
    const std::string& txnId)
{
}

void Binder::BoundRollbackStatement(
    const RollbackStatement& rollbackStmt,
    BoundedRollbackStatement& boundedRollbackStmt,
    const std::string& txnId)
{
}

BoundStatement Binder::bind(
    const Statement& statement,
    const std::string& txnId)
{
    BoundStatement boundStatement;

    switch(statement.index())
    {
        case 0:
            boundStatement = BoundedSelectStatement{};

            BoundSelectStatement(
                std::get<SelectStatement>(statement),
                std::get<BoundedSelectStatement>(boundStatement),
                txnId
            );
            break;

        case 1:
            boundStatement = BoundedInsertStatement{};

            BoundInsertStatement(
                std::get<InsertStatement>(statement),
                std::get<BoundedInsertStatement>(boundStatement),
                txnId
            );
            break;

        case 2:
            boundStatement = BoundedUpdateStatement{};

            BoundUpdateStatement(
                std::get<UpdateStatement>(statement),
                std::get<BoundedUpdateStatement>(boundStatement),
                txnId
            );
            break;

        case 3:
            boundStatement = BoundedDeleteStatement{};

            BoundDeleteStatement(
                std::get<DeleteStatement>(statement),
                std::get<BoundedDeleteStatement>(boundStatement),
                txnId
            );
            break;

        case 4:
            boundStatement = BoundedCreateTableStatement{};

            BoundCreateTableStatement(
                std::get<CreateTableStatement>(statement),
                std::get<BoundedCreateTableStatement>(boundStatement),
                txnId
            );
            break;

        case 5:
            boundStatement = BoundedCreateIndexStatement{};

            BoundCreateIndexStatement(
                std::get<CreateIndexStatement>(statement),
                std::get<BoundedCreateIndexStatement>(boundStatement),
                txnId
            );
            break;

        case 6:
            boundStatement = BoundedDropTableStatement{};

            BoundDropTableStatement(
                std::get<DropTableStatement>(statement),
                std::get<BoundedDropTableStatement>(boundStatement),
                txnId
            );
            break;

        case 7:
            boundStatement = BoundedDropIndexStatement{};

            BoundDropIndexStatement(
                std::get<DropIndexStatement>(statement),
                std::get<BoundedDropIndexStatement>(boundStatement),
                txnId
            );
            break;

        case 8:
            boundStatement = BoundedBeginStatement{};

            BoundBeginStatement(
                std::get<BeginStatement>(statement),
                std::get<BoundedBeginStatement>(boundStatement),
                txnId
            );
            break;

        case 9:
            boundStatement = BoundedCommitStatement{};

            BoundCommitStatement(
                std::get<CommitStatement>(statement),
                std::get<BoundedCommitStatement>(boundStatement),
                txnId
            );
            break;

        case 10:
            boundStatement = BoundedRollbackStatement{};

            BoundRollbackStatement(
                std::get<RollbackStatement>(statement),
                std::get<BoundedRollbackStatement>(boundStatement),
                txnId
            );
            break;

        default:
            throw std::runtime_error("Unknown statement type");
    }

    return boundStatement;
}