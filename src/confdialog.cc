// This is copyrighted software. More information is at the end of this file.
#include "confdialog.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QDebug>
#include <QFileDialog>
#include <QPushButton>
#include <QResource>
#include <QSignalMapper>
#include <QStyle>
#ifndef DISABLE_AUDIO
#include <Aulib/AudioDecoderFluidsynth.h>
#include <Aulib/AudioResamplerSpeex.h>
#include <Aulib/AudioStream.h>
#include <SDL_rwops.h>
#endif
#include <cmath>

#include "happlication.h"
#include "hmainwindow.h"
#include "hugodefs.h"
#include "settings.h"
#include "ui_confdialog.h"

ConfDialog::ConfDialog(HMainWindow* parent)
    : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint)
    , ui_(new Ui::ConfDialog)
{
    ui_->setupUi(this);
    Settings* sett = hApp->settings();
    sett->loadFromDisk();

    const auto& playIcon = style()->standardIcon(QStyle::SP_MediaPlay);
    const auto& stopIcon = style()->standardIcon(QStyle::SP_MediaStop);
    if (playIcon.isNull() or stopIcon.isNull()) {
        ui_->midiPlayButton->setText("Play");
        ui_->midiStopButton->setText("Stop");
    } else {
        ui_->midiPlayButton->setIcon(playIcon);
        ui_->midiStopButton->setIcon(stopIcon);
        ui_->midiPlayButton->adjustSize();
        ui_->midiStopButton->adjustSize();
    }

    ui_->gainLabel->setToolTip(ui_->gainSpinBox->toolTip());
    initial_sound_vol_ = sett->sound_volume;
    initial_gain_ = sett->synth_gain;

#ifdef DISABLE_AUDIO
    ui_->soundFontGroupBox->setEnabled(false);
    ui_->midiTestLabel->setEnabled(false);
    ui_->midiPlayButton->setEnabled(false);
    ui_->midiStopButton->setEnabled(false);
    ui_->gainLabel->setEnabled(false);
    ui_->gainSpinBox->setEnabled(false);
#endif

#ifdef Q_OS_MAC
    // On the Mac, make the color selection buttons smaller so that they
    // become square instead of round.
    QSize macSize(48, 24);
    ui_->mainBgColorButton->setFixedSize(macSize);
    ui_->mainTextColorButton->setFixedSize(macSize);
    ui_->bannerBgColorButton->setFixedSize(macSize);
    ui_->bannerTextColorButton->setFixedSize(macSize);
    ui_->fsMarginColorButton->setFixedSize(macSize);
#endif

    ui_->allowGraphicsCheckBox->setChecked(sett->enable_graphics);
#ifdef DISABLE_VIDEO
    ui_->allowVideoCheckBox->setChecked(false);
    ui_->allowVideoCheckBox->setDisabled(true);
#else
    if (sett->video_sys_error) {
        ui_->allowVideoCheckBox->setChecked(false);
        ui_->allowVideoCheckBox->setDisabled(true);
    } else {
        ui_->allowVideoCheckBox->setChecked(sett->enable_video);
    }
#endif
#ifdef DISABLE_AUDIO
    ui_->allowSoundEffectsCheckBox->setDisabled(true);
    ui_->allowMusicCheckBox->setDisabled(true);
    ui_->muteWhenMinimizedCheckBox->setDisabled(true);
#else
    ui_->allowSoundEffectsCheckBox->setChecked(sett->enable_sound_effects);
    ui_->allowMusicCheckBox->setChecked(sett->enable_music);
    ui_->muteWhenMinimizedCheckBox->setChecked(sett->mute_when_minimized);
#endif
#if defined(DISABLE_VIDEO) and defined(DISABLE_AUDIO)
    ui_->volumeLabel->setDisabled(true);
    ui_->volumeSlider->setValue(0);
    ui_->volumeSlider->setDisabled(true);
#else
    ui_->volumeSlider->setValue(sett->sound_volume);
    connect(ui_->volumeSlider, SIGNAL(valueChanged(int)), SLOT(setSoundVolume(int)));
#endif
    ui_->smoothScalingCheckBox->setChecked(sett->use_smooth_scaling);
    ui_->soundFontGroupBox->setChecked(sett->use_custom_soundfont);
    ui_->soundFontLineEdit->setText(sett->soundfont);
    ui_->gainSpinBox->setValue(sett->synth_gain);

    ui_->mainBgColorButton->setColor(sett->main_bg_color);
    ui_->mainTextColorButton->setColor(sett->main_text_color);
    ui_->bannerBgColorButton->setColor(sett->status_bg_color);
    ui_->bannerTextColorButton->setColor(sett->status_text_color);
    ui_->fsMarginColorButton->setColor(sett->fs_margin_color);
    connect(ui_->customFsMarginColorCheckBox, SIGNAL(toggled(bool)), ui_->fsMarginColorButton,
            SLOT(setEnabled(bool)));
    ui_->customFsMarginColorCheckBox->setChecked(sett->custom_fs_margin_color);

    ui_->mainFontSizeSpinBox->setValue(sett->prop_font.pointSize());
    ui_->fixedFontSizeSpinBox->setValue(sett->fixed_font.pointSize());
    ui_->scrollbackFontSizeSpinBox->setValue(sett->scrollback_font.pointSize());
    ui_->mainFontBox->setCurrentFont(sett->prop_font);
    ui_->fixedFontBox->setCurrentFont(sett->fixed_font);
    ui_->scrollbackFontBox->setCurrentFont(sett->scrollback_font);
    ui_->softScrollCheckBox->setChecked(sett->soft_text_scrolling);
    ui_->smartFormattingCheckBox->setChecked(sett->smart_formatting);
    ui_->marginSizeSpinBox->setValue(sett->margin_size);
    ui_->overlayScrollbackCheckBox->setChecked(sett->overlay_scrollback);
    ui_->fullscreenWidthSpinBox->setValue(sett->fullscreen_width);
    if (sett->script_wrap < ui_->scriptWrapSpinBox->minimum()) {
        ui_->scriptWrapSpinBox->setValue(ui_->scriptWrapSpinBox->minimum());
    } else {
        ui_->scriptWrapSpinBox->setValue(sett->script_wrap);
    }
    if (sett->start_fullscreen) {
        ui_->fullscreenRadioButton->setChecked(true);
    } else if (sett->start_windowed) {
        ui_->windowRadioButton->setChecked(true);
    } else {
        ui_->lastStateRadioButton->setChecked(true);
    }

#ifdef Q_OS_MAC
    // On Mac OS X, the dialog should not have any buttons, and settings
    // changes should apply instantly.
    makeInstantApply();
    ui_->buttonBox->setStandardButtons(QDialogButtonBox::NoButton);
#else
    if (hApp->desktopIsGnome()) {
        // On Gnome (and other Gtk-based environments, like XFCE), we follow Gnome standards. We
        // only provide a "Close" button and settings changes should apply instantly.
        makeInstantApply();
        ui_->buttonBox->setStandardButtons(QDialogButtonBox::Close);
    } else {
        // Assume KDE/MS Windows standards. No instant apply, and use OK/Apply/Cancel buttons.
        ui_->buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Apply
                                           | QDialogButtonBox::Cancel);
        connect(ui_->buttonBox->button(QDialogButtonBox::Apply), SIGNAL(clicked()), this,
                SLOT(applySettings()));
        connect(this, SIGNAL(accepted()), this, SLOT(applySettings()));
        connect(this, SIGNAL(rejected()), SLOT(cancel()));
    }
