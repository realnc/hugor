// This is copyrighted software. More information is at the end of this file.
#include "hugodefs.h"
#include "hugohandlers.h"

#include <QDebug>
#include <QFile>
#include <QResource>
#include <SDL.h>
#include <SDL_audio.h>
#include <cmath>

#include "Aulib/AudioDecoderFluidsynth.h"
#include "Aulib/AudioDecoderMpg123.h"
#include "Aulib/AudioDecoderOpenmpt.h"
#include "Aulib/AudioDecoderSndfile.h"
#include "Aulib/AudioResamplerSpeex.h"
#include "Aulib/AudioStream.h"
#include "aulib.h"
#include "happlication.h"
extern "C" {
#include "heheader.h"
}
#include "hugorfile.h"
#include "rwopsbundle.h"
#include "settings.h"

// Current music and sample volumes. Needed to restore the volumes after muting them.
static int currentMusicVol = 100;
static int currentSampleVol = 100;
static bool isMuted = false;

// We only play one music and one sample track at a time, so it's enough to make these static.
static std::unique_ptr<Aulib::AudioStream>& musicStream()
{
    static auto p = std::unique_ptr<Aulib::AudioStream>();
    return p;
}

static std::unique_ptr<Aulib::AudioStream>& sampleStream()
{
    static auto p = std::unique_ptr<Aulib::AudioStream>();
    return p;
}

static Aulib::AudioDecoderFluidSynth*& fsynthDec()
{
    static Aulib::AudioDecoderFluidSynth* p = nullptr;
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
    if (Aulib::init(44100, AUDIO_S16SYS, 2, 4096) != 0) {
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
    if (mute and not isMuted) {
        if (musicStream()) {
            musicStream()->setVolume(0);
        }
        if (sampleStream()) {
            sampleStream()->setVolume(0);
        }
        isMuted = true;
    } else if (not mute and isMuted) {
        if (musicStream()) {
            musicStream()->setVolume(convertHugoVolume(currentMusicVol));
        }
        if (sampleStream()) {
            sampleStream()->setVolume(convertHugoVolume(currentSampleVol));
        }
        isMuted = false;
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
        delete infile;
        return false;
    }

    auto& stream = isMusic ? musicStream() : sampleStream();
    std::unique_ptr<Aulib::AudioDecoder> decoder;

    if (stream) {
        stream->stop();
        stream.reset();
    }

    // Create an RWops for the embedded media resource.
    SDL_RWops* rwops = RWFromMediaBundle(infile->get(), reslength);
    if (rwops == nullptr) {
        qWarning() << "ERROR:" << SDL_GetError();
        delete infile;
        return false;
    }

    if (not isMusic) {
        decoder = std::make_unique<Aulib::AudioDecoderSndfile>();
    } else {
        switch (resource_type) {
        case MIDI_R: {
            auto fsdec = std::make_unique<Aulib::AudioDecoderFluidSynth>();
            if (hApp->settings()->soundfont.isEmpty()
                or not hApp->settings()->use_custom_soundfont) {
                QResource sf2Res(":/soundfont.sf2");
                auto* sf2_rwops = SDL_RWFromConstMem(sf2Res.data(), sf2Res.size());
                fsdec->loadSoundfont(sf2_rwops);
            } else {
                fsdec->loadSoundfont(hApp->settings()->soundfont.toStdString());
            }
            fsdec->setGain(hApp->settings()->synth_gain);
            fsynthDec() = fsdec.get();
            decoder = std::move(fsdec);
            break;
        }
        case XM_R:
        case S3M_R:
        case MOD_R:
            decoder = std::make_unique<Aulib::AudioDecoderOpenmpt>();
            fsynthDec() = nullptr;
            break;
        case MP3_R:
            decoder = std::make_unique<Aulib::AudioDecoderMpg123>();
            fsynthDec() = nullptr;
            break;
        default:
            qWarning() << "ERROR: Unknown music resource type";
            return false;
        }
    }

    stream = std::make_unique<Aulib::AudioStream>(
        rwops, std::move(decoder), std::make_unique<Aulib::AudioResamplerSpeex>(), true);
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
    if (musicStream() and not isMuted) {
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
    if (sampleStream() and not isMuted) {
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
