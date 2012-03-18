#ifndef HSCROLLBACK_H
#define HSCROLLBACK_H

#include <QTextEdit>


class HScrollbackWindow: public QTextEdit {
    Q_OBJECT

  public:
    HScrollbackWindow( QWidget* parent = 0 );

  protected:
    virtual void
    keyPressEvent( QKeyEvent* e );

  private:
    int fMaximumBlockCount;
    int fInitialWidth;
    int fInitialHeight;
};


#endif
