#pragma once

#include "ast.h"
#include "token.h"
#include <iostream>

class Parser
{
    std::vector<Token> tokens;
    std::size_t pos = 0;

    // DML statements
    void parseSelectStatement(SelectStatement& selectStmt);
    void parseInsertStatement(InsertStatement& insertStmt);
    void parseUpdateStatement(UpdateStatement& updateStmt);
    void parseDeleteStatement(DeleteStatement& deleteStmt);

    // DDL statements
    void parseCreateTableStatement(CreateTableStatement& createStmt);
    void parseCreateIndexStatement(CreateIndexStatement& createStmt);
    void parseDropTableStatement(DropTableStatement& dropStmt);
    void parseDropIndexStatement(DropIndexStatement& dropStmt);

    // Transaction statements
    void parseBeginStatement(BeginStatement& beginStmt);
    void parseCommitStatement(CommitStatement& commitStmt);
    void parseRollbackStatement(RollbackStatement& rollbackStmt);

    //Expression parsing
    Expr parsePrimaryExpression();
    Expr parseUnaryExpression();
    Expr parseMultiplicativeExpression();
    Expr parseAdditiveExpression();
    Expr parseComparisonExpression();
    Expr parseLogicalAndExpression();
    Expr parseLogicalOrExpression();
    Expr parseExpression();
    Expr parseAggregateFunctionCall();

    // Helper functions
    Token& peek();
    Token& advance();
    bool check(TokenType type);
    bool match (TokenType type);
    Token& expect(TokenType type, const std::string& errorMessage);

public:
    Parser(std::vector<Token>& tokens) : tokens(tokens), pos(0) {}

    void setTokens(std::vector<Token>& newTokens);
    std::vector<Statement> parseAll();
    Statement parse();
};