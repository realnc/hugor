#ifndef HAPPLICATION_H
#define HAPPLICATION_H

#include <QApplication>


extern class HApplication*  hApp;


class HApplication: public QApplication {
    Q_OBJECT

  private:
    // Preferences (fonts, colors, etc.)
    class Settings* fSettings;

    // Main application window.
    class HMainWindow* fMainWin;

    // Frame widget containing all subwindows.
    class HFrame* fFrameWin;

    // Parent of fFrameWin, provides margins.
    class HMarginWidget* fMarginWidget;

    const int fBottomMarginSize;

    // Are we currently executing a game?
    bool fGameRunning;

    // Filename of the game we're currently executing.
    QString fGameFile;

    // The game we should try to run after the current one ends.
    QString fNextGame;

    // Text codec used by the Hugo engine.
    QTextCodec* fHugoCodec;

    // Run the game file contained in fNextGame.
    void
    fRunGame();

#ifdef Q_WS_MAC
  protected:
    // On the Mac, dropping a file on our application icon will generate a
    // FileOpen event, so we override this to be able to handle it.
    virtual bool
    event( QEvent* );
#endif

  signals:
    // Emitted just prior to starting a game.  The game has not started yet
    // when this is emitted.
    void gameStarting();

    // Emitted prior to quitting a game.  The game has not quit yet when this
    // is emitted.
    void gameQuitting();

    // Emitted after quiting a game.  The game has already quit when this is
    // emitted.
    void gameHasQuit();

  public slots:
    // Replacement for main().  We need this so that we can start the
    // Hugo engine after the QApplication main event loop has started.
    void
    main( QString gameFileName );

  public:
    HApplication( int& argc, char* argv[], const char* appName, const char* appVersion,
                  const char* orgName, const char* orgDomain );

    virtual
    ~HApplication();

    /* Passing a negative value as 'color' will keep the current margin color.
     */
    void
    updateMargins( int color );

    class Settings*
    settings()
    { return this->fSettings; }

    class HFrame*
    frameWindow()
    { return this->fFrameWin; }

    class HMarginWidget*
    marginWidget()
    { return this->fMarginWidget; }

    bool
    gameRunning()
    { return this->fGameRunning; }

    const QString&
    gameFile()
    { return this->fGameFile; }

    void
    setGameRunning( bool f )
    {
        this->fGameRunning = f;
        if (f == false) {
            emit gameQuitting();
        }
    }

    // Notify the application that preferences have changed.
    void
    notifyPreferencesChange( const class Settings* sett );

    // Advance the event loop.
    void
    advanceEventLoop( QEventLoop::ProcessEventsFlags flags = QEventLoop::AllEvents )
    {
        // Guard against re-entrancy.
        static bool working = false;
        if (working) {
            return;
        }
        working = true;

        // DeferredDelete events need to be dispatched manually, since we don't
        // return to the main event loop while a game is running.
#ifndef Q_WS_MAC
        // On OS X, this causes CPU utilization to go through the roof.  Not
        // sure why.  Disable this for now on OS X until further information
        // is available on this.
        this->sendPostedEvents(0, QEvent::DeferredDelete);
#endif
        this->sendPostedEvents();
        this->processEvents(flags);
        this->sendPostedEvents();
        working = false;
    }

    // Advance the event loop with a timeout.
    void
    advanceEventLoop( QEventLoop::ProcessEventsFlags flags, int maxtime )
    {
        // Guard against re-entrancy.
        static bool working = false;
        if (working) {
            return;
        }
        working = true;

#ifndef Q_WS_MAC
        this->sendPostedEvents(0, QEvent::DeferredDelete);
#endif
        this->sendPostedEvents();
        this->processEvents(flags, maxtime);
        this->sendPostedEvents();
        working = false;
    }

    // Text codec used by Hugo.
    QTextCodec*
    hugoCodec()
    { return this->fHugoCodec; }
};


#endif
