#include <QColorDialog>
#include <QSignalMapper>
#include <QPushButton>
#include <QCheckBox>

#include "confdialog.h"
#include "ui_confdialog.h"
#include "settings.h"
#include "happlication.h"
#include "hmainwindow.h"
#include "hugodefs.h"


ConfDialog::ConfDialog( HMainWindow* parent )
    : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
      ui(new Ui::ConfDialog)
{
    ui->setupUi(this);
    Settings* sett = hApp->settings();
    sett->loadFromDisk();

    fInitialSoundVol = sett->soundVolume;

#ifdef Q_OS_MAC
    // On the Mac, make the color selection buttons smaller so that they
    // become square instead of round.
    QSize macSize(48, 24);
    ui->mainBgColorButton->setFixedSize(macSize);
    ui->mainTextColorButton->setFixedSize(macSize);
    ui->bannerBgColorButton->setFixedSize(macSize);
    ui->bannerTextColorButton->setFixedSize(macSize);
    ui->fsMarginColorButton->setFixedSize(macSize);
#endif

    ui->allowGraphicsCheckBox->setChecked(sett->enableGraphics);
#ifdef DISABLE_VIDEO
    ui->allowVideoCheckBox->setDisabled(true);
#else
    ui->allowVideoCheckBox->setChecked(sett->enableVideo);
#endif
#ifdef DISABLE_AUDIO
    ui->allowSoundEffectsCheckBox->setDisabled(true);
    ui->allowMusicCheckBox->setDisabled(true);
    ui->muteSoundCheckBox->setDisabled(true);
#else
    ui->allowSoundEffectsCheckBox->setChecked(sett->enableSoundEffects);
    ui->allowMusicCheckBox->setChecked(sett->enableMusic);
    ui->muteSoundCheckBox->setChecked(sett->muteSoundInBackground);
#endif
#if defined(DISABLE_VIDEO) and defined(DISABLE_AUDIO)
    ui->volumeSlider->setValue(0);
    ui->volumeSlider->setDisabled(true);
#else
    ui->volumeSlider->setValue(sett->soundVolume);
    connect(ui->volumeSlider, SIGNAL(valueChanged(int)), SLOT(fSetSoundVolume(int)));
#endif
    ui->smoothScalingCheckBox->setChecked(sett->useSmoothScaling);

    ui->mainBgColorButton->setColor(sett->mainBgColor);
    ui->mainTextColorButton->setColor(sett->mainTextColor);
    ui->bannerBgColorButton->setColor(sett->statusBgColor);
    ui->bannerTextColorButton->setColor(sett->statusTextColor);
    ui->fsMarginColorButton->setColor(sett->fsMarginColor);
    connect(ui->customFsMarginColorCheckBox, SIGNAL(toggled(bool)), ui->fsMarginColorButton, SLOT(setEnabled(bool)));
    connect(ui->customFsMarginColorCheckBox, SIGNAL(toggled(bool)), ui->fsMarginColorLabel, SLOT(setEnabled(bool)));
    ui->customFsMarginColorCheckBox->setChecked(sett->customFsMarginColor);

    ui->mainFontSizeSpinBox->setValue(sett->propFont.pointSize());
    ui->fixedFontSizeSpinBox->setValue(sett->fixedFont.pointSize());
    ui->scrollbackFontSizeSpinBox->setValue(sett->scrollbackFont.pointSize());
    ui->mainFontBox->setCurrentFont(sett->propFont);
    ui->fixedFontBox->setCurrentFont(sett->fixedFont);
    ui->scrollbackFontBox->setCurrentFont(sett->scrollbackFont);
    ui->softScrollCheckBox->setChecked(sett->softTextScrolling);
    ui->smartFormattingCheckBox->setChecked(sett->smartFormatting);
    ui->marginSizeSpinBox->setValue(sett->marginSize);
    ui->overlayScrollbackCheckBox->setChecked(sett->overlayScrollback);
    ui->fullscreenWidthSpinBox->setValue(sett->fullscreenWidth);
    if (sett->scriptWrap < ui->scriptWrapSpinBox->minimum()) {
        ui->scriptWrapSpinBox->setValue(ui->scriptWrapSpinBox->minimum());
    } else {
        ui->scriptWrapSpinBox->setValue(sett->scriptWrap);
    }

#ifdef Q_OS_MAC
    // On Mac OS X, the dialog should not have any buttons, and settings
    // changes should apply instantly.
    this->fMakeInstantApply();
    ui->buttonBox->setStandardButtons(QDialogButtonBox::NoButton);
#else
    if (hApp->desktopIsGnome()) {
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
        connect(this, SIGNAL(rejected()), SLOT(fCancel()));
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
    connect(ui->scrollbackFontBox, SIGNAL(currentFontChanged(QFont)), this, SLOT(fApplySettings()));
    connect(ui->mainFontSizeSpinBox, SIGNAL(valueChanged(int)), this, SLOT(fApplySettings()));
    connect(ui->fixedFontSizeSpinBox, SIGNAL(valueChanged(int)), this, SLOT(fApplySettings()));
    connect(ui->scrollbackFontSizeSpinBox, SIGNAL(valueChanged(int)), this, SLOT(fApplySettings()));
    connect(ui->softScrollCheckBox, SIGNAL(toggled(bool)), this, SLOT(fApplySettings()));
    connect(ui->smartFormattingCheckBox, SIGNAL(toggled(bool)), this, SLOT(fApplySettings()));
    connect(ui->marginSizeSpinBox, SIGNAL(valueChanged(int)), this, SLOT(fApplySettings()));
    connect(ui->fullscreenWidthSpinBox, SIGNAL(valueChanged(int)), this, SLOT(fApplySettings()));
    connect(ui->scriptWrapSpinBox, SIGNAL(valueChanged(int)), this, SLOT(fApplySettings()));
    connect(ui->allowGraphicsCheckBox, SIGNAL(toggled(bool)), this, SLOT(fApplySettings()));
    connect(ui->allowVideoCheckBox, SIGNAL(toggled(bool)), this, SLOT(fApplySettings()));
    connect(ui->allowSoundEffectsCheckBox, SIGNAL(toggled(bool)), this, SLOT(fApplySettings()));
    connect(ui->allowMusicCheckBox, SIGNAL(toggled(bool)), this, SLOT(fApplySettings()));
    connect(ui->smoothScalingCheckBox, SIGNAL(toggled(bool)), this, SLOT(fApplySettings()));
    connect(ui->muteSoundCheckBox, SIGNAL(toggled(bool)), this, SLOT(fApplySettings()));
    connect(ui->volumeSlider, SIGNAL(valueChanged(int)), SLOT(fApplySettings()));
    connect(ui->overlayScrollbackCheckBox, SIGNAL(toggled(bool)), this, SLOT(fApplySettings()));
    connect(ui->mainTextColorButton, SIGNAL(changed(QColor)), this, SLOT(fApplySettings()));
    connect(ui->mainBgColorButton, SIGNAL(changed(QColor)), this, SLOT(fApplySettings()));
    connect(ui->bannerTextColorButton, SIGNAL(changed(QColor)), this, SLOT(fApplySettings()));
    connect(ui->bannerBgColorButton, SIGNAL(changed(QColor)), this, SLOT(fApplySettings()));
    connect(ui->customFsMarginColorCheckBox, SIGNAL(toggled(bool)), this, SLOT(fApplySettings()));
    connect(ui->fsMarginColorButton, SIGNAL(changed(QColor)), this, SLOT(fApplySettings()));
}


void
ConfDialog::fApplySettings()
{
    Settings* sett = hApp->settings();

    sett->enableGraphics = ui->allowGraphicsCheckBox->isChecked();
    sett->enableVideo = ui->allowVideoCheckBox->isChecked();
    sett->enableSoundEffects = ui->allowSoundEffectsCheckBox->isChecked();
    sett->enableMusic = ui->allowMusicCheckBox->isChecked();
    sett->useSmoothScaling = ui->smoothScalingCheckBox->isChecked();
    sett->muteSoundInBackground = ui->muteSoundCheckBox->isChecked();
    sett->soundVolume = ui->volumeSlider->value();
    sett->mainBgColor = ui->mainBgColorButton->color();
    sett->mainTextColor = ui->mainTextColorButton->color();
    sett->statusBgColor = ui->bannerBgColorButton->color();
    sett->statusTextColor = ui->bannerTextColorButton->color();
    sett->customFsMarginColor = ui->customFsMarginColorCheckBox->isChecked();
    sett->fsMarginColor = ui->fsMarginColorButton->color();
    sett->propFont = ui->mainFontBox->currentFont();
    sett->fixedFont = ui->fixedFontBox->currentFont();
    sett->scrollbackFont = ui->scrollbackFontBox->currentFont();
    sett->propFont.setPointSize(ui->mainFontSizeSpinBox->value());
    sett->fixedFont.setPointSize(ui->fixedFontSizeSpinBox->value());
    sett->scrollbackFont.setPointSize(ui->scrollbackFontSizeSpinBox->value());
    sett->softTextScrolling = ui->softScrollCheckBox->isChecked();
    sett->smartFormatting = ui->smartFormattingCheckBox->isChecked();
    sett->overlayScrollback = ui->overlayScrollbackCheckBox->isChecked();
    sett->marginSize = ui->marginSizeSpinBox->value();
    sett->fullscreenWidth = ui->fullscreenWidthSpinBox->value();
    if (ui->scriptWrapSpinBox->value() <= ui->scriptWrapSpinBox->minimum()) {
        sett->scriptWrap = 0;
    } else {
        sett->scriptWrap = ui->scriptWrapSpinBox->value();
    }

    // Notify the application that preferences have changed.
    hApp->notifyPreferencesChange(sett);

    sett->saveToDisk();

    fInitialSoundVol = sett->soundVolume;
}


void
ConfDialog::fCancel()
{
    hApp->settings()->soundVolume = fInitialSoundVol;
    updateMusicVolume();
    updateSoundVolume();
    updateVideoVolume();
}


void
ConfDialog::fSetSoundVolume(int vol)
{
    hApp->settings()->soundVolume = vol;
    updateMusicVolume();
    updateSoundVolume();
    updateVideoVolume();
}
