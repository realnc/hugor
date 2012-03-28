#ifndef HMAINWINDOW_H
#define HMAINWINDOW_H

#include <QMainWindow>


extern class HMainWindow* hMainWin;


class HMainWindow: public QMainWindow {
    Q_OBJECT

  private:
    class ConfDialog* fConfDialog;
    class AboutDialog* fAboutDialog;
    class HScrollbackWindow* fScrollbackWindow;
    class QAction* fFullscreenAction;
#if QT_VERSION >= 0x040600
    QIcon fFullscreenEnterIcon;
    QIcon fFullscreenExitIcon;
#endif

  private slots:
    void
    fShowConfDialog();

    void
    fHideConfDialog();

    void
    fShowAbout();

    void
    fHideAbout();

  protected:
    virtual void
    closeEvent( QCloseEvent* e );

    virtual void
    changeEvent( QEvent* e );

  public:
    HMainWindow( QWidget* parent );

    void
    appendToScrollback( const QByteArray& str );

  public slots:
    void
    showScrollback();

    void
    hideScrollback();

    void
    setFullscreen( bool f );
};


#endif // HMAINWINDOW_H
