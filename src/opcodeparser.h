// This is copyrighted software. More information is at the end of this file.
#pragma once
#include <QObject>
#include <cstdint>
#include <queue>

class OpcodeParser final
{
    Q_GADGET
public:
    void pushByte(int c);

    bool hasOutput() const
    {
        return not output_.empty();
    }

    int getNextOutputByte();
    void parse();

    enum class Opcode : std::int16_t
    {
        IS_OPCODE_AVAILABLE = 1,
        GET_VERSION = 100,
        GET_OS = 200,
        ABORT = 300,
        FADE_SCREEN = 400,
        OPEN_URL = 500,
        SET_FULLSCREEN = 600,
        SET_CLIPBOARD = 700,
        IS_MUSIC_PLAYING = 800,
        IS_SAMPLE_PLAYING = 900,
        IS_FLUID_LAYOUT = 1000,
        SET_COLOR = 1100,
        IS_FULLSCREEN = 1200,
        HIDES_CURSOR = 1300,
        TOP_JUSTIFIED = 1400,
        SCREENREADER_CAPABLE = 1500,
        CHECK_RESOURCE = 1600,
    };

    enum class OpcodeResult : std::int16_t
    {
        OK = 0,
        WRONG_PARAM_COUNT = 10,
        WRONG_BYTE_COUNT = 20,
        UNKNOWN_OPCODE = 30,
    };

    Q_ENUM(Opcode)

private:
    // Bytes to be parsed.
    std::queue<int> buffer_;

    // Output bytes. The initial values are two bytes representing the number 12121 (little-endian
    // order), which is the initial Hugor control file handshake.
    std::queue<int> output_{{0x59, 0x2F}};

    Opcode popOpcode();
    int popValue();
    void pushOutput(int val);
    void pushOutput(OpcodeResult res);
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