#endif

    connect(ui_->midiPlayButton, &QPushButton::clicked, this, &ConfDialog::playTestMidi);
    connect(ui_->midiStopButton, &QPushButton::clicked, this, &ConfDialog::stopTestMidi);
    connect(ui_->soundFontPushButton, &QPushButton::clicked, this, &ConfDialog::browseForSoundFont);
    connect(ui_->gainSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            &ConfDialog::setGain);
}

ConfDialog::~ConfDialog()
{
    delete ui_;
}

void ConfDialog::changeEvent(QEvent* e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui_->retranslateUi(this);
        break;
    default:
        break;
    }
}

void ConfDialog::makeInstantApply()
{
    connect(ui_->mainFontBox, SIGNAL(currentFontChanged(QFont)), this, SLOT(applySettings()));
    connect(ui_->fixedFontBox, SIGNAL(currentFontChanged(QFont)), this, SLOT(applySettings()));
    connect(ui_->scrollbackFontBox, SIGNAL(currentFontChanged(QFont)), this, SLOT(applySettings()));
    connect(ui_->mainFontSizeSpinBox, SIGNAL(valueChanged(int)), this, SLOT(applySettings()));
    connect(ui_->fixedFontSizeSpinBox, SIGNAL(valueChanged(int)), this, SLOT(applySettings()));
    connect(ui_->scrollbackFontSizeSpinBox, SIGNAL(valueChanged(int)), this, SLOT(applySettings()));
    connect(ui_->softScrollCheckBox, SIGNAL(toggled(bool)), this, SLOT(applySettings()));
    connect(ui_->smartFormattingCheckBox, SIGNAL(toggled(bool)), this, SLOT(applySettings()));
    connect(ui_->marginSizeSpinBox, SIGNAL(valueChanged(int)), this, SLOT(applySettings()));
    connect(ui_->fullscreenWidthSpinBox, SIGNAL(valueChanged(int)), this, SLOT(applySettings()));
    connect(ui_->scriptWrapSpinBox, SIGNAL(valueChanged(int)), this, SLOT(applySettings()));
    connect(ui_->allowGraphicsCheckBox, SIGNAL(toggled(bool)), this, SLOT(applySettings()));
    connect(ui_->allowVideoCheckBox, SIGNAL(toggled(bool)), this, SLOT(applySettings()));
    connect(ui_->allowSoundEffectsCheckBox, SIGNAL(toggled(bool)), this, SLOT(applySettings()));
    connect(ui_->allowMusicCheckBox, SIGNAL(toggled(bool)), this, SLOT(applySettings()));
    connect(ui_->smoothScalingCheckBox, SIGNAL(toggled(bool)), this, SLOT(applySettings()));
    connect(ui_->muteWhenMinimizedCheckBox, SIGNAL(toggled(bool)), this, SLOT(applySettings()));
    connect(ui_->volumeSlider, SIGNAL(valueChanged(int)), SLOT(applySettings()));
    connect(ui_->soundFontGroupBox, &QGroupBox::toggled, this, &ConfDialog::applySettings);
    connect(ui_->soundFontLineEdit, &QLineEdit::textChanged, this, &ConfDialog::applySettings);
    connect(ui_->gainSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            &ConfDialog::applySettings);
    connect(ui_->overlayScrollbackCheckBox, SIGNAL(toggled(bool)), this, SLOT(applySettings()));
    connect(ui_->mainTextColorButton, SIGNAL(changed(QColor)), this, SLOT(applySettings()));
    connect(ui_->mainBgColorButton, SIGNAL(changed(QColor)), this, SLOT(applySettings()));
    connect(ui_->bannerTextColorButton, SIGNAL(changed(QColor)), this, SLOT(applySettings()));
    connect(ui_->bannerBgColorButton, SIGNAL(changed(QColor)), this, SLOT(applySettings()));
    connect(ui_->customFsMarginColorCheckBox, SIGNAL(toggled(bool)), this, SLOT(applySettings()));
    connect(ui_->fsMarginColorButton, SIGNAL(changed(QColor)), this, SLOT(applySettings()));
    connect(ui_->startInButtonGroup, SIGNAL(buttonClicked(int)), SLOT(applySettings()));
}

