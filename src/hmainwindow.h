#ifndef HMAINWINDOW_H
#define HMAINWINDOW_H

#include <QMainWindow>


extern class HMainWindow* hMainWin;


class HMainWindow: public QMainWindow {
    Q_OBJECT

  private:
    class ConfDialog* fConfDialog;
    class AboutDialog* fAboutDialog;
    class QTextEdit* fScrollbackWindow;
    unsigned int fScrollbackSize;

  private slots:
    void
    fShowConfDialog();

    void
    fHideConfDialog();

    void
    fShowAbout();

    void
    fHideAbout();

    void
    fShowScrollback();

  protected:
    virtual void
    closeEvent( QCloseEvent* e );

  public:
    HMainWindow( QWidget* parent );

    void
    appendToScrollback( const QByteArray& str );
};


#endif // HMAINWINDOW_H
