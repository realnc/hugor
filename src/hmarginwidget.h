#ifndef HMARGINWIDGET_H
#define HMARGINWIDGET_H

#include <QWidget>


class HMarginWidget: public QWidget {
    Q_OBJECT

  private:
    QWidget* fBannerWidget;
    class QVBoxLayout* fLayout;

  public:
    HMarginWidget( QWidget* parent = 0 );

    void
    setBannerWidget( QWidget* w );

    QWidget*
    bannerWidget()
    { return this->fBannerWidget; }

    void
    addWidget( QWidget* w );

    void
    removeWidget( QWidget* w );

  protected:
    virtual void
    wheelEvent( QWheelEvent* e );
};


#endif
