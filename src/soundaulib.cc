// This is copyrighted software. More information is at the end of this file.
#include "hugodefs.h"
#include "hugohandlers.h"

#include <QDebug>
#include <QFile>
#include <QResource>
#include <SDL.h>
#include <SDL_audio.h>
#include <algorithm>
#include <array>
#include <cmath>

#include "Aulib/DecoderAdlmidi.h"
#include "Aulib/DecoderFluidsynth.h"
#include "Aulib/DecoderModplug.h"
#include "Aulib/DecoderMpg123.h"
#include "Aulib/DecoderOpenmpt.h"
#include "Aulib/DecoderSndfile.h"
#include "Aulib/DecoderXmp.h"
#include "Aulib/ResamplerSpeex.h"
#include "Aulib/Stream.h"
#include "Aulib/Processor.h"
#include "aulib.h"
#include "happlication.h"
extern "C" {
#include "heheader.h"
}
#include "hugorfile.h"
#include "oplvolumebooster.h"
#include "rwopsbundle.h"
#include "settings.h"
#include "synthfactory.h"

// Current music and sample volumes. Needed to restore the volumes after muting them.
static int currentMusicVol = 100;
static int currentSampleVol = 100;

// We only play one music and one sample track at a time, so it's enough to make these static.
static std::unique_ptr<Aulib::Stream>& musicStream()
{
    static auto p = std::unique_ptr<Aulib::Stream>();
    return p;
}

static std::unique_ptr<Aulib::Stream>& sampleStream()
{
    static auto p = std::unique_ptr<Aulib::Stream>();
    return p;
}

// We hold a pointer to the fluidsynth decoder so we can change the gain while the stream is
// playing.
static Aulib::DecoderFluidsynth*& fsynthDec()
{
    static Aulib::DecoderFluidsynth* p = nullptr;
    return p;
}

static float convertHugoVolume(int hugoVol)
{
    if (hugoVol < 0) {
        hugoVol = 0;
    } else if (hugoVol > 100) {
        hugoVol = 100;
    }
    return (hugoVol / 100.f) * std::pow((float)hApp->settings()->sound_volume / 100.f, 2.f);
}

void initSoundEngine()
{
    // Initialize only the audio part of SDL.
    if (SDL_Init(SDL_INIT_AUDIO) != 0) {
        qWarning("Unable to initialize sound system: %s", SDL_GetError());
        exit(1);
    }

    // Initialize audio with 44.1kHz, 16 bit, 2 channels (stereo) and a 4k chunk size.
    // TODO: Make this configurable?
    if (not Aulib::init(44100, AUDIO_S16SYS, 2, 4096)) {
        qWarning("Unable to initialize audio: %s", SDL_GetError());
        exit(1);
    }
}

void closeSoundEngine()
{
    if (musicStream()) {
        musicStream()->stop();
        musicStream().reset();
    }
    if (sampleStream()) {
        sampleStream()->stop();
        sampleStream().reset();
    }
    Aulib::quit();
    SDL_Quit();
}

void muteSound(bool mute)
{
    if (musicStream()) {
        if (mute) {
            musicStream()->mute();
        } else {
            musicStream()->unmute();
        }
    }
    if (sampleStream()) {
        if (mute) {
            sampleStream()->mute();
        } else {
            sampleStream()->unmute();
        }
    }
}

void updateMusicVolume()
{
    hHandlers->musicvolume(currentMusicVol);
}

void updateSoundVolume()
{
    hHandlers->samplevolume(currentSampleVol);
}

void updateSynthGain()
{
    if (fsynthDec() != nullptr) {
        fsynthDec()->setGain(hApp->settings()->synth_gain);
    }
}

bool isMusicPlaying()
{
    return musicStream() and musicStream()->isPlaying();
}

bool isSamplePlaying()
{
    return sampleStream() and sampleStream()->isPlaying();
}

