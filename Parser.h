#pragma once
#include "Lexer.h"

/*
 * CParser
 * Parses and transforms a flatconfig into some graphical representation
 * 
 * The grammar is as follows:
 */

/*
    require := REQUIRE PARENCONFIG ';'
    
    element := IDENT COLONCOLON IDENT PARENCONFIG? ';'
                    
    port := empty | '[' INTEGER ']'
    dest := port RIGHTARROW port IDENT
    conn := IDENT dest* ';'
                                                                         
    stmt := require | element | conn;
    program := stmt*
*/

class CParseError
{
public:
    CParseError(std ::string message)
        : Message(std::move(message))
    {}

    const std::string &GetMessage() const { return Message; }

    static CParseError UnexpectedToken(EToken token);
    static CParseError TokenMismatch(EToken expected);

private:
    std::string Message;
};

class IParserHandler
{
public:
    virtual ~IParserHandler() = default;

    virtual void OnElementDecl(const std::string &name,
                               const std::string &className,
                               const std::string &configStr)
        = 0;

    virtual void OnConnection(const std::string &source,
                              const std::string &sink,
                              uint32_t srcPort,
                              uint32_t sinkPort)
        = 0;
};

class CParser
{
public:
    CParser(CLexer &lexer, IParserHandler *handler);

    void Parse();

private:
    void ParseStmt();

    void ParseRequire();

    void ParseElement();

    void ParsePort(uint32_t &outPort);
    void ParseDest(const std::string &srcName, std::string &outDstName);
    void ParseConn();

    void MustMatch(EToken expected, CTokenVal &outVal);

    CLexer &Lexer; // It's pretty dangerous to keep a reference, but this is a small project so
    IParserHandler *Handler;
};
