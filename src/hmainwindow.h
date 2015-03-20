#ifndef HMAINWINDOW_H
#define HMAINWINDOW_H

#include <QMainWindow>


extern class HMainWindow* hMainWin;


class HMainWindow: public QMainWindow {
    Q_OBJECT

  private:
    class QErrorMessage* fErrorMsg;
    class ConfDialog* fConfDialog;
    class AboutDialog* fAboutDialog;
    class HScrollbackWindow* fScrollbackWindow;
    class QAction* fPreferencesAction;
    class QAction* fFullscreenAction;
    class QAction* fScrollbackAction;
    bool fMenuBarVisible;
    QIcon fFullscreenEnterIcon;
    QIcon fFullscreenExitIcon;

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

    virtual void
    contextMenuEvent( QContextMenuEvent* e );

  public:
    HMainWindow( QWidget* parent );

    void
    hideMenuBar();

    void
    showMenuBar();

    QErrorMessage*
    errorMsgObj()
    { return this->fErrorMsg; }

    void
    setScrollbackFont( const QFont& font );

  public slots:
    void
    appendToScrollback( const QByteArray& str );

    void
    showScrollback();

    void
    hideScrollback();

    void
    toggleFullscreen();
};


#endif // HMAINWINDOW_H
