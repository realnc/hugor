#ifndef HMARGINWIDGET_H
#define HMARGINWIDGET_H

#include <QStackedWidget>


class HMarginWidget: public QStackedWidget {
    Q_OBJECT

  signals:
    // Emitted when scrolling.
    void requestScrollback();

  public:
    HMarginWidget( QWidget* parent = 0 );

  protected:
    void
    wheelEvent( QWheelEvent* e );
};


#endif
