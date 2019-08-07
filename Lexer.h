#pragma once
#include <string>
#include <utility>
#include <variant>

/*
 * CLexer
 * Simple lexer than scans the Click flatconfig file
 */

// 0-127 is reserved for plain character tokens, EOF is explicitly defined
#undef EOF
enum class EToken : uint8_t {
    REQUIRE = 128,
    PARENCONFIG,
    IDENT,
    COLONCOLON,
    INTEGER,
    RIGHTARROW,
    EOF = 255
};

typedef std::variant<int64_t, std::string> CTokenVal;

class CLexer
{
public:
    CLexer(std::string source);

    // Eats a token and return it
    std::pair<EToken, CTokenVal> Advance();
    // Advance only if the next token matches the specified token, otherwise returns false
    // TODO: also give the actual token encountered
    bool Match(EToken token, CTokenVal &outVal);
    // Return next token without advancing
    std::pair<EToken, CTokenVal> Peek();
    std::pair<EToken, CTokenVal> Peek2();

private:
    // Operations on the charater stream
    char AdvanceChar();
    bool MatchChar(char ch);
    bool MatchKeyword(const std::string &keyword);
    char PeekChar();

    static bool IsPartOfIdent(char ch);
    static bool IsWhitespace(char ch);

    std::string Source;
    size_t Cursor = 0;
};
