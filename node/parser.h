#pragma once

#include "ast.h"
#include "token.h"
#include <iostream>

class Parser
{
    // DML statements
    void parseSelectStatement(SelectStatement& selectStmt, 
        std::vector<Token>& tokens, std::size_t& pos);
    void parseInsertStatement(InsertStatement& insertStmt, 
        std::vector<Token>& tokens, std::size_t& pos);
    void parseUpdateStatement(UpdateStatement& updateStmt, 
        std::vector<Token>& tokens, std::size_t& pos);
    void parseDeleteStatement(DeleteStatement& deleteStmt, 
        std::vector<Token>& tokens, std::size_t& pos);

    // DDL statements
    void parseCreateTableStatement(CreateTableStatement& createStmt, 
        std::vector<Token>& tokens, std::size_t& pos);
    void parseCreateIndexStatement(CreateIndexStatement& createStmt, 
        std::vector<Token>& tokens, std::size_t& pos);
    void parseDropTableStatement(DropTableStatement& dropStmt, 
        std::vector<Token>& tokens, std::size_t& pos);
    void parseDropIndexStatement(DropIndexStatement& dropStmt, 
        std::vector<Token>& tokens, std::size_t& pos);

    // Transaction statements
    void parseBeginStatement(BeginStatement& beginStmt, std::vector<Token>& tokens, std::size_t& pos);
    void parseCommitStatement(CommitStatement& commitStmt, std::vector<Token>& tokens, std::size_t& pos);
    void parseRollbackStatement(RollbackStatement& rollbackStmt, std::vector<Token>& tokens, std::size_t& pos);

    //Expression parsing
    Expr parsePrimaryExpression(std::vector<Token>& tokens, std::size_t& pos);
    Expr parseUnaryExpression(std::vector<Token>& tokens, std::size_t& pos);
    Expr parseMultiplicativeExpression(std::vector<Token>& tokens, std::size_t& pos);
    Expr parseAdditiveExpression(std::vector<Token>& tokens, std::size_t& pos);
    Expr parseComparisonExpression(std::vector<Token>& tokens, std::size_t& pos);
    Expr parseLogicalAndExpression(std::vector<Token>& tokens, std::size_t& pos);
    Expr parseLogicalOrExpression(std::vector<Token>& tokens, std::size_t& pos);
    Expr parseExpression(std::vector<Token>& tokens, std::size_t& pos);
    Expr parseAggregateFunctionCall(std::vector<Token>& tokens, std::size_t& pos);

    // Helper functions
    Token& peek(std::vector<Token>& tokens, std::size_t& pos);
    Token& advance(std::vector<Token>& tokens, std::size_t& pos);
    bool check(TokenType type, std::vector<Token>& tokens, std::size_t& pos);
    bool match (TokenType type, std::vector<Token>& tokens, std::size_t& pos);
    Token& expect(TokenType type, const std::string& errorMessage, std::vector<Token>& tokens, std::size_t& pos);

public:
    Parser() {}

    std::vector<Statement> parseAll(std::vector<Token>& tokens);
    Statement parse(std::vector<Token>& tokens, std::size_t& pos);
    Statement parse(std::vector<Token>& tokens);
};