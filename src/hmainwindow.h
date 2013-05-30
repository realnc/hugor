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
    bool fMenuBarVisible;
#if QT_VERSION >= 0x040600
    QIcon fFullscreenEnterIcon;
    QIcon fFullscreenExitIcon;
#endif

    void
    fFullscreenAdjust();

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

    void
    hideMenuBar();

    void
    showMenuBar();

  public slots:
    void
    showScrollback();

    void
    hideScrollback();

    void
    toggleFullscreen();
};


#endif // HMAINWINDOW_H
