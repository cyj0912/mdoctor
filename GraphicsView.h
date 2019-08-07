#pragma once
#include <QGraphicsView>
#include <QTimer>
#include <QVariantAnimation>

class CGraphicsView : public QGraphicsView
{
public:
    CGraphicsView(QWidget *parent = nullptr);
    CGraphicsView(QGraphicsScene *scene, QWidget *parent = nullptr);

    void PanToSmooth(qreal x, qreal y);

protected:
    bool viewportEvent(QEvent *event) override;

private:
    qreal TotalScaleFactor = 1.0;

    QVariantAnimation *CenterOnAnim = nullptr;
};
