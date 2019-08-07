#include "FlatConfigToScene.h"

CFlatConfigToScene::CFlatConfigToScene()
{
    Scene = new CClickGraphScene();
}

void CFlatConfigToScene::OnElementDecl(const std::string &name,
                                       const std::string &className,
                                       const std::string &configStr)
{
    Scene->AddElement(ElementCount, name, className, configStr);
    ElementCount++;
}

void CFlatConfigToScene::OnConnection(const std::string &source,
                                      const std::string &sink,
                                      uint32_t srcPort,
                                      uint32_t sinkPort)
{
    Scene->AddConnection(source, sink, srcPort, sinkPort);
}

CClickGraphScene *CFlatConfigToScene::GetScene()
{
    return Scene;
}
