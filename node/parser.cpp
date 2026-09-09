#include "parser.h"
#include <stdexcept>
#include <utility>

// Helper functions
const Token& Parser::peek(const std::vector<Token>& tokens, 
    std::size_t& pos) const
{
    return tokens[pos];
}

const Token& Parser::advance(const std::vector<Token>& tokens, 
    std::size_t& pos) const
{
    const Token& current = tokens[pos];

    if(current.type != TokenType::END_OF_FILE)
        pos++;

    return current;
}

bool Parser::check(TokenType type, const std::vector<Token>& tokens, 
    std::size_t& pos) const
{
    return pos < tokens.size() && tokens[pos].type == type;
}

bool Parser::match(TokenType type, const std::vector<Token>& tokens, 
    std::size_t& pos) const
{
    if(check(type, tokens, pos))
    {
        advance(tokens, pos);
        return true;
    }
    return false;
}

const Token& Parser::expect(TokenType type, const std::string& message, 
    const std::vector<Token>& tokens, std::size_t& pos) const
{
    if(check(type, tokens, pos))
        return advance(tokens, pos);

    const Token& token = peek(tokens, pos);

    throw std::runtime_error(
        "Error at line " + std::to_string(token.line) +
        ", column " + std::to_string(token.col) +
        ": " + message
    );
}

// Expression parsing
Expr Parser::parseAggregateFunctionCall(const std::vector<Token>& tokens, 
    std::size_t& pos) const
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

    expect(TokenType::LPAREN, "expected a left parenthesis", tokens, pos);

    bool star = false;
    std::optional<Expr> arg;

    if(match(TokenType::STAR, tokens, pos))
    {
        star = true;
    }
    else if(check(TokenType::IDENTIFIER, tokens, pos))
    {
        arg = parseExpression(tokens, pos);
    }
    else
    {
        throw std::runtime_error("Expected a identifier at position" + std::to_string(pos));
    }

    expect(TokenType::RPAREN, "expected closing parenthesis", tokens, pos);

    FunctionCall call{func, star, std::move(arg)};
    return std::make_unique<FunctionCall>(std::move(call));
}

Expr Parser::parsePrimaryExpression(const std::vector<Token>& tokens, 
    std::size_t& pos) const
{
    if(match(TokenType::INTEGER, tokens, pos))
    {
        int value = std::stoll(tokens[pos - 1].value);
        return Literal{value};
    }
    else if(match(TokenType::FLOAT, tokens, pos))
    {
        double value = std::stod(tokens[pos - 1].value);
        return Literal{value};
    }
    else if(match(TokenType::STRING, tokens, pos))
    {
        return Literal{tokens[pos - 1].value};
    }
    else if(match(TokenType::TRUE_LIT, tokens, pos))
    {
        return Literal{true};
    }
    else if(match(TokenType::FALSE_LIT, tokens, pos))
    {
        return Literal{false};
    }
    else if(match(TokenType::NULL_LIT, tokens, pos))
    {
        return Literal{nullptr};
    }
    else if(match(TokenType::IDENTIFIER, tokens, pos))
    {
        std::string columnName = tokens[pos - 1].value;
        std::string tableName = "";

        if(match(TokenType::DOT, tokens, pos))
        {
            tableName = columnName;
            columnName = expect(TokenType::IDENTIFIER, 
                "Expected column name after '.'", tokens, pos).value;
        }

        return ColumnRef{tableName, columnName};
    }
    else if(match(TokenType::LPAREN, tokens, pos))
    {
        Expr expr = parseExpression(tokens, pos);
        expect(TokenType::RPAREN, "Expected ')' after expression", tokens, pos);
        return expr;
    }
    else if(match(TokenType::COUNT, tokens, pos) || match(TokenType::SUM, tokens, pos) || 
            match(TokenType::AVG, tokens, pos) || match(TokenType::MIN, tokens, pos) || 
            match(TokenType::MAX, tokens, pos))
    {
        return parseAggregateFunctionCall(tokens, pos);
    }
    else if(match(TokenType::STAR, tokens, pos))
    {
        return AllColumns{};
    }

    throw std::runtime_error("Unexpected token: " + peek(tokens, pos).value);
}

