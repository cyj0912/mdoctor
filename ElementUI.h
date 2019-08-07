#pragma once
#include <QGraphicsItem>
#include <QPainterPath>

#include <set>
#include <utility>

class CConnectionUI;

class CElementUI : public QGraphicsItem
{
public:
    explicit CElementUI(uint32_t eindex,
                        const std::string &name,
                        const std::string &className,
                        const std::string &config,
                        QGraphicsItem *parent = nullptr);
    ~CElementUI() override;

    // QGraphicsItem interface
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    enum { Type = UserType + 1 };
    int type() const override { return Type; }

    typedef std::pair<uint32_t, CConnectionUI *> CConnInfo;
    static QPointF GetPortPosition(int inout, uint32_t index);
    const std::set<CConnInfo> &GetConnections(uint32_t inout) const
    {
        if (inout == 0)
            return InputPorts;
        else
            return OutputPorts;
    }

    uint32_t GetEIndex() const { return EIndex; }
    std::string GetName() const { return Name.toStdString(); }
    const QString &GetClassName() const { return ClassName; }
    const QString &GetConfigStr() const { return ConfigStr; }

    void SetExtraComment(const QString &str);
    const QString &GetExtraComment() const { return ExtraComment; }

    // State changes (may invovle repaint)
    void AddConnection(uint32_t inOrOut, uint32_t index, CConnectionUI *conn);
    void RemoveConnection(uint32_t inOrOut, uint32_t index, CConnectionUI *conn);

    enum class EState { Normal = 0, Selected = 1 };
    void SetState(EState value);

protected:
    // Event and interactoin handling
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

private:
    void UpdateMaxIndexSeen(uint32_t inout, uint32_t index);

    friend class CConnectionUI;

    const uint32_t EIndex;
    const QString Name;
    const QString ClassName;
    const QString ConfigStr;
    QString ExtraComment;

    std::set<CConnInfo> InputPorts, OutputPorts;

    uint32_t MaxIndexSeen[2]{};
    qreal CardHeight = 30.0;

    EState State = EState::Normal;
};
