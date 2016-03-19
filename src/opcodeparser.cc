#include <QApplication>
#include <QMetaObject>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QClipboard>
#include <QDebug>
#include "opcodeparser.h"
extern "C" {
#include "heheader.h"
}
#include "hugohandlers.h"
#include "hmainwindow.h"

enum class OpcodeParser::Opcode: std::int16_t {
    GET_VERSION         =   100,
    GET_OS              =   200,
    ABORT               =   300,
    FADE_SCREEN         =   400,
    OPEN_URL            =   500,
    SET_FULLSCREEN      =   600,
    SET_CLIPBOARD       =   700,
};

enum class OpcodeParser::OpcodeResult: std::int16_t {
    OK                  =  0,
    WRONG_PARAM_COUNT   = 10,
    WRONG_BYTE_COUNT    = 20,
};

static const int OS_TYPE =
#ifdef Q_OS_WIN32
        1;
#elif defined(Q_OS_OSX)
        2;
#elif defined(Q_OS_LINUX)
        3;
#else
        0;
#endif


static void
clearQueue(std::queue<int>& q)
{
    std::queue<int> empty;
    std::swap(q, empty);
}


OpcodeParser::Opcode
OpcodeParser::popOpcode()
{
    return static_cast<Opcode>(popValue());
}


int
OpcodeParser::popValue()
{
    Q_ASSERT(fBuffer.size() > 1);
    auto byte1 = fBuffer.front();
    fBuffer.pop();
    auto byte2 = fBuffer.front();
    fBuffer.pop();
    return byte1 + (byte2 << 8);
}


void
OpcodeParser::fPushOutput(int val)
{
    fOutput.push(val & 0xFF);
    fOutput.push((val >> 8) & 0xFF);
}


void
OpcodeParser::fPushOutput(OpcodeParser::OpcodeResult res)
{
    fPushOutput(static_cast<int>(res));
}


void
OpcodeParser::pushByte(const int c)
{
    fBuffer.push(c);
}


int
OpcodeParser::getNextOutputByte()
{
    auto ret = fOutput.front();
    fOutput.pop();
    return ret;
}


void
OpcodeParser::parse()
{
    clearQueue(fOutput);

    if (fBuffer.empty()) {
        return;
    }

    if(fBuffer.size() % 2) {
        // Opcodes and parameters are always byte pairs. We got an odd number
        // of bytes, so discard the input as incomplete.
        clearQueue(fBuffer);
        fPushOutput(OpcodeResult::WRONG_BYTE_COUNT);
        qWarning() << "Incomplete opcode input.";
        return;
    }

    // Get the opcode and amount of parameters.
    auto opcode = popOpcode();
    auto paramCount = fBuffer.size() / 2u;

    switch (opcode) {
    case Opcode::GET_VERSION:
        if (paramCount != 0) {
            fPushOutput(OpcodeResult::WRONG_PARAM_COUNT);
            break;
        }
        fPushOutput(OpcodeResult::OK);
        fPushOutput(1);
        fPushOutput(0);
        fPushOutput(99);
        break;

    case Opcode::GET_OS:
        if (paramCount != 0) {
            fPushOutput(OpcodeResult::WRONG_PARAM_COUNT);
            break;
        }
        fPushOutput(OpcodeResult::OK);
        fPushOutput(OS_TYPE);
        break;

    case Opcode::ABORT:
        if (paramCount != 0) {
            fPushOutput(OpcodeResult::WRONG_PARAM_COUNT);
            break;
        }
        abort();

    case Opcode::FADE_SCREEN: {
        if (paramCount != 4) {
            fPushOutput(OpcodeResult::WRONG_PARAM_COUNT);
            break;
        }
        auto duration = popValue();
        auto startAlpha = popValue();
        auto endAlpha = popValue();
        bool block = popValue();
        QMetaObject::invokeMethod(hHandlers, "fadeScreen", Qt::BlockingQueuedConnection,
                                  Q_ARG(int, duration), Q_ARG(int, startAlpha), Q_ARG(int, endAlpha),
                                  Q_ARG(bool, block));
        fPushOutput(OpcodeResult::OK);
        break;
    }

    case Opcode::OPEN_URL:
        if (paramCount != 1) {
            fPushOutput(OpcodeResult::WRONG_PARAM_COUNT);
            break;
        }
        QDesktopServices::openUrl(QUrl::fromUserInput(GetWord(popValue()), QDir::currentPath(),
                                                      QUrl::AssumeLocalFile));
        fPushOutput(OpcodeResult::OK);
        break;

    case Opcode::SET_FULLSCREEN: {
        if (paramCount != 1) {
            fPushOutput(OpcodeResult::WRONG_PARAM_COUNT);
            break;
        }
        bool f = popValue();
        QMetaObject::invokeMethod(hMainWin, "setFullscreen", Qt::BlockingQueuedConnection, Q_ARG(bool, f));
        fPushOutput(OpcodeResult::OK);
        break;
    }

    case Opcode::SET_CLIPBOARD: {
        if (paramCount != 1) {
            fPushOutput(OpcodeResult::WRONG_PARAM_COUNT);
            break;
        }
        auto cb = QApplication::clipboard();
        cb->setText(GetWord(popValue()));
        fPushOutput(OpcodeResult::OK);
        break;
    }

    default:
        qWarning() << "Unrecognized opcode:" << (int)opcode;
    }

    clearQueue(fBuffer);
}
