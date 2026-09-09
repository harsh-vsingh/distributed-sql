#pragma once

#include "ast.h"
#include "token.h"
#include <iostream>

class Parser
{
    // DML statements
    void parseSelectStatement(SelectStatement& selectStmt, 
        const std::vector<Token>& tokens, std::size_t& pos) const;
    void parseInsertStatement(InsertStatement& insertStmt, 
        const std::vector<Token>& tokens, std::size_t& pos) const;
    void parseUpdateStatement(UpdateStatement& updateStmt, 
        const std::vector<Token>& tokens, std::size_t& pos) const;
    void parseDeleteStatement(DeleteStatement& deleteStmt, 
        const std::vector<Token>& tokens, std::size_t& pos) const;

    // DDL statements
    void parseCreateTableStatement(CreateTableStatement& createStmt, 
        const std::vector<Token>& tokens, std::size_t& pos) const;
    void parseCreateIndexStatement(CreateIndexStatement& createStmt, 
        const std::vector<Token>& tokens, std::size_t& pos) const;
    void parseDropTableStatement(DropTableStatement& dropStmt, 
        const std::vector<Token>& tokens, std::size_t& pos) const;
    void parseDropIndexStatement(DropIndexStatement& dropStmt, 
        const std::vector<Token>& tokens, std::size_t& pos) const;

    // Transaction statements
    void parseBeginStatement(BeginStatement& beginStmt, 
        const std::vector<Token>& tokens, std::size_t& pos) const;
    void parseCommitStatement(CommitStatement& commitStmt, 
        const std::vector<Token>& tokens, std::size_t& pos) const;
    void parseRollbackStatement(RollbackStatement& rollbackStmt, 
        const std::vector<Token>& tokens, std::size_t& pos) const;

    //Expression parsing
    Expr parsePrimaryExpression(const std::vector<Token>& tokens, 
        std::size_t& pos) const;
    Expr parseUnaryExpression(const std::vector<Token>& tokens, 
        std::size_t& pos) const;
    Expr parseMultiplicativeExpression(const std::vector<Token>& tokens, 
        std::size_t& pos) const;
    Expr parseAdditiveExpression(const std::vector<Token>& tokens, 
        std::size_t& pos) const;
    Expr parseComparisonExpression(const std::vector<Token>& tokens, 
        std::size_t& pos) const;
    Expr parseLogicalAndExpression(const std::vector<Token>& tokens, 
        std::size_t& pos) const;
    Expr parseLogicalOrExpression(const std::vector<Token>& tokens, 
        std::size_t& pos) const;
    Expr parseExpression(const std::vector<Token>& tokens, 
        std::size_t& pos) const;
    Expr parseAggregateFunctionCall(const std::vector<Token>& tokens, 
        std::size_t& pos) const;

    // Helper functions
    const Token& peek(const std::vector<Token>& tokens, std::size_t& pos) const;
    const Token& advance(const std::vector<Token>& tokens, std::size_t& pos) const;
    bool check(TokenType type, const std::vector<Token>& tokens, std::size_t& pos) const;
    bool match(TokenType type, const std::vector<Token>& tokens, std::size_t& pos) const;
    const Token& expect(TokenType type, const std::string& errorMessage, 
        const std::vector<Token>& tokens, std::size_t& pos) const;

public:
    Parser() {}

    Statement parse(const std::vector<Token>& tokens) const;
    Statement parse(const std::vector<Token>& tokens, std::size_t& pos) const;
    std::vector<Statement> parseAll(const std::vector<Token>& tokens) const;
};