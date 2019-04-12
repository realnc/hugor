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
#include "oplvolumebooster.h"
#include "synthfactory.h"
#include <Aulib/DecoderAdlmidi.h>
#include <Aulib/DecoderFluidsynth.h>
#include <Aulib/ResamplerSpeex.h>
#include <Aulib/Stream.h>
#include <SDL_rwops.h>
#endif
#include <cmath>

#include "happlication.h"
#include "hframe.h"
#include "hmainwindow.h"
#include "hugodefs.h"
#include "settings.h"
#include "ui_confdialog.h"
#include "util.h"

#ifndef DISABLE_AUDIO
using namespace std::chrono_literals;
#endif

ConfDialog::ConfDialog(HMainWindow* parent)
    : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint)
    , ui_(std::make_unique<Ui::ConfDialog>())
{
    setWindowModality(Qt::ApplicationModal);
    ui_->setupUi(this);
    hApp->settings().loadFromDisk();
    const Settings& sett = hApp->settings();

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
    initial_sound_vol_ = sett.sound_volume;
    initial_gain_ = sett.synth_gain;

#ifdef DISABLE_AUDIO
    ui_->volumeSlider->setEnabled(false);
    ui_->soundFontGroupBox->setEnabled(false);
    ui_->midiTestLabel->setEnabled(false);
    ui_->midiPlayButton->setEnabled(false);
    ui_->midiStopButton->setEnabled(false);
    ui_->gainLabel->setEnabled(false);
    ui_->gainSpinBox->setEnabled(false);
    ui_->midiGroupBox->setEnabled(false);
#elif not USE_DEC_ADLMIDI
    ui_->midiGroupBox->setEnabled(false);
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

    ui_->allowGraphicsCheckBox->setChecked(sett.enable_graphics);
#ifdef DISABLE_VIDEO
    ui_->allowVideoCheckBox->setChecked(false);
    ui_->allowVideoCheckBox->setDisabled(true);
#else
    if (sett.video_sys_error) {
        ui_->allowVideoCheckBox->setChecked(false);
        ui_->allowVideoCheckBox->setDisabled(true);
    } else {
        ui_->allowVideoCheckBox->setChecked(sett.enable_video);
    }
#endif
#ifdef DISABLE_AUDIO
    ui_->allowSoundEffectsCheckBox->setDisabled(true);
    ui_->allowMusicCheckBox->setDisabled(true);
    ui_->muteWhenMinimizedCheckBox->setDisabled(true);
#else
    ui_->allowSoundEffectsCheckBox->setChecked(sett.enable_sound_effects);
    ui_->allowMusicCheckBox->setChecked(sett.enable_music);
    ui_->muteWhenMinimizedCheckBox->setChecked(sett.mute_when_minimized);
#endif
#if defined(DISABLE_VIDEO) and defined(DISABLE_AUDIO)
    ui_->volumeLabel->setDisabled(true);
    ui_->volumeSlider->setValue(0);
    ui_->volumeSlider->setDisabled(true);
#else
    ui_->volumeSlider->setValue(sett.sound_volume);
    connect(ui_->volumeSlider, &QSlider::valueChanged, this, &ConfDialog::setSoundVolume);
#endif
    ui_->soundFontGroupBox->setChecked(sett.use_custom_soundfont);
    ui_->soundFontLineEdit->setText(sett.soundfont);
    ui_->gainSpinBox->setValue(sett.synth_gain);
#if USE_DEC_ADLMIDI
    connect(ui_->adlibRadioButton, &QRadioButton::toggled, [this](bool checked) {
        ui_->soundFontGroupBox->setDisabled(checked);
        ui_->gainLabel->setDisabled(checked);
        ui_->gainSpinBox->setDisabled(checked);
    });
    if (sett.use_adlmidi) {
        ui_->adlibRadioButton->setChecked(true);
    } else {
        ui_->fsynthRadioButton->setChecked(true);
    }
#else
    ui_->fsynthRadioButton->setChecked(true);
#endif

    ui_->mainBgColorButton->setColor(sett.main_bg_color);
    ui_->mainTextColorButton->setColor(sett.main_text_color);
    ui_->bannerBgColorButton->setColor(sett.status_bg_color);
    ui_->bannerTextColorButton->setColor(sett.status_text_color);
    ui_->fsMarginColorButton->setColor(sett.fs_margin_color);
    connect(ui_->customFsMarginColorCheckBox, &QCheckBox::toggled, ui_->fsMarginColorButton,
            &KColorButton::setEnabled);
    ui_->customFsMarginColorCheckBox->setChecked(sett.custom_fs_margin_color);

    ui_->mainFontSizeSpinBox->setValue(sett.prop_font.pointSize());
    ui_->fixedFontSizeSpinBox->setValue(sett.fixed_font.pointSize());
    ui_->scrollbackFontSizeSpinBox->setValue(sett.scrollback_font.pointSize());
    ui_->mainFontBox->setCurrentFont(sett.prop_font);
    ui_->fixedFontBox->setCurrentFont(sett.fixed_font);
    ui_->scrollbackFontBox->setCurrentFont(sett.scrollback_font);
    ui_->softScrollCheckBox->setChecked(sett.soft_text_scrolling);
    ui_->smartFormattingCheckBox->setChecked(sett.smart_formatting);
    ui_->marginSizeSpinBox->setValue(sett.margin_size);
    ui_->overlayScrollbackCheckBox->setChecked(sett.overlay_scrollback);
    ui_->scrollWheelCheckBox->setChecked(sett.scrollback_on_wheel);
    ui_->fullscreenWidthSpinBox->setValue(sett.fullscreen_width);
    if (sett.script_wrap < ui_->scriptWrapSpinBox->minimum()) {
        ui_->scriptWrapSpinBox->setValue(ui_->scriptWrapSpinBox->minimum());
    } else {
        ui_->scriptWrapSpinBox->setValue(sett.script_wrap);
    }
    switch (sett.cursor_shape) {
    case Settings::TextCursorShape::Ibeam:
        ui_->cursorShapeComboBox->setCurrentIndex(0);
        break;
    case Settings::TextCursorShape::Block:
        ui_->cursorShapeComboBox->setCurrentIndex(1);
        break;
    default:
    case Settings::TextCursorShape::Underline:
        ui_->cursorShapeComboBox->setCurrentIndex(2);
        break;
    }
    ui_->cursorThicknessComboBox->setCurrentIndex(sett.cursor_thickness);
    ui_->cursorThicknessComboBox->setDisabled(sett.cursor_shape
                                              == Settings::TextCursorShape::Block);
    if (sett.start_fullscreen) {
        ui_->fullscreenRadioButton->setChecked(true);
    } else if (sett.start_windowed) {
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
        connect(ui_->buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked, this,
                &ConfDialog::applySettings);
        connect(this, &ConfDialog::accepted, this, &ConfDialog::applySettings);
        connect(this, &ConfDialog::rejected, this, &ConfDialog::cancel);
    }
#endif

    connect(ui_->midiPlayButton, &QPushButton::clicked, this, &ConfDialog::playTestMidi);
    connect(ui_->midiStopButton, &QPushButton::clicked, this, &ConfDialog::stopTestMidi);
    connect(ui_->soundFontPushButton, &QPushButton::clicked, this, &ConfDialog::browseForSoundFont);
    connect(ui_->gainSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            &ConfDialog::setGain);
    connect(ui_->cursorShapeComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this](int index) { ui_->cursorThicknessComboBox->setDisabled(index == 1); });
}

