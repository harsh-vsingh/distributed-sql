#include "parser.h"
#include <stdexcept>
#include <utility>

// Helper functions
void Parser::setTokens(std::vector<Token>& newTokens)
{
    tokens = newTokens;
    pos = 0;
}

Token& Parser::peek()
{
    return tokens[pos];
}

Token& Parser::advance()
{
    Token& current = tokens[pos];

    if(current.type != TokenType::END_OF_FILE)
        pos++;

    return current;
}

bool Parser::check(TokenType type)
{
    return pos < tokens.size() && tokens[pos].type == type;
}

bool Parser::match(TokenType type)
{
    if(check(type))
    {
        advance();
        return true;
    }
    return false;
}

Token& Parser::expect(TokenType type, const std::string& message)
{
    if (check(type))
        return advance();

    const Token& token = peek();

    throw std::runtime_error(
        "Error at line " + std::to_string(token.line) +
        ", column " + std::to_string(token.col) +
        ": " + message
    );
}

// Expression parsing
Expr Parser::parseAggregateFunctionCall()
{
    Token funcToken = tokens[pos - 1];
    AggFunc func;

    switch(funcToken.type)
    {
        case TokenType::COUNT:
            func = AggFunc::COUNT;
            break;
        case TokenType::SUM:
            func = AggFunc::SUM;
            break;
        case TokenType::AVG:
            func = AggFunc::AVG;
            break;
        case TokenType::MIN:
            func = AggFunc::MIN;
            break;
        case TokenType::MAX:
            func = AggFunc::MAX;
            break;
    }

    expect(TokenType::LPAREN, "expected a left parenthesis");

    bool star = false;
    std::optional<Expr> arg;

    if(match(TokenType::STAR))
    {
        star = true;
    }
    else if(check(TokenType::IDENTIFIER))
    {
        arg = parseExpression();
    }
    else
    {
        throw std::runtime_error("Expected a identifier at position" + pos);
    }

    expect(TokenType::RPAREN, "expected closing parenthesis");

    FunctionCall call{func, star, std::move(arg)};
    return std::make_unique<FunctionCall>(std::move(call));
}

Expr Parser::parsePrimaryExpression()
{
    if(match(TokenType::INTEGER))
    {
        int value = std::stoll(tokens[pos - 1].value);
        return Literal{value};
    }
    else if(match(TokenType::FLOAT))
    {
        double value = std::stod(tokens[pos - 1].value);
        return Literal{value};
    }
    else if(match(TokenType::STRING))
    {
        return Literal{tokens[pos - 1].value};
    }
    else if(match(TokenType::TRUE_LIT))
    {
        return Literal{true};
    }
    else if(match(TokenType::FALSE_LIT))
    {
        return Literal{false};
    }
    else if(match(TokenType::NULL_LIT))
    {
        return Literal{nullptr};
    }
    else if(match(TokenType::IDENTIFIER))
    {
        std::string columnName = tokens[pos - 1].value;
        std::string tableName = "";

        if(match(TokenType::DOT))
        {
            tableName = columnName;
            columnName = expect(TokenType::IDENTIFIER, "Expected column name after '.'").value;
        }

        return ColumnRef{tableName, columnName};
    }
    else if(match(TokenType::LPAREN))
    {
        Expr expr = parseExpression();
        expect(TokenType::RPAREN, "Expected ')' after expression");
        return expr;
    }
    else if(match(TokenType::COUNT) || match(TokenType::SUM) || match(TokenType::AVG) ||
            match(TokenType::MIN) || match(TokenType::MAX))
    {
        return parseAggregateFunctionCall();
    }
    else if(match(TokenType::STAR))
    {
        return AllColumns{};
    }

    throw std::runtime_error("Unexpected token: " + peek().value);
}

Expr Parser::parseUnaryExpression()
{
    if(match(TokenType::NOT) || match(TokenType::MINUS))
    {
        Token opToken = tokens[pos - 1];
        Expr operand = parseUnaryExpression();

        UnaryOp op;
        switch(opToken.type)
        {
            case TokenType::NOT:
                op = UnaryOp::NOT;
                break;
            case TokenType::MINUS:
                op = UnaryOp::NEGATE;
                break;
            default:
                throw std::runtime_error("Unexpected unary operator");
        }
        return std::make_unique<UnaryExpr>(UnaryExpr{op, std::move(operand)});
    }
    return parsePrimaryExpression();
}

