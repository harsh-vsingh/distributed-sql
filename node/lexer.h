#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <algorithm>
#include "token.h"

class Lexer
{
    int pos = 0;
    int line = 1;
    int col = 1;
    int lastResolvedTokenPos = -1; // Last token position that was resolved
    int tokenStartLine = 0;        // Line number where the current token starts
    int tokenStartCol = 0;         // Column number where the current token starts

    void advance(const std::string& source);
    bool isSpecialChar(const std::string& source);
    bool isSymbol(const std::string& source);
    bool isAlpha(const std::string& source);
    bool isDigit(const std::string& source);
    bool isComment(const std::string& source);

    void resolveSymbol(const std::string& source, std::vector<Token>& tokens);
    void resolveUnresolved(const std::string& source, std::vector<Token>& tokens);
    void resolveSpecialChar(const std::string& source);
    void resolveString(const std::string& source, std::vector<Token>& tokens);
    void resolveNumber(const std::string& source, std::vector<Token>& tokens);
    void skipComment(const std::string& source);


public:
    Lexer() {}

    std::vector<Token> tokenize(const std::string& source);
};