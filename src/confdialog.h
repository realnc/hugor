// This is copyrighted software. More information is at the end of this file.
#pragma once
#include <QDialog>

#include <memory>

namespace Ui {
class ConfDialog;
}

namespace Aulib {
class Stream;
class DecoderFluidsynth;
class DecoderAdlmidi;
} // namespace Aulib

class HMainWindow;

class ConfDialog final: public QDialog
{
    Q_OBJECT

public:
    ConfDialog(HMainWindow* parent = nullptr);
    ~ConfDialog() override;

protected:
    void changeEvent(QEvent* e) override;

private:
    std::unique_ptr<Ui::ConfDialog> ui_;
    int initial_sound_vol_;
    float initial_gain_;
#ifndef DISABLE_AUDIO
    std::unique_ptr<Aulib::Stream> midi_stream_;
    Aulib::DecoderFluidsynth* fsynth_dec_ = nullptr;
#endif

    // Makes the dialog's controls apply instantly when they change.
    void makeInstantApply();

private slots:
    void applySettings();
    void cancel();
    void setSoundVolume(int vol);
    void playTestMidi();
    void stopTestMidi();
    void setGain();
    void browseForSoundFont();
};

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
