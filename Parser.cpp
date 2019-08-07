#include "Parser.h"

#include <iostream>
#include <sstream>

CParseError CParseError::UnexpectedToken(EToken token)
{
    std::stringstream ss;
    ss << "Unexpected token " << (uint32_t) token;
    return {ss.str()};
}

CParseError CParseError::TokenMismatch(EToken expected)
{
    std::stringstream ss;
    ss << "Expected token " << (uint32_t) expected << " not found";
    return {ss.str()};
}

CParser::CParser(CLexer &lexer, IParserHandler *handler)
    : Lexer(lexer)
    , Handler(handler)
{
}

void CParser::Parse()
{
    try {
        while (Lexer.Peek().first != EToken::EOF)
            ParseStmt();
    } catch (const CParseError &e) {
        // TODO: hook into Qt's debug print framework
        std::cout << e.GetMessage().c_str() << std::endl;
    }
}

void CParser::ParseStmt()
{
    auto next = Lexer.Peek();
    if (next.first == EToken::REQUIRE)
        ParseRequire();
    else if (next.first == EToken::IDENT) {
        next = Lexer.Peek2();
        if (next.first == EToken::COLONCOLON)
            ParseElement();
        else
            ParseConn();
    } else
        throw CParseError::UnexpectedToken(next.first);
}

void CParser::ParseRequire()
{
    CTokenVal val;
    MustMatch(EToken::REQUIRE, val);
    MustMatch(EToken::PARENCONFIG, val);
    MustMatch(static_cast<EToken>(';'), val);
}

void CParser::ParseElement()
{
    std::string name;
    std::string className;
    std::string configStr;

    CTokenVal val;
    MustMatch(EToken::IDENT, val);
    name = std::get<std::string>(val);
    MustMatch(EToken::COLONCOLON, val);
    MustMatch(EToken::IDENT, val);
    className = std::get<std::string>(val);

    // Optional config string
    auto next = Lexer.Advance();
    if (next.first == EToken::PARENCONFIG) {
        configStr = std::get<std::string>(next.second);
        MustMatch(static_cast<EToken>(';'), val);
    } else if (next.first == static_cast<EToken>(';'))
        ;
    else
        throw CParseError::UnexpectedToken(next.first);

    if (Handler)
        Handler->OnElementDecl(name, className, configStr);
}

void CParser::ParsePort(uint32_t &outPort)
{
    CTokenVal val;
    if (Lexer.Match(static_cast<EToken>('['), val)) {
        MustMatch(EToken::INTEGER, val);
        outPort = (uint32_t) std::get<int64_t>(val);
        MustMatch(static_cast<EToken>(']'), val);
    } else {
        outPort = 0;
    }
}

void CParser::ParseDest(const std::string &srcName, std::string &outDstName)
{
    uint32_t srcPort, dstPort;

    CTokenVal val;
    ParsePort(srcPort);
    MustMatch(EToken::RIGHTARROW, val);
    ParsePort(dstPort);
    MustMatch(EToken::IDENT, val);
    outDstName = std::get<std::string>(val);

    if (Handler)
        Handler->OnConnection(srcName, outDstName, srcPort, dstPort);
}

void CParser::ParseConn()
{
    CTokenVal val;
    MustMatch(EToken::IDENT, val);
    std::string srcName = std::get<std::string>(val);
    std::string dstName;

    auto next = Lexer.Peek();
    while (next.first == static_cast<EToken>('[') || next.first == EToken::RIGHTARROW) {
        ParseDest(srcName, dstName);
        srcName = dstName;
        next = Lexer.Peek();
    }
    MustMatch(static_cast<EToken>(';'), val);
}

void CParser::MustMatch(EToken expected, CTokenVal &outVal)
{
    bool match = Lexer.Match(expected, outVal);
    if (!match)
        throw CParseError::TokenMismatch(expected);
}