Expr Parser::parseUnaryExpression(const std::vector<Token>& tokens, 
    std::size_t& pos) const
{
    if(match(TokenType::NOT, tokens, pos) || match(TokenType::MINUS, tokens, pos))
    {
        Token opToken = tokens[pos - 1];
        Expr operand = parseUnaryExpression(tokens, pos);

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
    return parsePrimaryExpression(tokens, pos);
}

Expr Parser::parseMultiplicativeExpression(const std::vector<Token>& tokens, 
    std::size_t& pos) const
{
    Expr left = parseUnaryExpression(tokens, pos);

    while(match(TokenType::STAR, tokens, pos) || match(TokenType::SLASH, tokens, pos))
    {
        Token opToken = tokens[pos - 1];
        Expr right = parseUnaryExpression(tokens, pos);

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

Expr Parser::parseAdditiveExpression(const std::vector<Token>& tokens, 
    std::size_t& pos) const
{
    Expr left = parseMultiplicativeExpression(tokens, pos);

    while(match(TokenType::PLUS, tokens, pos) || match(TokenType::MINUS, tokens, pos))
    {
        Token opToken = tokens[pos - 1];
        Expr right = parseMultiplicativeExpression(tokens, pos);

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

Expr Parser::parseComparisonExpression(const std::vector<Token>& tokens, 
    std::size_t& pos) const
{
    Expr left = parseAdditiveExpression(tokens, pos);

    while(match(TokenType::EQ, tokens, pos) || match(TokenType::NEQ, tokens, pos) || 
        match(TokenType::LT, tokens, pos) || match(TokenType::GT, tokens, pos) || 
        match(TokenType::LTE, tokens, pos) || match(TokenType::GTE, tokens, pos))
    {
        Token opToken = tokens[pos - 1];
        Expr right = parseAdditiveExpression(tokens, pos);

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

Expr Parser::parseLogicalAndExpression(const std::vector<Token>& tokens, 
    std::size_t& pos) const
{
    Expr left = parseComparisonExpression(tokens, pos);
    while(match(TokenType::AND, tokens, pos))
    {
        Expr right = parseComparisonExpression(tokens, pos);
        left = std::make_unique<BinaryExpr>(BinaryExpr{std::move(left), BinaryOp::AND, std::move(right)});
    }
    return left;
}

Expr Parser::parseLogicalOrExpression(const std::vector<Token>& tokens, 
    std::size_t& pos) const
{
    Expr left = parseLogicalAndExpression(tokens, pos);
    while(match(TokenType::OR, tokens, pos))
    {
        Expr right = parseLogicalAndExpression(tokens, pos);
        left = std::make_unique<BinaryExpr>(BinaryExpr{std::move(left), BinaryOp::OR, std::move(right)});
    }
    return left;
}

Expr Parser::parseExpression(const std::vector<Token>& tokens, 
    std::size_t& pos) const
{
    return parseLogicalOrExpression(tokens, pos);
}

// Statement parsing
void Parser::parseSelectStatement(SelectStatement& selectStmt, 
    const std::vector<Token>& tokens, std::size_t& pos) const
{
    std::vector<SelectColumn> columns;

    if(match(TokenType::DISTINCT, tokens, pos))
    {
        selectStmt.distinct = true;
    }

    while(true)
    {
        Expr expr = parseExpression(tokens, pos);
        std::optional<std::string> alias;

        if(match(TokenType::AS, tokens, pos))
        {
            alias = expect(TokenType::IDENTIFIER, 
                "expected alias after AS", tokens, pos).value;
        }

        columns.push_back(SelectColumn{std::move(expr), std::move(alias)});

        if(!match(TokenType::COMMA, tokens, pos))
            break;
    }
    selectStmt.columns = std::move(columns);

    expect(TokenType::FROM, "expected FROM after column list", tokens, pos);

    std::vector<TableSource> from;
    while(true)
    {
        std::string tableName = expect(TokenType::IDENTIFIER, 
            "expected table name after FROM", tokens, pos).value;
        std::optional<std::string> alias;

        if(match(TokenType::AS, tokens, pos))
        {
            alias = expect(TokenType::IDENTIFIER, 
                "expected alias after AS", tokens, pos).value;
        }

        from.push_back(TableSource{tableName, std::move(alias)});

        if(!match(TokenType::COMMA, tokens, pos))
            break;
    }
    selectStmt.from = std::move(from);

    std::vector<JoinClause> joins;
    while(match(TokenType::JOIN, tokens, pos) || match(TokenType::INNER, tokens, pos) || 
       match(TokenType::LEFT, tokens, pos) || match(TokenType::RIGHT, tokens, pos))
    {
        JoinType joinType;
        if(tokens[pos - 1].type == TokenType::JOIN)
            joinType = JoinType::INNER;
        else if(tokens[pos - 1].type == TokenType::INNER)
        {
            expect(TokenType::JOIN, "expected JOIN after INNER", tokens, pos);
            joinType = JoinType::INNER;
        }
        else if(tokens[pos - 1].type == TokenType::LEFT)
        {
            expect(TokenType::JOIN, "expected JOIN after LEFT", tokens, pos);
            joinType = JoinType::LEFT;
        }
        else if(tokens[pos - 1].type == TokenType::RIGHT)
        {
            expect(TokenType::JOIN, "expected JOIN after RIGHT", tokens, pos);
            joinType = JoinType::RIGHT;
        }

        expect(TokenType::IDENTIFIER, "expected table name after JOIN", tokens, pos);
        std::string tableName = tokens[pos - 1].value;

        TableSource tableSource;
        if(match(TokenType::AS, tokens, pos))
        {
            std::string alias = expect(TokenType::IDENTIFIER, 
                "expected alias after AS", tokens, pos).value;
            tableSource = TableSource{tableName, alias};
        }
        else
        {
            tableSource = TableSource{tableName, std::nullopt};
        }

        expect(TokenType::ON, "expected ON after table name in JOIN clause", tokens, pos);
        Expr onExpr = parseExpression(tokens, pos);

        JoinClause joinClause{joinType, std::move(tableSource), std::move(onExpr)};
        joins.push_back(std::move(joinClause));
    }
    selectStmt.joins = std::move(joins);
    
    if(match(TokenType::WHERE, tokens, pos))
    {
        selectStmt.where = parseExpression(tokens, pos);
    }

    if(match(TokenType::GROUP, tokens, pos))
    {
        expect(TokenType::BY, "expected BY after GROUP", tokens, pos);
        std::vector<Expr> groupBy;

        while(true)
        {
            groupBy.push_back(parseExpression(tokens, pos));
            if(!match(TokenType::COMMA, tokens, pos))
                break;
        }
        selectStmt.groupBy = std::move(groupBy);
    }

    if(match(TokenType::HAVING, tokens, pos))
    {
        selectStmt.having = parseExpression(tokens, pos);
    }

    if(match(TokenType::ORDER, tokens, pos))
    {
        expect(TokenType::BY, "expected BY after ORDER", tokens, pos);
        std::vector<OrderByClause> orderBy;

        while(true)
        {
            Expr expr = parseExpression(tokens, pos);
            OrderDir dir = OrderDir::ASC;

            if(match(TokenType::ASC, tokens, pos))
                dir = OrderDir::ASC;
            else if(match(TokenType::DESC, tokens, pos))
                dir = OrderDir::DESC;

            orderBy.push_back(OrderByClause{std::move(expr), dir});

            if(!match(TokenType::COMMA, tokens, pos))
                break;
        }
        selectStmt.orderBy = std::move(orderBy);
    }

    if(match(TokenType::LIMIT, tokens, pos))
    {
        int limitValue = std::stoi(expect(TokenType::INTEGER, 
            "expected integer after LIMIT", tokens, pos).value);
        selectStmt.limit = limitValue;
    }

    expect(TokenType::SEMICOLON, "expected ';' after SELECT statement", tokens, pos);
    return;
}

void Parser::parseInsertStatement(InsertStatement& insertStmt, 
    const std::vector<Token>& tokens, std::size_t& pos) const
{
    expect(TokenType::INTO, "expected INTO after INSERT", tokens, pos);
    insertStmt.tableName = expect(TokenType::IDENTIFIER, 
        "expected table name after INTO", tokens, pos).value;

    std::vector<std::string> columns;
    if(match(TokenType::LPAREN, tokens, pos))
    {
        while(true)
        {
            columns.push_back(expect(TokenType::IDENTIFIER, 
                "expected column name", tokens, pos).value);
            if(!match(TokenType::COMMA, tokens, pos))
                break;
        }
        expect(TokenType::RPAREN, "expected ')' after column list", tokens, pos);
    }
    insertStmt.columns = std::move(columns);

    expect(TokenType::VALUES, "expected VALUES after table name and columns", tokens, pos);
  
    std::vector<std::vector<Expr>> values;

    while(true)
    {
        expect(TokenType::LPAREN, "expected a '(' ", tokens, pos);
        std::vector<Expr> expressions;

        while(true)
        {
            expressions.push_back(parseExpression(tokens, pos));
            if(!match(TokenType::COMMA, tokens, pos))
            {
                break;
            }
        }

        expect(TokenType::RPAREN, "expected a ')'", tokens, pos);
        values.push_back(std::move(expressions));
        
        if(!match(TokenType::COMMA, tokens, pos))
        {
            break;
        }
    }
    expect(TokenType::SEMICOLON, "expected a ; after INSERT", tokens, pos);
    insertStmt.values = std::move(values);

    return;
}

void Parser::parseUpdateStatement(UpdateStatement& updateStmt, 
    const std::vector<Token>& tokens, std::size_t& pos) const
{
    updateStmt.tableName = expect(TokenType::IDENTIFIER, 
        "expected table name", tokens, pos).value;
    expect(TokenType::SET, "expected SET after table name", tokens, pos);

    std::vector<std::pair<std::string, Expr>> assignments;

    while(true)
    {
        expect(TokenType::IDENTIFIER, "expected column name", tokens, pos);
        std::string columnName = tokens[pos - 1].value;

        expect(TokenType::EQ, "expected '=' after column name", tokens, pos);
        Expr value = parseExpression(tokens, pos);

        assignments.push_back({columnName, std::move(value)});

        if(!match(TokenType::COMMA, tokens, pos))
            break;
    }
    updateStmt.assignments = std::move(assignments);

    if(match(TokenType::WHERE, tokens, pos))
        updateStmt.where = parseExpression(tokens, pos);

    return;
}

void Parser::parseDeleteStatement(DeleteStatement& deleteStmt, 
    const std::vector<Token>& tokens, std::size_t& pos) const
{
    expect(TokenType::FROM, "expected FROM after DELETE", tokens, pos);
    deleteStmt.tableName = expect(TokenType::IDENTIFIER, 
        "expected table name", tokens, pos).value;

    if(match(TokenType::WHERE, tokens, pos))
    {
        deleteStmt.where = parseExpression(tokens, pos);
    }

    expect(TokenType::SEMICOLON, "expected ';' after DELETE statement", tokens, pos);
    
    return;
}

void Parser::parseCreateTableStatement(CreateTableStatement& createStmt, 
    const std::vector<Token>& tokens, std::size_t& pos) const
{
    createStmt.tableName = expect(TokenType::IDENTIFIER, 
        "Expected table name after CREATE TABLE", tokens, pos).value;
    expect(TokenType::LPAREN, "Expected '(' after table name", tokens, pos);

    while(peek(tokens, pos).type != TokenType::RPAREN)
    {
        ColumnDef column;
        column.name = expect(TokenType::IDENTIFIER, 
            "Expected column name", tokens, pos).value;

        if(match(TokenType::INT, tokens, pos))
            column.type = IntType{};
        else if(match(TokenType::BOOL, tokens, pos))
            column.type = BoolType{};
        else if(match(TokenType::FLOAT_TYPE, tokens, pos))
            column.type = FloatType{};
        else if(match(TokenType::VARCHAR, tokens, pos))
        {
            expect(TokenType::LPAREN, "Expected '(' after VARCHAR", tokens, pos);
            int length = std::stoi(expect(TokenType::INTEGER, 
                "Expected length for VARCHAR", tokens, pos).value);
                
            expect(TokenType::RPAREN, "Expected ')' after VARCHAR length", tokens, pos);
            column.type = VarCharType{length};
        }
        else if(match(TokenType::TIMESTAMP, tokens, pos))
            column.type = TimestampType{};
        else
            throw std::runtime_error("Expected data type for column " + column.name);

        while(peek(tokens, pos).type != TokenType::COMMA && peek(tokens, pos).type != TokenType::RPAREN)
        {
            if(match(TokenType::PRIMARY, tokens, pos))
            {
                expect(TokenType::KEY, "Expected KEY after PRIMARY", tokens, pos);
                column.primaryKey = true;
            }
            else if(match(TokenType::NOT, tokens, pos))
            {
                expect(TokenType::NULL_LIT, "Expected NULL after NOT", tokens, pos);
                column.nullable = false;
            }
            else if(match(TokenType::UNIQUE, tokens, pos))
            {
                column.unique = true;
            }
            else
            {
                throw std::runtime_error("Unexpected token in column definition: " 
                    + peek(tokens, pos).value);
            }
        }

        createStmt.columns.push_back(std::move(column));

        if(peek(tokens, pos).type == TokenType::COMMA)
            advance(tokens, pos);
    }

    expect(TokenType::RPAREN, "Expected ')' after column definitions", tokens, pos);
    expect(TokenType::SEMICOLON, "Expected ';' after CREATE TABLE statement", tokens, pos);
    return;
}

void Parser::parseCreateIndexStatement(CreateIndexStatement& createStmt, 
    const std::vector<Token>& tokens, std::size_t& pos) const
{
    createStmt.indexName = expect(TokenType::IDENTIFIER, 
        "Expected index name after CREATE INDEX", tokens, pos).value;
    expect(TokenType::ON, "Expected ON after index name", tokens, pos);

    createStmt.tableName = expect(TokenType::IDENTIFIER, 
        "Expected table name after ON", tokens, pos).value;
    expect(TokenType::LPAREN, "Expected '(' after table name", tokens, pos);

    createStmt.columnName = expect(TokenType::IDENTIFIER, 
        "Expected column name after '('", tokens, pos).value;
    expect(TokenType::RPAREN, "Expected ')' after column name", tokens, pos);
    expect(TokenType::SEMICOLON, "Expected ';' after CREATE INDEX statement", tokens, pos);

    return;
}

void Parser::parseDropTableStatement(DropTableStatement& dropStmt, 
    const std::vector<Token>& tokens, std::size_t& pos) const
{
    dropStmt.tableName = expect(TokenType::IDENTIFIER, 
        "Expected table name after DROP TABLE", tokens, pos).value;

    expect(TokenType::SEMICOLON, "Expected ';' after DROP TABLE statement", tokens, pos);
    return;
}

void Parser::parseDropIndexStatement(DropIndexStatement& dropStmt, 
    const std::vector<Token>& tokens, std::size_t& pos) const
{
    dropStmt.indexName = expect(TokenType::IDENTIFIER, 
        "Expected index name after DROP INDEX", tokens, pos).value;

    expect(TokenType::SEMICOLON, "Expected ';' after DROP INDEX statement", tokens, pos);
    return;
}

void Parser::parseBeginStatement(BeginStatement& beginStmt, 
    const std::vector<Token>& tokens, std::size_t& pos) const
{
    expect(TokenType::SEMICOLON, "Expected ';' after BEGIN statement", tokens, pos);
    return;
}

void Parser::parseCommitStatement(CommitStatement& commitStmt, 
    const std::vector<Token>& tokens, std::size_t& pos) const
{
    expect(TokenType::SEMICOLON, "Expected ';' after COMMIT statement", tokens, pos);
    return;
}

void Parser::parseRollbackStatement(RollbackStatement& rollbackStmt, 
    const std::vector<Token>& tokens, std::size_t& pos) const
{
    expect(TokenType::SEMICOLON, "Expected ';' after ROLLBACK statement", tokens, pos);
    return;
}

Statement Parser::parse(const std::vector<Token>& tokens, 
    std::size_t& pos) const
{
    if(pos >= tokens.size())
        throw std::runtime_error("No more tokens to parse");

    Statement parseStatement;

    switch(peek(tokens, pos).type)
    {
        case TokenType::SELECT:
            advance(tokens, pos);
            parseSelectStatement(std::get<SelectStatement>(parseStatement), tokens, pos);
            break;

        case TokenType::INSERT:
            advance(tokens, pos);
            parseInsertStatement(std::get<InsertStatement>(parseStatement), tokens, pos);
            break;

        case TokenType::UPDATE:
            advance(tokens, pos);
            parseUpdateStatement(std::get<UpdateStatement>(parseStatement), tokens, pos);
            break;

        case TokenType::DELETE:
            advance(tokens, pos);
            parseDeleteStatement(std::get<DeleteStatement>(parseStatement), tokens, pos);
            break;

        case TokenType::CREATE:
            advance(tokens, pos);
            if(check(TokenType::TABLE, tokens, pos))
            {
                advance(tokens, pos);
                parseCreateTableStatement(std::get<CreateTableStatement>(parseStatement), tokens, pos);
            }
            else if(check(TokenType::INDEX, tokens, pos))
            {
                advance(tokens, pos);
                parseCreateIndexStatement(std::get<CreateIndexStatement>(parseStatement), tokens, pos);
            }
            else
                throw std::runtime_error("Expected TABLE or INDEX after CREATE");
            break;

        case TokenType::DROP:
            advance(tokens, pos);
            if(check(TokenType::TABLE, tokens, pos))
            {
                advance(tokens, pos);
                parseDropTableStatement(std::get<DropTableStatement>(parseStatement), tokens, pos);
            }
            else if(check(TokenType::INDEX, tokens, pos))
            {
                advance(tokens, pos);
                parseDropIndexStatement(std::get<DropIndexStatement>(parseStatement), tokens, pos);
            }
            else
                throw std::runtime_error("Expected TABLE or INDEX after DROP");
            break;

        case TokenType::BEGIN:
            advance(tokens, pos);
            parseBeginStatement(std::get<BeginStatement>(parseStatement), tokens, pos);
            break;

        case TokenType::COMMIT:
            advance(tokens, pos);
            parseCommitStatement(std::get<CommitStatement>(parseStatement), tokens, pos);
            break;

        case TokenType::ROLLBACK:
            advance(tokens, pos);
            parseRollbackStatement(std::get<RollbackStatement>(parseStatement), tokens, pos);
            break;

        default:
            throw std::runtime_error("Unexpected token: " + peek(tokens, pos).value);
    }

    return parseStatement;
}

Statement Parser::parse(const std::vector<Token>& tokens) const
{
    std::size_t pos = 0;
    return parse(tokens, pos);
}

std::vector<Statement> Parser::parseAll(const std::vector<Token>& tokens) const
{
    std::size_t pos = 0;
    std::vector<Statement> statements;

    while (peek(tokens, pos).type != TokenType::END_OF_FILE)
    {
        statements.push_back(parse(tokens, pos));
    }

    return statements;
}