ConfDialog::~ConfDialog() = default;

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
    connect(ui_->mainFontBox, &QFontComboBox::currentFontChanged, this, &ConfDialog::applySettings);
    connect(ui_->fixedFontBox, &QFontComboBox::currentFontChanged, this,
            &ConfDialog::applySettings);
    connect(ui_->scrollbackFontBox, &QFontComboBox::currentFontChanged, this,
            &ConfDialog::applySettings);
    connect(ui_->mainFontSizeSpinBox, qOverload<int>(&QSpinBox::valueChanged), this,
            &ConfDialog::applySettings);
    connect(ui_->fixedFontSizeSpinBox, qOverload<int>(&QSpinBox::valueChanged), this,
            &ConfDialog::applySettings);
    connect(ui_->scrollbackFontSizeSpinBox, qOverload<int>(&QSpinBox::valueChanged), this,
            &ConfDialog::applySettings);
    connect(ui_->softScrollCheckBox, &QCheckBox::toggled, this, &ConfDialog::applySettings);
    connect(ui_->smartFormattingCheckBox, &QCheckBox::toggled, this, &ConfDialog::applySettings);
    connect(ui_->marginSizeSpinBox, qOverload<int>(&QSpinBox::valueChanged), this,
            &ConfDialog::applySettings);
    connect(ui_->fullscreenWidthSpinBox, qOverload<int>(&QSpinBox::valueChanged), this,
            &ConfDialog::applySettings);
    connect(ui_->scriptWrapSpinBox, qOverload<int>(&QSpinBox::valueChanged), this,
            &ConfDialog::applySettings);
    connect(ui_->cursorShapeComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this,
            &ConfDialog::applySettings);
    connect(ui_->cursorThicknessComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this,
            &ConfDialog::applySettings);
    connect(ui_->allowGraphicsCheckBox, &QCheckBox::toggled, this, &ConfDialog::applySettings);
    connect(ui_->allowVideoCheckBox, &QCheckBox::toggled, this, &ConfDialog::applySettings);
    connect(ui_->allowSoundEffectsCheckBox, &QCheckBox::toggled, this, &ConfDialog::applySettings);
    connect(ui_->allowMusicCheckBox, &QCheckBox::toggled, this, &ConfDialog::applySettings);
    connect(ui_->muteWhenMinimizedCheckBox, &QCheckBox::toggled, this, &ConfDialog::applySettings);
    connect(ui_->volumeSlider, &QSlider::valueChanged, this, &ConfDialog::applySettings);
    connect(ui_->soundFontGroupBox, &QGroupBox::toggled, this, &ConfDialog::applySettings);
    connect(ui_->soundFontLineEdit, &QLineEdit::textChanged, this, &ConfDialog::applySettings);
    connect(ui_->gainSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            &ConfDialog::applySettings);
    connect(ui_->adlibRadioButton, &QRadioButton::toggled, this, &ConfDialog::applySettings);
    connect(ui_->overlayScrollbackCheckBox, &QCheckBox::toggled, this, &ConfDialog::applySettings);
    connect(ui_->scrollWheelCheckBox, &QCheckBox::toggled, this, &ConfDialog::applySettings);
    connect(ui_->mainTextColorButton, &KColorButton::changed, this, &ConfDialog::applySettings);
    connect(ui_->mainBgColorButton, &KColorButton::changed, this, &ConfDialog::applySettings);
    connect(ui_->bannerTextColorButton, &KColorButton::changed, this, &ConfDialog::applySettings);
    connect(ui_->bannerBgColorButton, &KColorButton::changed, this, &ConfDialog::applySettings);
    connect(ui_->customFsMarginColorCheckBox, &QCheckBox::toggled, this,
            &ConfDialog::applySettings);
    connect(ui_->fsMarginColorButton, &KColorButton::changed, this, &ConfDialog::applySettings);
    connect(ui_->startInButtonGroup, qOverload<int>(&QButtonGroup::buttonClicked), this,
            &ConfDialog::applySettings);
}