void ConfDialog::applySettings()
{
    Settings* sett = hApp->settings();

    sett->enable_graphics = ui_->allowGraphicsCheckBox->isChecked();
#ifndef DISABLE_VIDEO
    if (not sett->video_sys_error) {
        sett->enable_video = ui_->allowVideoCheckBox->isChecked();
    }
#endif
    sett->enable_sound_effects = ui_->allowSoundEffectsCheckBox->isChecked();
    sett->enable_music = ui_->allowMusicCheckBox->isChecked();
    sett->use_smooth_scaling = ui_->smoothScalingCheckBox->isChecked();
    sett->mute_when_minimized = ui_->muteWhenMinimizedCheckBox->isChecked();
    sett->sound_volume = ui_->volumeSlider->value();
    sett->soundfont = ui_->soundFontLineEdit->text();
    sett->use_custom_soundfont =
        ui_->soundFontGroupBox->isChecked() and not sett->soundfont.isEmpty();
    sett->synth_gain = ui_->gainSpinBox->value();
    sett->main_bg_color = ui_->mainBgColorButton->color();
    sett->main_text_color = ui_->mainTextColorButton->color();
    sett->status_bg_color = ui_->bannerBgColorButton->color();
    sett->status_text_color = ui_->bannerTextColorButton->color();
    sett->custom_fs_margin_color = ui_->customFsMarginColorCheckBox->isChecked();
    sett->fs_margin_color = ui_->fsMarginColorButton->color();
    sett->prop_font = ui_->mainFontBox->currentFont();
    sett->fixed_font = ui_->fixedFontBox->currentFont();
    sett->scrollback_font = ui_->scrollbackFontBox->currentFont();
    sett->prop_font.setPointSize(ui_->mainFontSizeSpinBox->value());
    sett->fixed_font.setPointSize(ui_->fixedFontSizeSpinBox->value());
    sett->scrollback_font.setPointSize(ui_->scrollbackFontSizeSpinBox->value());
    sett->soft_text_scrolling = ui_->softScrollCheckBox->isChecked();
    sett->smart_formatting = ui_->smartFormattingCheckBox->isChecked();
    sett->overlay_scrollback = ui_->overlayScrollbackCheckBox->isChecked();
    sett->margin_size = ui_->marginSizeSpinBox->value();
    sett->fullscreen_width = ui_->fullscreenWidthSpinBox->value();
    if (ui_->scriptWrapSpinBox->value() <= ui_->scriptWrapSpinBox->minimum()) {
        sett->script_wrap = 0;
    } else {
        sett->script_wrap = ui_->scriptWrapSpinBox->value();
    }
    sett->start_fullscreen = ui_->fullscreenRadioButton->isChecked();
    sett->start_windowed = ui_->windowRadioButton->isChecked();

    // Notify the application that preferences have changed.
    hApp->notifyPreferencesChange(sett);

    sett->saveToDisk();

    initial_sound_vol_ = sett->sound_volume;
    initial_gain_ = sett->synth_gain;
}

