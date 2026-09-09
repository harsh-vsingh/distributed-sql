#include "lexer.h"

std::unordered_map<std::string, TokenType> keywords = {
    {"CREATE", TokenType::CREATE},
    {"TABLE", TokenType::TABLE},
    {"INDEX", TokenType::INDEX},
    {"ON", TokenType::ON},
    {"DROP", TokenType::DROP},
    {"SELECT", TokenType::SELECT},
    {"INSERT", TokenType::INSERT},
    {"INTO", TokenType::INTO},
    {"VALUES", TokenType::VALUES},
    {"UPDATE", TokenType::UPDATE},
    {"SET", TokenType::SET},
    {"DELETE", TokenType::DELETE},
    {"FROM", TokenType::FROM},
    {"WHERE", TokenType::WHERE},
    {"AND", TokenType::AND},
    {"OR", TokenType::OR},
    {"NOT", TokenType::NOT},
    {"AS", TokenType::AS},
    {"DISTINCT", TokenType::DISTINCT},
    {"JOIN", TokenType::JOIN},
    {"INNER", TokenType::INNER},
    {"LEFT", TokenType::LEFT},
    {"RIGHT", TokenType::RIGHT},
    {"GROUP", TokenType::GROUP},
    {"BY", TokenType::BY},
    {"ORDER", TokenType::ORDER},
    {"HAVING", TokenType::HAVING},
    {"LIMIT", TokenType::LIMIT},
    {"ASC", TokenType::ASC},
    {"DESC", TokenType::DESC},
    {"COUNT", TokenType::COUNT},
    {"SUM", TokenType::SUM},
    {"AVG", TokenType::AVG},
    {"MIN", TokenType::MIN},
    {"MAX", TokenType::MAX},
    {"BEGIN", TokenType::BEGIN},
    {"COMMIT", TokenType::COMMIT},
    {"ROLLBACK", TokenType::ROLLBACK},
    {"INT", TokenType::INT},
    {"BOOL", TokenType::BOOL},
    {"FLOAT", TokenType::FLOAT_TYPE},
    {"VARCHAR", TokenType::VARCHAR},
    {"TIMESTAMP", TokenType::TIMESTAMP},
    {"PRIMARY", TokenType::PRIMARY},
    {"KEY", TokenType::KEY},
    {"FOREIGN", TokenType::FOREIGN},
    {"REFERENCES", TokenType::REFERENCES},
    {"UNIQUE", TokenType::UNIQUE},
    {"FALSE", TokenType::FALSE_LIT},
    {"TRUE", TokenType::TRUE_LIT},
    {"NULL", TokenType::NULL_LIT}
};

std::unordered_map<std::string, TokenType> symbols = {
    {"=", TokenType::EQ},
    {"!=", TokenType::NEQ},
    {"<", TokenType::LT},
    {">", TokenType::GT},
    {"<>", TokenType::NEQ},
    {"<=", TokenType::LTE},
    {">=", TokenType::GTE},
    {"+", TokenType::PLUS},
    {"-", TokenType::MINUS},
    {"*", TokenType::STAR},
    {"/", TokenType::SLASH},
    {"(", TokenType::LPAREN},
    {")", TokenType::RPAREN},
    {",", TokenType::COMMA},
    {";", TokenType::SEMICOLON},
    {".", TokenType::DOT}
};


void Lexer::advance(const std::string& source)
{
    if(pos < source.length())
    {
        if(source[pos] == '\n')
        {
            line++;
            col = 1;
        }
        else
        {
            col++;
        }
        pos++;
    }
}

bool Lexer::isSpecialChar(const std::string& source)
{
    return source[pos] == ' ' || source[pos] == '\t' || source[pos] == '\n' || source[pos] == '\r';
}

bool Lexer::isSymbol(const std::string& source)
{
    char c = source[pos];
    return c == '=' || c == '<' || c == '>' || c == '!' ||
           c == '+' || c == '-' || c == '*' || c == '/' ||
           c == '(' || c == ')' || c == ',' || c == ';' || c == '.';
}

bool Lexer::isAlpha(const std::string& source)
{
    return std::isalpha(static_cast<unsigned char>(source[pos])) || source[pos] == '_';
}

bool Lexer::isDigit(const std::string& source)
{
    return std::isdigit(static_cast<unsigned char>(source[pos]));
}

void Lexer::resolveSymbol(const std::string& source, std::vector<Token>& tokens)
{
    int startLine = line;
    int startCol = col;
    char c = source[pos];
    if((c == '=' || c == '<' || c == '>' || c == '!') && pos + 1 < source.length() && source[pos + 1] == '=')
    {
        std::string symbol = std::string(1, c) + "=";
        tokens.push_back({symbols.at(symbol), symbol, startLine, startCol});
        advance(source);
        advance(source);
    }
    else if(c == '<' && pos + 1 < source.length() && source[pos + 1] == '>')
    {
        std::string symbol = "<>";
        tokens.push_back({symbols.at(symbol), symbol, startLine, startCol});
        advance(source);
        advance(source);
    }
    else
    {
        std::string symbol = std::string(1, c);
        tokens.push_back({symbols.at(symbol), symbol, startLine, startCol});
        advance(source);
    }
    lastResolvedTokenPos = pos - 1;
}

