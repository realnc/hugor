#ifndef OPCODEPARSER_H
#define OPCODEPARSER_H

#include <queue>
#include <vector>
#include <cstddef>
#include <cstdint>

class OpcodeParser
{
public:
    void pushByte(const int c);

    bool hasOutput() const
    {
        return not fOutput.empty();
    }

    int getNextOutputByte();

    void parse();

private:
    enum class Opcode: std::int16_t;
    enum class OpcodeResult: std::int16_t;

    // Bytes to be parsed.
    std::queue<int> fBuffer;

    // Output bytes. The initial values are two bytes representing the number
    // 12121 (little-endian order), which is the initial Hugor control file
    // handshake.
    std::queue<int> fOutput = std::queue<int>({0x59, 0x2F});

    static int fParamCount(Opcode opcode);
    static Opcode fIntToOpcode(int value);

    Opcode popOpcode();
    int popValue();
    void fPushOutput(int val);
    void fPushOutput(OpcodeResult res);
};

#endif // OPCODEPARSER_H