void ConfDialog::applySettings()
{
    Settings& sett = hApp->settings();

    sett.enable_graphics = ui_->allowGraphicsCheckBox->isChecked();
#ifndef DISABLE_VIDEO
    if (not sett.video_sys_error) {
        sett.enable_video = ui_->allowVideoCheckBox->isChecked();
    }
#endif
    sett.enable_sound_effects = ui_->allowSoundEffectsCheckBox->isChecked();
    sett.enable_music = ui_->allowMusicCheckBox->isChecked();
    sett.mute_when_minimized = ui_->muteWhenMinimizedCheckBox->isChecked();
    sett.sound_volume = ui_->volumeSlider->value();
    sett.soundfont = ui_->soundFontLineEdit->text();
    sett.use_custom_soundfont =
        ui_->soundFontGroupBox->isChecked() and not sett.soundfont.isEmpty();
    sett.synth_gain = ui_->gainSpinBox->value();
    sett.use_adlmidi = ui_->adlibRadioButton->isChecked();
    sett.main_bg_color = ui_->mainBgColorButton->color();
    sett.main_text_color = ui_->mainTextColorButton->color();
    sett.status_bg_color = ui_->bannerBgColorButton->color();
    sett.status_text_color = ui_->bannerTextColorButton->color();
    sett.custom_fs_margin_color = ui_->customFsMarginColorCheckBox->isChecked();
    sett.fs_margin_color = ui_->fsMarginColorButton->color();
    sett.prop_font = ui_->mainFontBox->currentFont();
    sett.prop_font.setKerning(false);
    sett.fixed_font = ui_->fixedFontBox->currentFont();
    sett.scrollback_font = ui_->scrollbackFontBox->currentFont();
    sett.prop_font.setPointSize(ui_->mainFontSizeSpinBox->value());
    sett.fixed_font.setPointSize(ui_->fixedFontSizeSpinBox->value());
    sett.scrollback_font.setPointSize(ui_->scrollbackFontSizeSpinBox->value());
    sett.soft_text_scrolling = ui_->softScrollCheckBox->isChecked();
    sett.smart_formatting = ui_->smartFormattingCheckBox->isChecked();
    sett.overlay_scrollback = ui_->overlayScrollbackCheckBox->isChecked();
    sett.scrollback_on_wheel = ui_->scrollWheelCheckBox->isChecked();
    sett.margin_size = ui_->marginSizeSpinBox->value();
    sett.fullscreen_width = ui_->fullscreenWidthSpinBox->value();
    if (ui_->scriptWrapSpinBox->value() <= ui_->scriptWrapSpinBox->minimum()) {
        sett.script_wrap = 0;
    } else {
        sett.script_wrap = ui_->scriptWrapSpinBox->value();
    }
    switch (ui_->cursorShapeComboBox->currentIndex()) {
    case 0:
        sett.cursor_shape = Settings::TextCursorShape::Ibeam;
        break;
    case 1:
        sett.cursor_shape = Settings::TextCursorShape::Block;
        break;
    case 2:
        sett.cursor_shape = Settings::TextCursorShape::Underline;
        break;
    }
    sett.cursor_thickness = ui_->cursorThicknessComboBox->currentIndex();
    sett.start_fullscreen = ui_->fullscreenRadioButton->isChecked();
    sett.start_windowed = ui_->windowRadioButton->isChecked();

    // Notify the application that preferences have changed.
    hApp->notifyPreferencesChange(sett);

    sett.saveToDisk();

    initial_sound_vol_ = sett.sound_volume;
    initial_gain_ = sett.synth_gain;
}