Expr Parser::parseMultiplicativeExpression()
{
    Expr left = parseUnaryExpression();

    while(match(TokenType::STAR) || match(TokenType::SLASH))
    {
        Token opToken = tokens[pos - 1];
        Expr right = parseUnaryExpression();

        BinaryOp op;
        switch(opToken.type)
        {
            case TokenType::STAR:
                op = BinaryOp::STAR;
                break;
            case TokenType::SLASH:
                op = BinaryOp::SLASH;
                break;
            default:
                throw std::runtime_error("Unexpected multiplicative operator");
        }
        left = std::make_unique<BinaryExpr>(BinaryExpr{std::move(left), op, std::move(right)});
    }
    return left;
}

Expr Parser::parseAdditiveExpression()
{
    Expr left = parseMultiplicativeExpression();

    while(match(TokenType::PLUS) || match(TokenType::MINUS))
    {
        Token opToken = tokens[pos - 1];
        Expr right = parseMultiplicativeExpression();

        BinaryOp op;
        switch(opToken.type)
        {
            case TokenType::PLUS:
                op = BinaryOp::PLUS;
                break;
            case TokenType::MINUS:
                op = BinaryOp::MINUS;
                break;
            default:
                throw std::runtime_error("Unexpected additive operator");
        }
        left = std::make_unique<BinaryExpr>(BinaryExpr{std::move(left), op, std::move(right)});
    }
    return left;
}

Expr Parser::parseComparisonExpression()
{
    Expr left = parseAdditiveExpression();

    while(match(TokenType::EQ) || match(TokenType::NEQ) || match(TokenType::LT) ||
          match(TokenType::GT) || match(TokenType::LTE) || match(TokenType::GTE))
    {
        Token opToken = tokens[pos - 1];
        Expr right = parseAdditiveExpression();

        BinaryOp op;
        switch(opToken.type)
        {
            case TokenType::EQ:
                op = BinaryOp::EQ;
                break;
            case TokenType::NEQ:
                op = BinaryOp::NEQ;
                break;
            case TokenType::LT:
                op = BinaryOp::LT;
                break;
            case TokenType::GT:
                op = BinaryOp::GT;
                break;
            case TokenType::LTE:
                op = BinaryOp::LTE;
                break;
            case TokenType::GTE:
                op = BinaryOp::GTE;
                break;
            default:
                throw std::runtime_error("Unexpected comparison operator");
        }
        left = std::make_unique<BinaryExpr>(BinaryExpr{std::move(left), op, std::move(right)});
    }
    return left;
}

Expr Parser::parseLogicalAndExpression()
{
    Expr left = parseComparisonExpression();
    while(match(TokenType::AND))
    {
        Expr right = parseComparisonExpression();
        left = std::make_unique<BinaryExpr>(BinaryExpr{std::move(left), BinaryOp::AND, std::move(right)});
    }
    return left;
}

Expr Parser::parseLogicalOrExpression()
{
    Expr left = parseLogicalAndExpression();
    while(match(TokenType::OR))
    {
        Expr right = parseLogicalAndExpression();
        left = std::make_unique<BinaryExpr>(BinaryExpr{std::move(left), BinaryOp::OR, std::move(right)});
    }
    return left;
}

Expr Parser::parseExpression()
{
    return parseLogicalOrExpression();
}

