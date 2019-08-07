#include "GraphicsView.h"

#include <QTimeLine>
#include <QWheelEvent>
#include <QtMath>

CGraphicsView::CGraphicsView(QWidget *parent)
    : QGraphicsView(parent)
{
    setCacheMode(QGraphicsView::CacheBackground);
    setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    setRenderHint(QPainter::Antialiasing);
    setDragMode(RubberBandDrag);
    viewport()->setAttribute(Qt::WA_AcceptTouchEvents);
}

CGraphicsView::CGraphicsView(QGraphicsScene *scene, QWidget *parent)
    : QGraphicsView(scene, parent)
{
    setCacheMode(QGraphicsView::CacheBackground);
    setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    setRenderHint(QPainter::Antialiasing);
    setDragMode(RubberBandDrag);
    viewport()->setAttribute(Qt::WA_AcceptTouchEvents);
}

void CGraphicsView::PanToSmooth(qreal x, qreal y)
{
    auto currCenter = mapToScene(viewport()->rect().center());
    auto target = QPointF(x, y);

    auto delta = target - currCenter;
    qreal dist = qLn(delta.x() * delta.x() + delta.y() * delta.y());
    int duration = dist * 20.0f;

    if (CenterOnAnim)
        delete CenterOnAnim;
    CenterOnAnim = new QVariantAnimation();
    CenterOnAnim->setEasingCurve(QEasingCurve::InOutCirc);
    CenterOnAnim->setDuration(duration);
    CenterOnAnim->setStartValue(currCenter);
    CenterOnAnim->setEndValue(target);
    connect(CenterOnAnim, &QVariantAnimation::valueChanged, [this](const QVariant &value) {
        this->centerOn(value.toPointF());
    });
    CenterOnAnim->start();
}

bool CGraphicsView::viewportEvent(QEvent *event)
{
    switch (event->type()) {
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd: {
        QTouchEvent *touchEvent = static_cast<QTouchEvent *>(event);
        QList<QTouchEvent::TouchPoint> touchPoints = touchEvent->touchPoints();
        if (touchPoints.count() == 2) {
            // determine scale factor
            const QTouchEvent::TouchPoint &touchPoint0 = touchPoints.first();
            const QTouchEvent::TouchPoint &touchPoint1 = touchPoints.last();
            qreal currentScaleFactor = QLineF(touchPoint0.pos(), touchPoint1.pos()).length()
                                       / QLineF(touchPoint0.startPos(), touchPoint1.startPos())
                                             .length();
            if (touchEvent->touchPointStates() & Qt::TouchPointReleased) {
                // if one of the fingers is released, remember the current scale
                // factor so that adding another finger later will continue zooming
                // by adding new scale factor to the existing remembered value.
                TotalScaleFactor *= currentScaleFactor;
                currentScaleFactor = 1;
            }
            setTransform(QTransform().scale(TotalScaleFactor * currentScaleFactor,
                                            TotalScaleFactor * currentScaleFactor));
        }
        return true;
    }
    default:
        break;
    }
    return QGraphicsView::viewportEvent(event);
}
