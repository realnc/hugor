// This is copyrighted software. More information is at the end of this file.
#include "synthfactory.h"

#include "Aulib/AudioDecoderAdlmidi.h"
#include "Aulib/AudioDecoderFluidsynth.h"
#include <QByteArray>
#include <QResource>
#include <SDL_rwops.h>

std::unique_ptr<Aulib::AudioDecoderFluidSynth> makeFluidsynthDec(const QString& soundfont,
                                                                 const float gain)
{
    auto decoder = std::make_unique<Aulib::AudioDecoderFluidSynth>();

    if (soundfont.isNull()) {
        QResource res(QLatin1String(":/soundfont.sf2"));
        auto* rwops = SDL_RWFromConstMem(res.data(), res.size());
        decoder->loadSoundfont(rwops);
    } else {
        decoder->loadSoundfont(soundfont.toStdString());
    }
    decoder->setGain(gain);
    return decoder;
}

#if USE_DEC_ADLMIDI
std::unique_ptr<Aulib::AudioDecoderAdlmidi> makeAdlmidiDec()
{
    auto decoder = std::make_unique<Aulib::AudioDecoderAdlmidi>();
    QResource res(":/genmidi_gs.wopl");
    const QByteArray data = res.isCompressed()
                                ? qUncompress(res.data(), res.size())
                                : QByteArray::fromRawData((const char*)res.data(), res.size());

    decoder->setEmulator(Aulib::AudioDecoderAdlmidi::Emulator::Dosbox);
    decoder->setChipAmount(6);
    decoder->loadBank(SDL_RWFromConstMem(data.constData(), data.size()));
    return decoder;
}
#endif

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