// Statement parsing
void Parser::parseSelectStatement(SelectStatement& selectStmt)
{
    std::vector<SelectColumn> columns;

    if(match(TokenType::DISTINCT))
    {
        selectStmt.distinct = true;
    }

    while(true)
    {
        Expr expr = parseExpression();
        std::optional<std::string> alias;

        if(match(TokenType::AS))
        {
            alias = expect(TokenType::IDENTIFIER, "expected alias after AS").value;
        }

        columns.push_back(SelectColumn{std::move(expr), std::move(alias)});

        if(!match(TokenType::COMMA))
            break;
    }
    selectStmt.columns = std::move(columns);

    expect(TokenType::FROM, "expected FROM after column list");

    std::vector<TableSource> from;
    while(true)
    {
        std::string tableName = expect(TokenType::IDENTIFIER, "expected table name after FROM").value;
        std::optional<std::string> alias;

        if(match(TokenType::AS))
        {
            alias = expect(TokenType::IDENTIFIER, "expected alias after AS").value;
        }

        from.push_back(TableSource{tableName, std::move(alias)});

        if(!match(TokenType::COMMA))
            break;
    }
    selectStmt.from = std::move(from);

    std::vector<JoinClause> joins;
    while(match(TokenType::JOIN) || match(TokenType::INNER) || 
       match(TokenType::LEFT) || match(TokenType::RIGHT))
    {
        JoinType joinType;
        if(tokens[pos - 1].type == TokenType::JOIN)
            joinType = JoinType::INNER;
        else if(tokens[pos - 1].type == TokenType::INNER)
        {
            expect(TokenType::JOIN, "expected JOIN after INNER");
            joinType = JoinType::INNER;
        }
        else if(tokens[pos - 1].type == TokenType::LEFT)
        {
            expect(TokenType::JOIN, "expected JOIN after LEFT");
            joinType = JoinType::LEFT;
        }
        else if(tokens[pos - 1].type == TokenType::RIGHT)
        {
            expect(TokenType::JOIN, "expected JOIN after RIGHT");
            joinType = JoinType::RIGHT;
        }

        expect(TokenType::IDENTIFIER, "expected table name after JOIN");
        std::string tableName = tokens[pos - 1].value;

        TableSource tableSource;
        if(match(TokenType::AS))
        {
            std::string alias = expect(TokenType::IDENTIFIER, "expected alias after AS").value;
            tableSource = TableSource{tableName, alias};
        }
        else
        {
            tableSource = TableSource{tableName, std::nullopt};
        }

        expect(TokenType::ON, "expected ON after table name in JOIN clause");
        Expr onExpr = parseExpression();

        JoinClause joinClause{joinType, std::move(tableSource), std::move(onExpr)};
        joins.push_back(std::move(joinClause));
    }
    selectStmt.joins = std::move(joins);
    
    if(match(TokenType::WHERE))
    {
        selectStmt.where = parseExpression();
    }

    if(match(TokenType::GROUP))
    {
        expect(TokenType::BY, "expected BY after GROUP");
        std::vector<Expr> groupBy;

        while(true)
        {
            groupBy.push_back(parseExpression());
            if(!match(TokenType::COMMA))
                break;
        }
        selectStmt.groupBy = std::move(groupBy);
    }

    if(match(TokenType::HAVING))
    {
        selectStmt.having = parseExpression();
    }

    if(match(TokenType::ORDER))
    {
        expect(TokenType::BY, "expected BY after ORDER");
        std::vector<OrderByClause> orderBy;

        while(true)
        {
            Expr expr = parseExpression();
            OrderDir dir = OrderDir::ASC;

            if(match(TokenType::ASC))
                dir = OrderDir::ASC;
            else if(match(TokenType::DESC))
                dir = OrderDir::DESC;

            orderBy.push_back(OrderByClause{std::move(expr), dir});

            if(!match(TokenType::COMMA))
                break;
        }
        selectStmt.orderBy = std::move(orderBy);
    }

    if(match(TokenType::LIMIT))
    {
        int limitValue = std::stoi(expect(TokenType::INTEGER, "expected integer after LIMIT").value);
        selectStmt.limit = limitValue;
    }

    expect(TokenType::SEMICOLON, "expected ';' after SELECT statement");
    return;
}

void Parser::parseInsertStatement(InsertStatement& insertStmt)
{
    expect(TokenType::INTO, "expected INTO after INSERT");
    insertStmt.tableName = expect(TokenType::IDENTIFIER, "expected table name after INTO").value;

    std::vector<std::string> columns;
    if(match(TokenType::LPAREN))
    {
        while(true)
        {
            columns.push_back(expect(TokenType::IDENTIFIER, "expected column name").value);
            if(!match(TokenType::COMMA))
                break;
        }
        expect(TokenType::RPAREN, "expected ')' after column list");
    }
    insertStmt.columns = std::move(columns);

    expect(TokenType::VALUES, "expected VALUES after table name and columns");
  
    std::vector<std::vector<Expr>> values;

    while(true)
    {
        expect(TokenType::LPAREN, "expected a '(' ");
        std::vector<Expr> expressions;

        while(true)
        {
            expressions.push_back(parseExpression());
            if(!match(TokenType::COMMA))
            {
                break;
            }
        }

        expect(TokenType::RPAREN, "expected a ')'");
        values.push_back(std::move(expressions));
        
        if(!match(TokenType::COMMA))
        {
            break;
        }
    }
    expect(TokenType::SEMICOLON, "expected a ; after INSERT");
    insertStmt.values = std::move(values);

    return;
}

