/* Copyright 2015 Nikos Chantziaras
 *
 * This file is part of Hugor.
 *
 * Hugor is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * Foobar is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Hugor.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Additional permission under GNU GPL version 3 section 7
 *
 * If you modify this Program, or any covered work, by linking or combining it
 * with the Hugo Engine (or a modified version of the Hugo Engine), containing
 * parts covered by the terms of the Hugo Engine License, the licensors of this
 * Program grant you additional permission to convey the resulting work.
 * Corresponding Source for a non-source form of such a combination shall
 * include the source code for the parts of the Hugo Engine used as well as
 * that of the covered work.
 */
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