void ConfDialog::cancel()
{
    hApp->settings().sound_volume = initial_sound_vol_;
    hApp->settings().synth_gain = initial_gain_;
    updateMusicVolume();
    updateSoundVolume();
    updateVideoVolume();
    updateSynthGain();
}

void ConfDialog::setSoundVolume(int vol)
{
    hApp->settings().sound_volume = vol;
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
    std::unique_ptr<Aulib::Decoder> decoder;
    std::shared_ptr<OplVolumeBooster> processor;
#if USE_DEC_ADLMIDI
    if (ui_->adlibRadioButton->isChecked()) {
        fsynth_dec_ = nullptr;
        decoder = makeAdlmidiDec();
        processor = std::make_shared<OplVolumeBooster>();
    } else {
#endif
        QString soundfont;
        if (ui_->soundFontGroupBox->isChecked() and not ui_->soundFontLineEdit->text().isEmpty()) {
            soundfont = ui_->soundFontLineEdit->text();
        }
        auto fsdec = makeFluidsynthDec(soundfont, ui_->gainSpinBox->value());
        fsynth_dec_ = fsdec.get();
        decoder = std::move(fsdec);
#if USE_DEC_ADLMIDI
    }
#endif
    auto resampler = std::make_unique<Aulib::ResamplerSpeex>();
    auto* rwops = SDL_RWFromConstMem(midiRes.data(), midiRes.size());
    midi_stream_ =
        std::make_unique<Aulib::Stream>(rwops, std::move(decoder), std::move(resampler), true);
    midi_stream_->addProcessor(std::move(processor));
    midi_stream_->setVolume(std::pow(ui_->volumeSlider->value() / 100.f, 2.f));
    midi_stream_->play(1, 1500ms);
#endif
}

void ConfDialog::stopTestMidi()
{
#ifndef DISABLE_AUDIO
    if (midi_stream_) {
        midi_stream_->stop(500ms);
    }
#endif
}

void ConfDialog::setGain()
{
#ifndef DISABLE_AUDIO
    if (midi_stream_ and fsynth_dec_ != nullptr) {
        fsynth_dec_->setGain(ui_->gainSpinBox->value());
    }
    hApp->settings().synth_gain = ui_->gainSpinBox->value();
    ::updateSynthGain();
#endif
}

void ConfDialog::browseForSoundFont()
{
    hFrame->setPreventAutoMinimize(true);
    auto file = QFileDialog::getOpenFileName(this, tr("Set SoundFont"),
                                             QFileInfo(ui_->soundFontLineEdit->text()).path(),
                                             "SoundFonts (*.sf2 *.sf3);;All Files (*)");
    hFrame->setPreventAutoMinimize(false);
    if (not file.isEmpty()) {
        ui_->soundFontLineEdit->setText(file);
    }
}

/* Copyright (C) 2011-2019 Nikos Chantziaras
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
 */