void Parser::parseUpdateStatement(UpdateStatement& updateStmt)
{
    updateStmt.tableName = expect(TokenType::IDENTIFIER, "expected table name").value;
    expect(TokenType::SET, "expected SET after table name");

    std::vector<std::pair<std::string, Expr>> assignments;

    while(true)
    {
        expect(TokenType::IDENTIFIER, "expected column name");
        std::string columnName = tokens[pos - 1].value;

        expect(TokenType::EQ, "expected '=' after column name");
        Expr value = parseExpression();

        assignments.push_back({columnName, std::move(value)});

        if(!match(TokenType::COMMA))
            break;
    }
    updateStmt.assignments = std::move(assignments);

    if(match(TokenType::WHERE))
        updateStmt.where = parseExpression();

    return;
}

void Parser::parseDeleteStatement(DeleteStatement& deleteStmt)
{
    expect(TokenType::FROM, "expected FROM after DELETE");
    deleteStmt.tableName = expect(TokenType::IDENTIFIER, "expected table name").value;

    if(match(TokenType::WHERE))
    {
        deleteStmt.where = parseExpression();
    }

    expect(TokenType::SEMICOLON, "expected ';' after DELETE statement");
    
    return;
}

void Parser::parseCreateTableStatement(CreateTableStatement& createStmt)
{
    createStmt.tableName = expect(TokenType::IDENTIFIER, "Expected table name after CREATE TABLE").value;
    expect(TokenType::LPAREN, "Expected '(' after table name");

    while(peek().type != TokenType::RPAREN)
    {
        ColumnDef column;
        column.name = expect(TokenType::IDENTIFIER, "Expected column name").value;

        if(match(TokenType::INT))
            column.type = IntType{};
        else if(match(TokenType::BOOL))
            column.type = BoolType{};
        else if(match(TokenType::FLOAT_TYPE))
            column.type = FloatType{};
        else if(match(TokenType::VARCHAR))
        {
            expect(TokenType::LPAREN, "Expected '(' after VARCHAR");
            int length = std::stoi(expect(TokenType::INTEGER, "Expected length for VARCHAR").value);
            expect(TokenType::RPAREN, "Expected ')' after VARCHAR length");
            column.type = VarCharType{length};
        }
        else if(match(TokenType::TIMESTAMP))
            column.type = TimestampType{};
        else
            throw std::runtime_error("Expected data type for column " + column.name);

        while(peek().type != TokenType::COMMA && peek().type != TokenType::RPAREN)
        {
            if(match(TokenType::PRIMARY))
            {
                expect(TokenType::KEY, "Expected KEY after PRIMARY");
                column.primaryKey = true;
            }
            else if(match(TokenType::NOT))
            {
                expect(TokenType::NULL_LIT, "Expected NULL after NOT");
                column.nullable = false;
            }
            else if(match(TokenType::UNIQUE))
            {
                column.unique = true;
            }
            else
            {
                throw std::runtime_error("Unexpected token in column definition: " + peek().value);
            }
        }

        createStmt.columns.push_back(std::move(column));

        if(peek().type == TokenType::COMMA)
            advance();
    }

    expect(TokenType::RPAREN, "Expected ')' after column definitions");
    expect(TokenType::SEMICOLON, "Expected ';' after CREATE TABLE statement");
    return;
}

void Parser::parseCreateIndexStatement(CreateIndexStatement& createStmt)
{
    createStmt.indexName = expect(TokenType::IDENTIFIER, "Expected index name after CREATE INDEX").value;
    expect(TokenType::ON, "Expected ON after index name");

    createStmt.tableName = expect(TokenType::IDENTIFIER, "Expected table name after ON").value;
    expect(TokenType::LPAREN, "Expected '(' after table name");

    createStmt.column = expect(TokenType::IDENTIFIER, "Expected column name after '('").value;
    expect(TokenType::RPAREN, "Expected ')' after column name");
    expect(TokenType::SEMICOLON, "Expected ';' after CREATE INDEX statement");

    return;
}

