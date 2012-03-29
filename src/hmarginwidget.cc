#include <QVBoxLayout>
#include <QWheelEvent>

#include "hmarginwidget.h"
#include "hmainwindow.h"


HMarginWidget::HMarginWidget( QWidget* parent )
    : QWidget(parent),
      fBannerWidget(0)
{
    this->fLayout = new QVBoxLayout;
    this->fLayout->setContentsMargins(0, 0, 0, 0);
    this->fLayout->setSpacing(0);
    this->setLayout(this->fLayout);
    this->setBackgroundRole(QPalette::Window);
    this->setAutoFillBackground(true);
}


void
HMarginWidget::wheelEvent( QWheelEvent* e )
{
    if (e->delta() > 0) {
        hMainWin->showScrollback();
    }
    e->accept();
}


void
HMarginWidget::mouseMoveEvent( QMouseEvent* e )
{
    if (this->cursor().shape() == Qt::BlankCursor) {
        this->unsetCursor();
        this->setMouseTracking(false);
    }
    QWidget::mouseMoveEvent(e);
}


void
HMarginWidget::setBannerWidget( QWidget* w )
{
    // If a banner widget is already set, delete it first.
    if (this->fBannerWidget != 0) {
        this->fLayout->removeWidget(this->fBannerWidget);
        this->fBannerWidget->deleteLater();
    }
    this->fBannerWidget = w;
    if (w != 0) {
        w->setParent(this);
        this->fLayout->insertWidget(0, w);
        w->show();
    }
}


void
HMarginWidget::addWidget( QWidget* w )
{
    this->fLayout->addWidget(w);
}


void
HMarginWidget::removeWidget( QWidget* w )
{
    this->fLayout->removeWidget(w);
}
