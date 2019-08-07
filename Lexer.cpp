#include "Lexer.h"

CLexer::CLexer(std::string source)
    : Source(std::move(source))
{}

std::pair<EToken, CTokenVal> CLexer::Advance()
{
    if (Cursor == Source.size())
        return {EToken::EOF, {}};

    std::string strVal;
    char ch = AdvanceChar();
    unsigned int openParens; // Counter to deal with nested parentheses

    while (IsWhitespace(ch))
        ch = AdvanceChar();

    switch (ch) {
    case '(':
        // Eat until ')' or EOF inclusive
        openParens = 1;
        strVal.push_back(ch);
        do {
            ch = AdvanceChar();
            if (ch == '(')
                openParens++;
            if (ch == ')')
                openParens--;
            if (ch == -1)
                break;
            strVal.push_back(ch);
        } while (openParens > 0);
        if (ch == -1)
            return {EToken::EOF, strVal};
        else
            return {EToken::PARENCONFIG, strVal};
    case 'r':
        // Could be REQUIRE or IDENT
        if (MatchKeyword("equire"))
            return {EToken::REQUIRE, {}};
        break;
    case ':':
        if (MatchChar(':'))
            return {EToken::COLONCOLON, {}};
        else
            return {static_cast<EToken>(ch), {}};
    case '-':
        if (MatchChar('>'))
            return {EToken::RIGHTARROW, {}};
        else
            return {static_cast<EToken>(ch), {}};
    default:
        break;
    }

    // Handle INTEGER or IDENT or other stuff starts with ch
    if (isnumber(ch)) {
        // Since identifiers can start with numbers, we need to make sure this is not an identifier
        bool isIdent = false;
        size_t offset = 0;
        for (; offset < Source.size(); offset++) {
            char lookAhead = Source[Cursor + offset];
            if (isnumber(lookAhead))
                continue;
            if (IsPartOfIdent(lookAhead)) {
                isIdent = true;
                break;
            }
            break;
        }

        if (!isIdent) {
            int64_t numVal = ch - '0';
            while (isnumber(ch = PeekChar())) {
                numVal *= 10;
                numVal += ch - '0';
                AdvanceChar();
            }
            return {EToken::INTEGER, numVal};
        }
    }
    if (IsPartOfIdent(ch)) {
        strVal.push_back(ch);
        while (IsPartOfIdent(ch = PeekChar())) {
            strVal.push_back(ch);
            AdvanceChar();
        }
        return {EToken::IDENT, strVal};
    }
    return {static_cast<EToken>(ch), {}};
}

bool CLexer::Match(EToken token, CTokenVal &outVal)
{
    auto savedCursor = Cursor;
    auto eaten = Advance();
    if (eaten.first == token) {
        outVal = eaten.second;
        return true;
    }
    Cursor = savedCursor;
    return false;
}

std::pair<EToken, CTokenVal> CLexer::Peek()
{
    auto savedCursor = Cursor;
    auto eaten = Advance();
    Cursor = savedCursor;
    return eaten;
}

std::pair<EToken, CTokenVal> CLexer::Peek2()
{
    auto savedCursor = Cursor;
    auto eaten = Advance();
    eaten = Advance();
    Cursor = savedCursor;
    return eaten;
}

char CLexer::AdvanceChar()
{
    if (Cursor == Source.size())
        return -1;
    return Source[Cursor++];
}

bool CLexer::MatchChar(char ch)
{
    if (Cursor == Source.size())
        return false;
    if (Source[Cursor] == ch) {
        Cursor++;
        return true;
    } else {
        return false;
    }
}

bool CLexer::MatchKeyword(const std::string &keyword)
{
    size_t ki = 0;
    for (; ki < keyword.size(); ki++) {
        auto i = Cursor + ki;
        if (i >= Source.size() || Source[i] != keyword[ki])
            return false;
    }
    Cursor += ki;
    return true;
}

char CLexer::PeekChar()
{
    if (Cursor == Source.size())
        return -1;
    return Source[Cursor];
}

bool CLexer::IsPartOfIdent(char ch)
{
    return isalnum(ch) || ch == '@' || ch == '_' || ch == '/';
}

bool CLexer::IsWhitespace(char ch)
{
    return isspace(ch);
}