void Parser::parseDropTableStatement(DropTableStatement& dropStmt)
{
    dropStmt.tableName = expect(TokenType::IDENTIFIER, "Expected table name after DROP TABLE").value;
    expect(TokenType::SEMICOLON, "Expected ';' after DROP TABLE statement");
    return;
}

void Parser::parseDropIndexStatement(DropIndexStatement& dropStmt)
{
    dropStmt.indexName = expect(TokenType::IDENTIFIER, "Expected index name after DROP INDEX").value;
    expect(TokenType::SEMICOLON, "Expected ';' after DROP INDEX statement");
    return;
}

void Parser::parseBeginStatement(BeginStatement& beginStmt)
{
    expect(TokenType::SEMICOLON, "Expected ';' after BEGIN statement");
    return;
}

void Parser::parseCommitStatement(CommitStatement& commitStmt)
{
    expect(TokenType::SEMICOLON, "Expected ';' after COMMIT statement");
    return;
}

void Parser::parseRollbackStatement(RollbackStatement& rollbackStmt)
{
    expect(TokenType::SEMICOLON, "Expected ';' after ROLLBACK statement");
    return;
}


Statement Parser::parse()
{
    if(pos >= tokens.size())
        throw std::runtime_error("No more tokens to parse");

    Statement parseStatement;

    switch(peek().type)
    {
        case TokenType::SELECT:
            parseStatement = SelectStatement{};
            advance();
            parseSelectStatement(std::get<SelectStatement>(parseStatement));
            break;

        case TokenType::INSERT:
            parseStatement = InsertStatement{};
            advance();
            parseInsertStatement(std::get<InsertStatement>(parseStatement));
            break;

        case TokenType::UPDATE:
            parseStatement = UpdateStatement{};
            advance();
            parseUpdateStatement(std::get<UpdateStatement>(parseStatement));
            break;

        case TokenType::DELETE:
            parseStatement = DeleteStatement{};
            advance();
            parseDeleteStatement(std::get<DeleteStatement>(parseStatement));
            break;

        case TokenType::CREATE:
            advance();
            if(check(TokenType::TABLE))
            {
                parseStatement = CreateTableStatement{};
                advance();
                parseCreateTableStatement(std::get<CreateTableStatement>(parseStatement));
            }
            else if(check(TokenType::INDEX))
            {
                parseStatement = CreateIndexStatement{};
                advance();
                parseCreateIndexStatement(std::get<CreateIndexStatement>(parseStatement));
            }
            else
                throw std::runtime_error("Expected TABLE or INDEX after CREATE");
            break;

        case TokenType::DROP:
            advance();
            if(check(TokenType::TABLE))
            {
                parseStatement = DropTableStatement{};
                advance();
                parseDropTableStatement(std::get<DropTableStatement>(parseStatement));
            }
            else if(check(TokenType::INDEX))
            {
                parseStatement = DropIndexStatement{};
                advance();
                parseDropIndexStatement(std::get<DropIndexStatement>(parseStatement));
            }
            else
                throw std::runtime_error("Expected TABLE or INDEX after DROP");
            break;

        case TokenType::BEGIN:
            parseStatement = BeginStatement{};
            advance();
            parseBeginStatement(std::get<BeginStatement>(parseStatement));
            break;

        case TokenType::COMMIT:
            parseStatement = CommitStatement{};
            advance();
            parseCommitStatement(std::get<CommitStatement>(parseStatement));
            break;

        case TokenType::ROLLBACK:
            parseStatement = RollbackStatement{};
            advance();
            parseRollbackStatement(std::get<RollbackStatement>(parseStatement));
            break;

        default:
            throw std::runtime_error("Unexpected token: " + peek().value);
    }

    return parseStatement;
}

std::vector<Statement> Parser::parseAll()
{
    std::vector<Statement> statements;

    while (peek().type != TokenType::END_OF_FILE)
    {
        statements.push_back(parse());
    }

    return statements;
}