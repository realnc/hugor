#include <QWheelEvent>

#include "hmarginwidget.h"
#include "hmainwindow.h"


HMarginWidget::HMarginWidget( QWidget* parent )
    : QStackedWidget(parent)
{
    this->setBackgroundRole(QPalette::Window);
    this->setAutoFillBackground(true);

    // Requesting scrollback simply triggers the scrollback window.
    // Since focus is lost, subsequent scrolling/paging events will work as expected.
    connect(this, SIGNAL(requestScrollback()), hMainWin, SLOT(showScrollback()));
}


void
HMarginWidget::wheelEvent( QWheelEvent* e )
{
    if (e->delta() > 0) {
        emit requestScrollback();
    }
    e->accept();
}
