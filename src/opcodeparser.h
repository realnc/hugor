#pragma once
#include <queue>
#include <cstdint>
#include <QObject>

class OpcodeParser final
{
    Q_GADGET
public:
    void pushByte(int c);

    bool hasOutput() const
    {
        return not fOutput.empty();
    }

    int getNextOutputByte();

    void parse();

    enum class Opcode: std::int16_t {
        IS_OPCODE_AVAILABLE     =     1,
        GET_VERSION             =   100,
        GET_OS                  =   200,
        ABORT                   =   300,
        FADE_SCREEN             =   400,
        OPEN_URL                =   500,
        SET_FULLSCREEN          =   600,
        SET_CLIPBOARD           =   700,
        IS_MUSIC_PLAYING        =   800,
        IS_SAMPLE_PLAYING       =   900,
        IS_FLUID_LAYOUT         =  1000,
        SET_COLOR               =  1100,
        IS_FULLSCREEN           =  1200,
        HIDES_CURSOR            =  1300,
        TOP_JUSTIFIED           =  1400,
        SCREENREADER_CAPABLE    =  1500,
        CHECK_RESOURCE          =  1600,
    };

    enum class OpcodeResult: std::int16_t {
        OK                      =  0,
        WRONG_PARAM_COUNT       = 10,
        WRONG_BYTE_COUNT        = 20,
        UNKNOWN_OPCODE          = 30,
    };

    Q_ENUM(Opcode)

private:
    // Bytes to be parsed.
    std::queue<int> fBuffer;

    // Output bytes. The initial values are two bytes representing the number
    // 12121 (little-endian order), which is the initial Hugor control file
    // handshake.
    std::queue<int> fOutput {{0x59, 0x2F}};

    Opcode popOpcode();
    int popValue();
    void fPushOutput(int val);
    void fPushOutput(OpcodeResult res);
};
