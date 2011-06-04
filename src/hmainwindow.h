#ifndef HMAINWINDOW_H
#define HMAINWINDOW_H

#include <QMainWindow>


extern class HMainWindow* hMainWin;


class HMainWindow: public QMainWindow {
    Q_OBJECT

  private:
    class ConfDialog* fConfDialog;

  private slots:
    void
    fShowConfDialog();

    void
    fHideConfDialog();

  protected:
    virtual void
    closeEvent( QCloseEvent* e );

  public:
    HMainWindow( QWidget* parent );
};


#endif // HMAINWINDOW_H