void ConfDialog::cancel()
{
    hApp->settings()->sound_volume = initial_sound_vol_;
    hApp->settings()->synth_gain = initial_gain_;
    updateMusicVolume();
    updateSoundVolume();
    updateVideoVolume();
    updateSynthGain();
}

void ConfDialog::setSoundVolume(int vol)
{
    hApp->settings()->sound_volume = vol;
    updateMusicVolume();
    updateSoundVolume();
    updateVideoVolume();
#ifndef DISABLE_AUDIO
    if (midi_stream_ != nullptr) {
        midi_stream_->setVolume(std::pow(vol / 100.f, 2.f));
    }
#endif
}

void ConfDialog::playTestMidi()
{
#ifndef DISABLE_AUDIO
    if (midi_stream_ != nullptr and midi_stream_->isPlaying()) {
        return;
    }
    // Re-create the stream each time because the selected soundfont might have changed.
    midi_stream_ = nullptr;
    QResource midiRes(":/test.mid");
    auto fsdec = std::make_unique<Aulib::AudioDecoderFluidSynth>();
    if (not ui_->soundFontGroupBox->isChecked() or ui_->soundFontLineEdit->text().isEmpty()) {
        QResource sf2Res(":/soundfont.sf2");
        fsdec->loadSoundfont(SDL_RWFromConstMem(sf2Res.data(), sf2Res.size()));
    } else {
        fsdec->loadSoundfont(ui_->soundFontLineEdit->text().toStdString());
    }
    fsdec->setGain(ui_->gainSpinBox->value());
    fsynth_dec_ = fsdec.get();
    auto resampler = std::make_unique<Aulib::AudioResamplerSpeex>();
    auto* rwops = SDL_RWFromConstMem(midiRes.data(), midiRes.size());
    midi_stream_ =
        std::make_unique<Aulib::AudioStream>(rwops, std::move(fsdec), std::move(resampler), true);
    midi_stream_->setVolume(std::pow(ui_->volumeSlider->value() / 100.f, 2.f));
    midi_stream_->play(1, 1.5f);
#endif
}

void ConfDialog::stopTestMidi()
{
#ifndef DISABLE_AUDIO
    if (midi_stream_) {
        midi_stream_->stop(0.5f);
    }
#endif
}

void ConfDialog::setGain()
{
#ifndef DISABLE_AUDIO
    if (midi_stream_) {
        fsynth_dec_->setGain(ui_->gainSpinBox->value());
    }
    hApp->settings()->synth_gain = ui_->gainSpinBox->value();
    ::updateSynthGain();
#endif
}

void ConfDialog::browseForSoundFont()
{
    auto file = QFileDialog::getOpenFileName(this, tr("Set SoundFont"),
                                             QFileInfo(ui_->soundFontLineEdit->text()).path(),
                                             "SoundFonts (*.sf2 *.sf3);;All Files (*)");
    if (not file.isEmpty()) {
        ui_->soundFontLineEdit->setText(file);
    }
}

/* Copyright (C) 2011-2018 Nikos Chantziaras
 *
 * This file is part of Hugor.
 *
 * Hugor is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * Hugor is distributed in the hope that it will be useful, but WITHOUT ANY
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
 * parts covered by the terms of the Hugo License, the licensors of this
 * Program grant you additional permission to convey the resulting work.
 * Corresponding Source for a non-source form of such a combination shall
 * include the source code for the parts of the Hugo Engine used as well as
 * that of the covered work.
 */
