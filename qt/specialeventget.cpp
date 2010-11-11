#include "specialeventget.h"
#include <QMouseEvent>
#include <QDebug>
#include <QWidget>
#include <QqWidget.h>

extern QSize viewSize;

SpecialEventGet::SpecialEventGet(QObject* parent) : QObject(parent)
{
    shift.setX(0);
    shift.setY(0);
    offset.setX(0);
    offset.setY(0);
}

bool SpecialEventGet::eventFilter(QObject *obj, QEvent *event)
{
    if(event->type() == QEvent::Move) {
        QMoveEvent* evt = static_cast<QMoveEvent*>(event);
    } else if(event->type() == QEvent::Resize) {
        QResizeEvent* evt = static_cast<QResizeEvent*>(event);
        m_OldSize = QSize(evt->oldSize().width(), evt->oldSize().height());
    } else if(event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent* evt = static_cast<QMouseEvent*>(event);
        btn= evt->button();
        offset = evt->pos();
    } else if(event->type() == QEvent::MouseMove)
    {
        QMouseEvent* evt = static_cast<QMouseEvent*>(event);
        FakeWindow *fake = static_cast<FakeWindow*>(obj);
        Context* context = fake->getContext();
        Layer* layer = fake->getLayer();
        TextLayer* textlayer = fake->getTextLayer();
        if (btn==Qt::LeftButton)  //change position, only layers
        {
            if (layer)
            {
                fake->move(fake->mapToParent(evt->pos() - offset));
                layer->set_position ((fake->pos().x() + shift.x()), (fake->pos().y()+shift.y()));
                context->screen->clear();
                //qDebug() << "posX:" << fake->pos().x() << "shiftX:" << shift.x();
            }
            else if (textlayer)
            {
                fake->move(fake->mapToParent(evt->pos() - offset));
                textlayer->set_position ((fake->pos().x() + shift.x()), (fake->pos().y()+shift.y()));
                context->screen->clear();
            }
            // todo textlayer dans QqWidget
        }
        else if (btn==Qt::RightButton && layer) //resize
        {
            int sizeW = fake->size().width() + evt->pos().x() - offset.x();
            int sizeH = fake->size().height() + evt->pos().y() - offset.y();
            fake->resize(sizeW, sizeH);
            offset = evt->pos();
            double x1, y1;
            x1 = m_OldSize.width() * 1.0 / layer->geo.w;
            y1 = m_OldSize.height() * 1.0 / layer->geo.h;
            layer->set_zoom(x1, y1);
            shift.setX((int)((m_OldSize.width() - layer->geo.w)/2));
            shift.setY((int)((m_OldSize.height() - layer->geo.h)/2));
            layer->set_position ((shift.x()+fake->pos().x()), (shift.y()+fake->pos().y()));
            context->screen->clear();
            //qDebug() << "zx:" << x1 << "sX:" << shift.x() << "posX:" << fake->pos().x();
        }
        else if (btn==Qt::RightButton && textlayer)
        {
            int sizeW = fake->size().width() + evt->pos().x() - offset.x();
            int sizeH = fake->size().height() + evt->pos().y() - offset.y();
            fake->resize(sizeW, sizeH);
            offset = evt->pos();
            double x1, y1;
            x1 = m_OldSize.width() * 1.0 / textlayer->geo.w;
            y1 = m_OldSize.height() * 1.0 / textlayer->geo.h;
            textlayer->set_zoom(x1, y1);
            shift.setX((int)((m_OldSize.width() - textlayer->geo.w)/2));
            shift.setY((int)((m_OldSize.height() - textlayer->geo.h)/2));
            textlayer->set_position ((shift.x()+fake->pos().x()), (shift.y()+fake->pos().y()));
            context->screen->clear();
            //qDebug() << "zx:" << x1 << "sX:" << shift.x() << "posX:" << fake->pos().x();
        }
        else if (btn==Qt::RightButton)
        {
            int sizeW = fake->size().width() + evt->pos().x() - offset.x();
            int sizeH = fake->size().height() + evt->pos().y() - offset.y();
            fake->resize(sizeW, sizeH);
            viewSize.setWidth(sizeW);
            viewSize.setHeight(sizeH);
            context->screen->resize(sizeW, sizeH);
            offset = evt->pos();
            context->screen->clear();
        }
        else if (btn==Qt::MiddleButton)
        {
            if (layer)
            {
                if (evt->pos().x() - offset.x() > 0)
                {
                    int angle = fake->getAngle() + (evt->pos().x() - offset.x()) / 10 % 360;
                    if (angle >= 360)
                        angle = 0;
                    qDebug() << "angle:" << angle;
                    fake->setAngle(angle);
                }
                else if (evt->pos().x() - offset.x() < 0)
                {
                    int angle = fake->getAngle() + (evt->pos().x() - offset.x()) / 10 % 360;
                    if (angle <= -360)
                        angle = 0;
                    qDebug() << "angle:" << angle;
                    fake->setAngle(angle);
                }
                fake->repaint();
                layer->set_rotate(- fake->getAngle());
                context->screen->clear();
            }
            else if (textlayer)
            {
                if (evt->pos().x() - offset.x() > 0)
                {
                    int angle = fake->getAngle() + (evt->pos().x() - offset.x()) / 10 % 360;
                    if (angle >= 360)
                        angle = 0;
                    fake->setAngle(angle);
                }
                else if (evt->pos().x() - offset.x() < 0)
                {
                    int angle = fake->getAngle() + (evt->pos().x() - offset.x()) / 10 % 360;
                    if (angle <= -360)
                        angle = 0;
                    fake->setAngle(angle);
                }
                fake->repaint();
                textlayer->set_rotate(- fake->getAngle());
                context->screen->clear();
            }
        }
    }
    else if(event->type() == QEvent::MouseButtonRelease)
    {
        offset = QPoint();
    }
    else if(event->type() == QEvent::Paint)
    {
        FakeWindow *fake = static_cast<FakeWindow*>(obj);
        if (fake->getLayer() || fake->getTextLayer())
        {
            QqWidget *widg = qobject_cast<QqWidget *>(this->parent());
            if (widg)
            {
                widg->setAngle((double)fake->getAngle());
            }
            QPainter* painter = fake->getPainter();
            if (painter->begin(fake))
            {
                painter->save();
                QColor color(127, 127, 127, 191);
                QRect size(fake->geometry());
                size.setTopLeft(QPoint(0,0));
                QPoint center = size.center();
                painter->translate(center.x(), center.y());
                painter->setPen(Qt::NoPen);
                painter->setBrush(color);
                painter->rotate(fake->getAngle());
                painter->setRenderHint(QPainter::Antialiasing);
                size = QRect(-center.x(), -center.y(), size.width(), size.height());
                painter->drawRect(size);
                painter->restore();
                painter->end();
            }
        }
    }
    return true;
}


