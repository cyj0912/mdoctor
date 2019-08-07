#pragma once
#include "ClickGraphScene.h"
#include "Parser.h"

class CFlatConfigToScene : public IParserHandler
{
public:
    CFlatConfigToScene();

    // IParserHandler interface
    void OnElementDecl(const std::string &name,
                       const std::string &className,
                       const std::string &configStr) override;
    void OnConnection(const std::string &source,
                      const std::string &sink,
                      uint32_t srcPort,
                      uint32_t sinkPort) override;

    CClickGraphScene *GetScene();

private:
    CClickGraphScene *Scene;
    uint32_t ElementCount = 0;
};
