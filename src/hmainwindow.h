#ifndef HMAINWINDOW_H
#define HMAINWINDOW_H

#include <QMainWindow>


extern class HMainWindow* hMainWin;


class HMainWindow: public QMainWindow {
    Q_OBJECT

  protected:
    virtual void
    closeEvent( QCloseEvent* e );

  public:
    HMainWindow( QWidget* parent );
};


#endif // HMAINWINDOW_H
