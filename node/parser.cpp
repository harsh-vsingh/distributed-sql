#include "parser.h"
#include <stdexcept>
#include <utility>

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

void Parser::parseSelectStatement(SelectStatement& selectStmt)
{
    // Implementation for parsing SELECT statement
}

void Parser::parseInsertStatement(InsertStatement& insertStmt)
{
    // Implementation for parsing INSERT statement
}

void Parser::parseUpdateStatement(UpdateStatement& updateStmt)
{
    // Implementation for parsing UPDATE statement
}

void Parser::parseDeleteStatement(DeleteStatement& deleteStmt)
{
    // Implementation for parsing DELETE statement
}

void Parser::parseCreateTableStatement(CreateTableStatement& createStmt)
{
    // Implementation for parsing CREATE TABLE statement
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