#include <QApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QClipboard>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QDebug>
#include "opcodeparser.h"
extern "C" {
#include "heheader.h"
}
#include "hugohandlers.h"
#include "hmainwindow.h"
#include "happlication.h"
#include "hframe.h"
#include "util.h"
#include "hugodefs.h"

enum class OpcodeParser::Opcode: std::int16_t {
    GET_VERSION         =   100,
    GET_OS              =   200,
    ABORT               =   300,
    FADE_SCREEN         =   400,
    OPEN_URL            =   500,
    SET_FULLSCREEN      =   600,
    SET_CLIPBOARD       =   700,
    IS_MUSIC_PLAYING    =   800,
    IS_SAMPLE_PLAYING   =   900,
};

enum class OpcodeParser::OpcodeResult: std::int16_t {
    OK                  =  0,
    WRONG_PARAM_COUNT   = 10,
    WRONG_BYTE_COUNT    = 20,
    UNKNOWN_OPCODE      = 30,
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

        if (duration < 0) {
            duration = 0;
        }
        startAlpha = std::max(-1, std::min(startAlpha, 255));
        endAlpha = std::max(0, std::min(endAlpha, 255));

        runInMainThread([duration, startAlpha, endAlpha, block]
        {
            qreal startF = (qreal)startAlpha / 255;
            qreal endF = (qreal)endAlpha / 255;

            static QGraphicsOpacityEffect* eff = nullptr;
            static QPropertyAnimation* fadeAnim = nullptr;
            static QEventLoop* idle = nullptr;
            if (eff == nullptr) {
                eff = new QGraphicsOpacityEffect(hApp->frameWindow());
                fadeAnim = new QPropertyAnimation(eff, "opacity", eff);
                idle = new QEventLoop(fadeAnim);
                hApp->frameWindow()->setGraphicsEffect(eff);
                QObject::connect(fadeAnim, &QPropertyAnimation::finished, idle, []{idle->exit();});
            }

            fadeAnim->stop();
            fadeAnim->setDuration(duration);
            if (startAlpha >= 0) {
                fadeAnim->setStartValue(startF);
            } else {
                fadeAnim->setStartValue(eff->opacity());
            }
            fadeAnim->setEndValue(endF);
            fadeAnim->setEasingCurve(QEasingCurve::OutQuad);
            if (block) {
                fadeAnim->start();
                idle->exec();
            } else {
                fadeAnim->start();
            }
        });

        fPushOutput(OpcodeResult::OK);
        break;
    }

    case Opcode::OPEN_URL: {
        if (paramCount != 1) {
            fPushOutput(OpcodeResult::WRONG_PARAM_COUNT);
            break;
        }
        auto url = GetWord(popValue());
        runInMainThread([url]
        {
#if QT_VERSION < QT_VERSION_CHECK(5, 4, 0)
            QDesktopServices::openUrl(QUrl::fromUserInput(url));
#else
            QDesktopServices::openUrl(QUrl::fromUserInput(url, QDir::currentPath(), QUrl::AssumeLocalFile));
#endif
        });
        fPushOutput(OpcodeResult::OK);
        break;
    }

    case Opcode::SET_FULLSCREEN: {
        if (paramCount != 1) {
            fPushOutput(OpcodeResult::WRONG_PARAM_COUNT);
            break;
        }
        bool f = popValue();
        runInMainThread([f]{hMainWin->setFullscreen(f);});
        fPushOutput(OpcodeResult::OK);
        break;
    }

    case Opcode::SET_CLIPBOARD: {
        if (paramCount != 1) {
            fPushOutput(OpcodeResult::WRONG_PARAM_COUNT);
            break;
        }
        auto text = GetWord(popValue());
        runInMainThread([text]{QApplication::clipboard()->setText(text);});
        fPushOutput(OpcodeResult::OK);
        break;
    }

    case Opcode::IS_MUSIC_PLAYING: {
        if (paramCount != 0) {
            fPushOutput(OpcodeResult::WRONG_PARAM_COUNT);
            break;
        }
        bool res;
        runInMainThread([&res]{res = isMusicPlaying();});
        fPushOutput(OpcodeResult::OK);
        fPushOutput(res);
        break;
    }

    case Opcode::IS_SAMPLE_PLAYING: {
        if (paramCount != 0) {
            fPushOutput(OpcodeResult::WRONG_PARAM_COUNT);
            break;
        }
        bool res;
        runInMainThread([&res]{res = isSamplePlaying();});
        fPushOutput(OpcodeResult::OK);
        fPushOutput(res);
        break;
    }

    default:
        qWarning() << "Unrecognized opcode:" << (int)opcode;
        fPushOutput(OpcodeResult::UNKNOWN_OPCODE);
        fPushOutput((int)opcode);
        fPushOutput(paramCount);
        for (int i = 0; i < paramCount; ++i) {
            fPushOutput(popValue());
        }
    }

    clearQueue(fBuffer);
}
