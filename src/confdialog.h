#ifndef CONFDIALOG_H
#define CONFDIALOG_H

#include <QDialog>


namespace Ui {
    class ConfDialog;
}

class ConfDialog: public QDialog {
    Q_OBJECT

  public:
    ConfDialog( class HMainWindow* parent = 0 );
    ~ConfDialog();

  protected:
    void
    changeEvent( QEvent* e );

  private:
    Ui::ConfDialog* ui;
    int fInitialSoundVol;

    // Makes the dialog's controls apply instantly when they change.
    void
    fMakeInstantApply();

  private slots:
    void
    fApplySettings();

    void
    fCancel();

    void
    fSetSoundVolume(int vol);
};


#endif
