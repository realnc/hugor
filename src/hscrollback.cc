#include <QKeyEvent>

#include "hscrollback.h"
#include "hmainwindow.h"


HScrollbackWindow::HScrollbackWindow( QWidget* parent )
    : QTextEdit(parent),
      fMaximumBlockCount(7000),
      fInitialWidth(600),
      fInitialHeight(440)
{
    this->setWindowFlags(Qt::Window);
    this->setWindowTitle("Hugor Scrollback");
    this->setReadOnly(true);
    this->setUndoRedoEnabled(false);
    this->setTextInteractionFlags(Qt::TextSelectableByMouse);
    this->setFrameStyle(QFrame::NoFrame | QFrame::Plain);
    // Don't allow the scrollbuffer to grow forever; limit it to fMaximumBlockCount lines.
    this->document()->setMaximumBlockCount(this->fMaximumBlockCount);
    this->resize(this->fInitialWidth, this->fInitialHeight);
}


void
HScrollbackWindow::keyPressEvent( QKeyEvent* e )
{
    if (e->matches(QKeySequence::Close) or e->key() == Qt::Key_Escape) {
        this->close();
        hMainWin->activateWindow();
        hMainWin->raise();
        e->accept();
    } else {
        QTextEdit::keyPressEvent(e);
    }
}
