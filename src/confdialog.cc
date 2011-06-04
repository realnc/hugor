#include <QColorDialog>
#include <QSignalMapper>
#include <QPushButton>
//#include <QTextCodec>

#include "confdialog.h"
#include "ui_confdialog.h"
#include "settings.h"
#include "happlication.h"
#include "hmainwindow.h"
//#include "syswingroup.h"


ConfDialog::ConfDialog( HMainWindow* parent )
    : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
      ui(new Ui::ConfDialog)
{
    ui->setupUi(this);
    Settings* sett = hApp->settings();
    sett->loadFromDisk();

#ifdef Q_WS_MAC
    // On the Mac, make the color selection buttons smaller so that they
    // become square instead of round.
    QSize macSize(48, 24);
    ui->mainBgColorButton->setFixedSize(macSize);
    ui->mainTextColorButton->setFixedSize(macSize);
    ui->bannerBgColorButton->setFixedSize(macSize);
    ui->bannerTextColorButton->setFixedSize(macSize);
#endif

    ui->allowGraphicsCheckBox->setChecked(sett->enableGraphics);
    ui->allowSoundEffectsCheckBox->setChecked(sett->enableSoundEffects);
    ui->allowMusicCheckBox->setChecked(sett->enableMusic);
    ui->smoothScalingCheckBox->setChecked(sett->useSmoothScaling);

    ui->mainBgColorButton->setColor(sett->mainBgColor);
    ui->mainTextColorButton->setColor(sett->mainTextColor);
    ui->bannerBgColorButton->setColor(sett->statusBgColor);
    ui->bannerTextColorButton->setColor(sett->statusTextColor);

    ui->mainFontSizeSpinBox->setValue(sett->propFont.pointSize());
    ui->fixedFontSizeSpinBox->setValue(sett->fixedFont.pointSize());
    ui->mainFontBox->setCurrentFont(sett->propFont);
    ui->fixedFontBox->setCurrentFont(sett->fixedFont);

    ui->askForGameFileCheckBox->setChecked(sett->askForGameFile);

#ifdef Q_WS_MAC
    // On Mac OS X, the dialog should not have any buttons, and settings
    // changes should apply instantly.
    this->fMakeInstantApply();
    ui->buttonBox->setStandardButtons(QDialogButtonBox::NoButton);
#else
    QDialogButtonBox::ButtonLayout layoutPolicy
        = QDialogButtonBox::ButtonLayout(ui->buttonBox->style()->styleHint(QStyle::SH_DialogButtonLayout));
    if (layoutPolicy == QDialogButtonBox::GnomeLayout) {
        // On Gnome (and other Gtk-based environments, like XFCE), we follow
        // Gnome standards. We only provide a "Close" button and settings
        // changes should apply instantly.
        this->fMakeInstantApply();
        ui->buttonBox->setStandardButtons(QDialogButtonBox::Close);
    } else {
        // Assume KDE/MS Windows standards. No instant apply, and use OK/Apply/Cancel
        // buttons.
        ui->buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel);
        connect(ui->buttonBox->button(QDialogButtonBox::Apply), SIGNAL(clicked()), this, SLOT(fApplySettings()));
        connect(this, SIGNAL(accepted()), this, SLOT(fApplySettings()));
    }
#endif
}


ConfDialog::~ConfDialog()
{
    delete ui;
}


void ConfDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}


void
ConfDialog::fMakeInstantApply()
{
    connect(ui->mainFontBox, SIGNAL(currentFontChanged(QFont)), this, SLOT(fApplySettings()));
    connect(ui->fixedFontBox, SIGNAL(currentFontChanged(QFont)), this, SLOT(fApplySettings()));
    connect(ui->mainFontSizeSpinBox, SIGNAL(valueChanged(int)), this, SLOT(fApplySettings()));
    connect(ui->fixedFontSizeSpinBox, SIGNAL(valueChanged(int)), this, SLOT(fApplySettings()));
    connect(ui->allowGraphicsCheckBox, SIGNAL(toggled(bool)), this, SLOT(fApplySettings()));
    connect(ui->allowSoundEffectsCheckBox, SIGNAL(toggled(bool)), this, SLOT(fApplySettings()));
    connect(ui->allowMusicCheckBox, SIGNAL(toggled(bool)), this, SLOT(fApplySettings()));
    connect(ui->smoothScalingCheckBox, SIGNAL(toggled(bool)), this, SLOT(fApplySettings()));
    connect(ui->mainTextColorButton, SIGNAL(changed(QColor)), this, SLOT(fApplySettings()));
    connect(ui->mainBgColorButton, SIGNAL(changed(QColor)), this, SLOT(fApplySettings()));
    connect(ui->bannerTextColorButton, SIGNAL(changed(QColor)), this, SLOT(fApplySettings()));
    connect(ui->bannerBgColorButton, SIGNAL(changed(QColor)), this, SLOT(fApplySettings()));
    connect(ui->askForGameFileCheckBox, SIGNAL(toggled(bool)), this, SLOT(fApplySettings()));
}


void
ConfDialog::fApplySettings()
{
    Settings* sett = hApp->settings();

    sett->enableGraphics = ui->allowGraphicsCheckBox->isChecked();
    sett->enableSoundEffects = ui->allowSoundEffectsCheckBox->isChecked();
    sett->enableMusic = ui->allowMusicCheckBox->isChecked();
    sett->useSmoothScaling = ui->smoothScalingCheckBox->isChecked();
    sett->mainBgColor = ui->mainBgColorButton->color();
    sett->mainTextColor = ui->mainTextColorButton->color();
    sett->statusBgColor = ui->bannerBgColorButton->color();
    sett->statusTextColor = ui->bannerTextColorButton->color();
    sett->propFont = ui->mainFontBox->currentFont();
    sett->fixedFont = ui->fixedFontBox->currentFont();
    sett->propFont.setPointSize(ui->mainFontSizeSpinBox->value());
    sett->fixedFont.setPointSize(ui->fixedFontSizeSpinBox->value());
    sett->askForGameFile = ui->askForGameFileCheckBox->isChecked();

    // Notify the application that preferences have changed.
    hApp->notifyPreferencesChange(sett);

    sett->saveToDisk();
}
