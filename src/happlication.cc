#include <QDebug>
#include <QLayout>
#include <QLabel>
#include <QIcon>
#include <QStatusBar>
#include <QDir>
#include <QTextCodec>
#include <QMessageBox>

extern "C" {
#include "heheader.h"
}
#include "happlication.h"
#include "hmainwindow.h"
#include "hframe.h"
#include "hwindow.h"
#include "settings.h"
#include "hugodefs.h"


HApplication* hApp = 0;


HApplication::HApplication( int& argc, char* argv[], const char* appName, const char* appVersion,
                                  const char* orgName, const char* orgDomain )
    : QApplication(argc, argv),
      fFrameWin(0),
      fGameRunning(false),
      fHugoCodec(QTextCodec::codecForName("Windows-1252"))
{
    //qDebug() << Q_FUNC_INFO;
    Q_ASSERT(hApp == 0);

    this->setApplicationName(QString::fromAscii(appName));
    this->setApplicationVersion(QString::fromAscii(appVersion));
    this->setOrganizationName(QString::fromAscii(orgName));
    this->setOrganizationDomain(QString::fromAscii(orgDomain));

    // Load our persistent settings.
    this->fSettings = new Settings;
    this->fSettings->loadFromDisk();

    // Apply the smart formatting setting.
    smartformatting = this->fSettings->smartFormatting;

    // Set our global pointer.
    hApp = this;

    // Create our main application window.
    this->fMainWin = new HMainWindow(0);
    this->fMainWin->setWindowTitle(QString::fromAscii(appName));
    //this->fMainWin->updateRecentGames();

    this->fFrameWin = new HFrame(hMainWin);
    this->fMainWin->setCentralWidget(this->fFrameWin);
    //this->fGameWin->resize(qWinGroup->centralWidget()->size());
    this->fFrameWin->show();
    //this->fGameWin->setFocus();

    // Set application window icon.
    this->setWindowIcon(QIcon(":/he_32-bit_48x48.png"));

    // Automatically quit the application when the last window has closed.
    connect(this, SIGNAL(lastWindowClosed()), this, SLOT(quit()));
}


HApplication::~HApplication()
{
    //qDebug() << Q_FUNC_INFO;
    Q_ASSERT(hApp != 0);
    this->fSettings->saveToDisk();
    delete this->fSettings;
    delete this->fMainWin;
    // We're being destroyed, so our global pointer is no longer valid.
    hApp = 0;
}


void
HApplication::fRunGame()
{
    if (this->fNextGame.isEmpty()) {
        // Nothing to run.
        return;
    }

    while (not this->fNextGame.isEmpty()) {
        QFileInfo finfo(QFileInfo(this->fNextGame).absoluteFilePath());
        this->fNextGame.clear();

        // Remember the directory of the game.
        this->fSettings->lastFileOpenDir = finfo.absolutePath();

        // Change to the game file's directory.
        QDir::setCurrent(finfo.absolutePath());

        if (true) {
            // Delete the current game window.
            //if (this->fGameWin != 0) {
                //delete this->fGameWin;
            //}

            // Recreate them.
            //this->fGameWin = new CHtmlSysWinInputQt(this->fFormatter, qWinGroup->centralWidget());
            //this->fGameWin->resize(qWinGroup->centralWidget()->size());
            //this->fGameWin->show();
            //this->fGameWin->setFocus();

            // Set the application's window title to contain the filename of
            // the game we're running. The game is free to change that later on.
#ifdef Q_WS_MAC
            // Just use the filename on OS X.  Seems to be the norm there.
            //qWinGroup->setWindowTitle(finfo.fileName());
#else
            // On all other systems, also append the application name.
            //qWinGroup->setWindowTitle(finfo.fileName() + QString::fromAscii(" - ") + qFrame->applicationName());
#endif

            // Add the game file to our "recent games" list.
            QStringList& gamesList = this->fSettings->recentGamesList;
            int recentIdx = gamesList.indexOf(finfo.absoluteFilePath());
            if (recentIdx > 0) {
                // It's already in the list and it's not the first item.  Make
                // it the first item so that it becomes the most recent entry.
                gamesList.move(recentIdx, 0);
            } else if (recentIdx < 0) {
                // We didn't find it in the list by absoluteFilePath(). Try to
                // find it by canonicalFilePath() instead. This way, we avoid
                // listing the same game twice if the user opened it through a
                // different path (through a symlink that leads to the same
                // file, for instance.)
                bool found = false;
                const QString& canonPath = finfo.canonicalFilePath();
                for (recentIdx = 0; recentIdx < gamesList.size() and not found; ++recentIdx) {
                    if (QFileInfo(gamesList.at(recentIdx)).canonicalFilePath() == canonPath) {
                        found = true;
                    }
                }
                if (found) {
                    gamesList.move(recentIdx - 1, 0);
                } else {
                    // It's not in the list.  Prepend it as the most recent item
                    // and, if the list is full, delete the oldest one.
                    if (gamesList.size() >= this->fSettings->recentGamesCapacity) {
                        gamesList.removeLast();
                    }
                    gamesList.prepend(finfo.absoluteFilePath());
                }
            }
            //this->fMainWin->updateRecentGames();
            this->fSettings->saveToDisk();

            // Run the Hugo engine.
            this->fGameRunning = true;
            this->fGameFile = finfo.absoluteFilePath();
            char argv0[] = "heqt";
            char* argv1 = new char[this->fGameFile.toLocal8Bit().size() + 1];
            strcpy(argv1, this->fGameFile.toLocal8Bit().constData());
            char* argv[2] = {argv0, argv1};
            emit gameStarting();
            he_main(2, argv);
            this->fGameRunning = false;
            this->fGameFile.clear();
            emit gameHasQuit();

            // Flush any pending output and cancel all sounds and animations.
            //this->flush_txtbuf(true, false);

            // Display a "game has ended" message.
            //QString endMsg(QString::fromAscii("<p><br><font face=tads-serif size=-1>(The game has ended.)</font></p>"));
            //this->display_output(endMsg.toUtf8().constData(), endMsg.length());
            //this->flush_txtbuf(true, false);
        } else {
            //QMessageBox::critical(this->fMainWin, tr("Open Game"), finfo.fileName() + tr(" is not a TADS game file."));
        }
    }
    hApp->quit();

    // Reset application window title.
    //qWinGroup->setWindowTitle(qFrame->applicationName());
}


#ifdef Q_WS_MAC
/*
#include <QFileOpenEvent>
bool
HApplication::event( QEvent* e )
{
    // We only handle the FileOpen event.
    if (e->type() != QEvent::FileOpen) {
        return QApplication::event(e);
    }
    return qWinGroup->handleFileOpenEvent(static_cast<QFileOpenEvent*>(e));
}
*/
#endif


void
HApplication::main( QString gameFileName )
{
    // Restore the application's size.
    this->fMainWin->resize(this->fSettings->appSize);
    this->fMainWin->show();

    // If a game file was specified, try to run it.
    if (not gameFileName.isEmpty()) {
        this->setNextGame(gameFileName);
    }
}


void
HApplication::notifyPreferencesChange( const Settings* sett )
{
    smartformatting = sett->smartFormatting;

    // Recalculate font dimensions, in case font settings have changed.
    calcFontDimensions();

    // Change the text cursor's height according to the new input font's height.
    //qFrame->gameWindow()->setCursorHeight(QFontMetrics(sett->inputFont).height());
    display_needs_repaint = true;
    if (not sett->enableMusic) {
        hugo_stopmusic();
    }
    if (not sett->enableSoundEffects) {
        hugo_stopsample();
    }
}
