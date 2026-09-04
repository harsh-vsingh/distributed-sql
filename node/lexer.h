#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <algorithm>
#include "token.h"

class Lexer
{
    std::string source;
    int pos = 0;
    int line = 1;
    int col = 1;
    int lastResolvedTokenPos = -1; // Last token position that was resolved
    int tokenStartLine = 0;        // Line number where the current token starts
    int tokenStartCol = 0;         // Column number where the current token starts
    std::vector<Token> tokens;

    void advance();

    bool isSpecialChar();
    bool isSymbol();
    bool isAlpha();
    bool isDigit();
    bool isComment();

    void resolveSymbol();
    void resolveUnresolved();
    void resolveSpecialChar();
    void resolveString();
    void resolveNumber();
    void skipComment();

public:
    Lexer(const std::string& source) : source(source) {}

    void setSource(const std::string& newSource);
    std::vector<Token> tokenize();
};