static bool playStream(HUGO_FILE infile, long reslength, char loop_flag, bool isMusic)
{
    if ((isMusic and not hApp->settings()->enable_music)
        or (not isMusic and not hApp->settings()->enable_sound_effects)) {
        return false;
    }

    auto& stream = isMusic ? musicStream() : sampleStream();
    std::unique_ptr<Aulib::Decoder> decoder;
    std::shared_ptr<OplVolumeBooster> processor;

    if (stream) {
        stream->stop();
        stream.reset();
    }

    // Create an RWops for the embedded media resource.
    SDL_RWops* rwops = RWFromMediaBundle(infile->get(), reslength);
    if (rwops == nullptr) {
        qWarning() << "ERROR:" << SDL_GetError();
        return false;
    }
    infile->release();

    if (not isMusic) {
        decoder = std::make_unique<Aulib::DecoderSndfile>();
    } else {
        switch (resource_type) {
        case MIDI_R: {
#if USE_DEC_ADLMIDI
            if (hApp->settings()->use_adlmidi) {
                decoder = makeAdlmidiDec();
                fsynthDec() = nullptr;
                processor = std::make_shared<OplVolumeBooster>();
            } else {
#endif
                const auto& soundfont = hApp->settings()->use_custom_soundfont
                                            ? hApp->settings()->soundfont
                                            : QString();
                auto dec = makeFluidsynthDec(soundfont, hApp->settings()->synth_gain);
                fsynthDec() = dec.get();
                decoder = std::move(dec);
#if USE_DEC_ADLMIDI
            }
#endif
            break;
        }
        case XM_R:
        case S3M_R:
        case MOD_R: {
            using ModDec_type =
#if USE_DEC_OPENMPT
                Aulib::DecoderOpenmpt;
#elif USE_DEC_XMP
                Aulib::DecoderXmp;
#elif USE_DEC_MODPLUG
                Aulib::DecoderModPlug;
#endif
            decoder = std::make_unique<ModDec_type>();
            fsynthDec() = nullptr;
            break;
        }
        case MP3_R: {
            // A bug in the Hugo base code allows WAV files to be played in the music channel. The
            // engine passes them as MP3_R. "Future Boy" is a known game that depends on this bug.
            std::array<char, 5> head{};
            if (SDL_RWread(rwops, head.data(), 1, 4) == 4 and head == decltype(head){"RIFF"}) {
                decoder = std::make_unique<Aulib::DecoderSndfile>();
            } else {
                decoder = std::make_unique<Aulib::DecoderMpg123>();
            }
            SDL_RWseek(rwops, 0, RW_SEEK_SET);
            fsynthDec() = nullptr;
            break;
        }
        default:
            qWarning() << "ERROR: Unknown music resource type";
            return false;
        }
    }

    stream = std::make_unique<Aulib::Stream>(rwops, std::move(decoder),
                                             std::make_unique<Aulib::ResamplerSpeex>(), true);
    stream->addProcessor(std::move(processor));
    if (stream->open()) {
        // Start playing the stream. Loop forever if 'loop_flag' is true. Otherwise, just play it
        // once.
        if (isMusic) {
            updateMusicVolume();
        } else {
            updateSoundVolume();
        }
        if (stream->play(loop_flag ? 0 : 1)) {
            return true;
        }
    }

    qWarning() << "ERROR:" << SDL_GetError();
    stream.reset();
    fsynthDec() = nullptr;
    return false;
}

void HugoHandlers::playmusic(HUGO_FILE infile, long reslength, char loop_flag, int* result) const
{
    *result = playStream(infile, reslength, loop_flag, true);
}

void HugoHandlers::musicvolume(int vol) const
{
    if (vol < 0) {
        vol = 0;
    } else if (vol > 100) {
        vol = 100;
    }
    currentMusicVol = vol;

    // Convert the Hugo volume range [0..100] to [0..1] and attenuate the result by the global
    // volume, using an exponential (power of 2) scale.
    if (musicStream()) {
        musicStream()->setVolume(convertHugoVolume(vol));
    }
}

void HugoHandlers::stopmusic() const
{
    if (musicStream()) {
        musicStream()->stop();
    }
}

void HugoHandlers::playsample(HUGO_FILE infile, long reslength, char loop_flag, int* result) const
{
    *result = playStream(infile, reslength, loop_flag, false);
}

void HugoHandlers::samplevolume(int vol) const
{
    if (vol < 0) {
        vol = 0;
    } else if (vol > 100) {
        vol = 100;
    }
    currentSampleVol = vol;
    if (sampleStream()) {
        sampleStream()->setVolume(convertHugoVolume(vol));
    }
}

void HugoHandlers::stopsample() const
{
    if (sampleStream()) {
        sampleStream()->stop();
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
