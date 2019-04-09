// This is copyrighted software. More information is at the end of this file.
#include "synthfactory.h"

#include "Aulib/DecoderAdlmidi.h"
#include "Aulib/DecoderFluidsynth.h"
#include <QByteArray>
#include <QResource>
#include <SDL_rwops.h>

std::unique_ptr<Aulib::DecoderFluidsynth> makeFluidsynthDec(const QString& soundfont,
                                                            const float gain)
{
    auto decoder = std::make_unique<Aulib::DecoderFluidsynth>();

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
std::unique_ptr<Aulib::DecoderAdlmidi> makeAdlmidiDec()
{
    auto decoder = std::make_unique<Aulib::DecoderAdlmidi>();
    QResource res(":/genmidi_gs.wopl");
    const QByteArray data = res.isCompressed()
                                ? qUncompress(res.data(), res.size())
                                : QByteArray::fromRawData((const char*)res.data(), res.size());

    decoder->setEmulator(Aulib::DecoderAdlmidi::Emulator::Dosbox);
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
 */