void Lexer::resolveUnresolved(const std::string& source, std::vector<Token>& tokens)
{
    std::string unresolved = source.substr(lastResolvedTokenPos + 1, pos - lastResolvedTokenPos - 1);

    if(unresolved.empty())
    {
        return;
    }

    std::string upper = unresolved;
    std::transform(upper.begin(), upper.end(), upper.begin(),
        [](unsigned char c) {
            return static_cast<char>(std::toupper(c));
        });

    if(keywords.find(upper) != keywords.end())
    {
        tokens.push_back({keywords[upper], unresolved, tokenStartLine, tokenStartCol});
    }
    else
    {
        tokens.push_back({TokenType::IDENTIFIER, unresolved, tokenStartLine, tokenStartCol});
    }
    lastResolvedTokenPos = pos - 1;
}

void Lexer::resolveSpecialChar(const std::string& source)
{
    advance(source);
    lastResolvedTokenPos = pos - 1;
}

void Lexer::resolveString(const std::string& source, std::vector<Token>& tokens)
{
    int startLine = line;
    int startCol = col;
    char quoteType = source[pos];
    std::string strValue;
    advance(source);

    while(pos < source.length())
    {
        if(source[pos] == '\\' && pos + 1 < source.length())
        {
            advance(source);
            strValue += source[pos];
            advance(source);
            continue;
        }
        if(source[pos] == quoteType) break;
        strValue += source[pos];
        advance(source);
    }

    if(pos >= source.length())
    {
        throw std::runtime_error("Unterminated string at line " + std::to_string(line) + ", col " + std::to_string(col) + "\n"
            "String started at line " + std::to_string(startLine) + ", col " + std::to_string(startCol));
    }

    advance(source);
    tokens.push_back({TokenType::STRING, strValue, startLine, startCol});
    lastResolvedTokenPos = pos - 1;
}

void Lexer::resolveNumber(const std::string& source, std::vector<Token>& tokens)
{
    int startLine = line;
    int startCol = col;
    bool isFloat = false;
    std::string numValue;

    while(pos < source.length() && (isDigit(source) || source[pos] == '.'))
    {
        if(source[pos] == '.')
        {
            if(isFloat)
            {
                throw std::runtime_error("Invalid number format at line " + std::to_string(line) + ", column " + std::to_string(col) + "\n"
                    + "Number started at line " + std::to_string(startLine) + ", column " + std::to_string(startCol));
                break;
            }
            if(pos + 1 >= source.length() || !std::isdigit(source[pos + 1]))
            {
                break;
            }
            isFloat = true;
        
        }
        
        numValue += source[pos];
        advance(source);
    }

    if(pos < source.length() && isAlpha(source))
    {
        throw std::runtime_error(
            "Invalid number at line " + std::to_string(startLine) +
            ", column " + std::to_string(startCol)
        );
    }

    if(!numValue.empty())
    {
        tokens.push_back({isFloat ? TokenType::FLOAT : TokenType::INTEGER, numValue, startLine, startCol});
    }
    lastResolvedTokenPos = pos - 1;
}

void Lexer::skipComment(const std::string& source)
{
    if(pos + 1 < source.length() && source[pos] == '-' && source[pos+1] == '-')
    {
        while(pos < source.length() && source[pos] != '\n')
            advance(source);
    }
    else if(pos + 1 < source.length() && source[pos] == '/' && source[pos+1] == '*')
    {
        advance(source);
        advance(source);
        while(pos + 1 < source.length() &&
            !(source[pos] == '*' && source[pos + 1] == '/'))
        {
            advance(source);
        }

        if(pos + 1 >= source.length())
            throw std::runtime_error("Unterminated comment at line " +
                    std::to_string(line) + ", column " + std::to_string(col));

        advance(source);
        advance(source);
    }
}

std::vector<Token> Lexer::tokenize(const std::string& source)
{
    std::vector<Token> tokens;
    pos = 0;
    line = 1;
    col = 1;
    lastResolvedTokenPos = -1;
    tokenStartLine = 0;
    tokenStartCol = 0;

    if(source.empty())
    {
        tokens.push_back({TokenType::END_OF_FILE, "", line, col});
        return tokens;
    }

    while(pos < source.length())
    {
        if(isSpecialChar(source))
        {
            resolveUnresolved(source, tokens);
            resolveSpecialChar(source);
        }
        else if(source[pos] == '-' && pos + 1 < source.length() && source[pos+1] == '-')
        {
            resolveUnresolved(source, tokens);
            skipComment(source);
        }
        else if(source[pos] == '/' && pos + 1 < source.length() && source[pos+1] == '*')
        {
            resolveUnresolved(source, tokens);
            skipComment(source);
        }
        else if(isSymbol(source))
        {
            resolveUnresolved(source, tokens);
            resolveSymbol(source, tokens);
        }
        else if(isDigit(source))
        {
            int unresolvedLen = pos - lastResolvedTokenPos - 1;
            if(unresolvedLen > 0) // The digit is part of an unresolved token, like an identifier or keyword
                advance(source);
            else
                resolveNumber(source, tokens);
        }
        else if(source[pos] == '"' || source[pos] == '\'')
        {
            resolveUnresolved(source, tokens);
            resolveString(source, tokens);
        }
        else if(isAlpha(source))
        {
            if(pos - lastResolvedTokenPos - 1 == 0)
            {
                tokenStartLine = line;
                tokenStartCol = col;
            }
            advance(source);
        }
        else
        {
            throw std::runtime_error("Unexpected character: " + std::string(1, source[pos]) 
                      + " at line " + std::to_string(line) + ", col " + std::to_string(col));
            advance(source);
        }
    }

    resolveUnresolved(source, tokens);
    tokens.push_back({TokenType::END_OF_FILE, "", line, col});
    return tokens;
}