/* Copyright 2015 Nikos Chantziaras
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
#include <QMessageBox>
#include <gst/gst.h>
#include <gst/gstplugin.h>

#include "happlication.h"
#include "settings.h"

#ifdef Q_OS_WIN
extern "C" {
// GST_PLUGIN_STATIC_DECLARE(accurip);
GST_PLUGIN_STATIC_DECLARE(adder);
GST_PLUGIN_STATIC_DECLARE(adpcmdec);
GST_PLUGIN_STATIC_DECLARE(adpcmenc);
GST_PLUGIN_STATIC_DECLARE(aiff);
GST_PLUGIN_STATIC_DECLARE(alaw);
GST_PLUGIN_STATIC_DECLARE(alpha);
GST_PLUGIN_STATIC_DECLARE(alphacolor);
GST_PLUGIN_STATIC_DECLARE(app);
GST_PLUGIN_STATIC_DECLARE(asfmux);
GST_PLUGIN_STATIC_DECLARE(audioconvert);
GST_PLUGIN_STATIC_DECLARE(audiofxbad);
GST_PLUGIN_STATIC_DECLARE(audiomixer);
GST_PLUGIN_STATIC_DECLARE(audioparsers);
GST_PLUGIN_STATIC_DECLARE(audiorate);
GST_PLUGIN_STATIC_DECLARE(audioresample);
GST_PLUGIN_STATIC_DECLARE(audiotestsrc);
// GST_PLUGIN_STATIC_DECLARE(audiovisualizers);
GST_PLUGIN_STATIC_DECLARE(auparse);
GST_PLUGIN_STATIC_DECLARE(autoconvert);
GST_PLUGIN_STATIC_DECLARE(autodetect);
GST_PLUGIN_STATIC_DECLARE(avi);
GST_PLUGIN_STATIC_DECLARE(bayer);
// GST_PLUGIN_STATIC_DECLARE(bz2);
// GST_PLUGIN_STATIC_DECLARE(cairo);
GST_PLUGIN_STATIC_DECLARE(camerabin);
GST_PLUGIN_STATIC_DECLARE(coloreffects);
GST_PLUGIN_STATIC_DECLARE(compositor);
GST_PLUGIN_STATIC_DECLARE(coreelements);
GST_PLUGIN_STATIC_DECLARE(cutter);
GST_PLUGIN_STATIC_DECLARE(d3dsinkwrapper);
// GST_PLUGIN_STATIC_DECLARE(dataurisrc);
// GST_PLUGIN_STATIC_DECLARE(debug);
// GST_PLUGIN_STATIC_DECLARE(debugutilsbad);
GST_PLUGIN_STATIC_DECLARE(directsound);
GST_PLUGIN_STATIC_DECLARE(directsoundsrc);
GST_PLUGIN_STATIC_DECLARE(dtmf);
GST_PLUGIN_STATIC_DECLARE(dvbsuboverlay);
GST_PLUGIN_STATIC_DECLARE(dvdspu);
GST_PLUGIN_STATIC_DECLARE(effectv);
GST_PLUGIN_STATIC_DECLARE(encoding);
GST_PLUGIN_STATIC_DECLARE(festival);
GST_PLUGIN_STATIC_DECLARE(fieldanalysis);
GST_PLUGIN_STATIC_DECLARE(flv);
GST_PLUGIN_STATIC_DECLARE(flxdec);
GST_PLUGIN_STATIC_DECLARE(freeverb);
GST_PLUGIN_STATIC_DECLARE(frei0r);
GST_PLUGIN_STATIC_DECLARE(gaudieffects);
GST_PLUGIN_STATIC_DECLARE(gdp);
GST_PLUGIN_STATIC_DECLARE(geometrictransform);
GST_PLUGIN_STATIC_DECLARE(gio);
GST_PLUGIN_STATIC_DECLARE(goom2k1);
GST_PLUGIN_STATIC_DECLARE(goom);
GST_PLUGIN_STATIC_DECLARE(icydemux);
GST_PLUGIN_STATIC_DECLARE(id3demux);
GST_PLUGIN_STATIC_DECLARE(id3tag);
GST_PLUGIN_STATIC_DECLARE(imagefreeze);
GST_PLUGIN_STATIC_DECLARE(inter);
GST_PLUGIN_STATIC_DECLARE(interlace);
GST_PLUGIN_STATIC_DECLARE(interleave);
GST_PLUGIN_STATIC_DECLARE(isomp4);
GST_PLUGIN_STATIC_DECLARE(ivfparse);
GST_PLUGIN_STATIC_DECLARE(ivtc);
GST_PLUGIN_STATIC_DECLARE(jp2kdecimator);
GST_PLUGIN_STATIC_DECLARE(jpeg);
GST_PLUGIN_STATIC_DECLARE(jpegformat);
GST_PLUGIN_STATIC_DECLARE(level);
GST_PLUGIN_STATIC_DECLARE(libav);
GST_PLUGIN_STATIC_DECLARE(liveadder);
GST_PLUGIN_STATIC_DECLARE(matroska);
GST_PLUGIN_STATIC_DECLARE(midi);
// GST_PLUGIN_STATIC_DECLARE(modplug);
GST_PLUGIN_STATIC_DECLARE(mpegpsdemux);
GST_PLUGIN_STATIC_DECLARE(mpegpsmux);
GST_PLUGIN_STATIC_DECLARE(mpegtsdemux);
GST_PLUGIN_STATIC_DECLARE(mpegtsmux);
// GST_PLUGIN_STATIC_DECLARE(mpg123);
GST_PLUGIN_STATIC_DECLARE(mulaw);
GST_PLUGIN_STATIC_DECLARE(multifile);
GST_PLUGIN_STATIC_DECLARE(multipart);
GST_PLUGIN_STATIC_DECLARE(mxf);
GST_PLUGIN_STATIC_DECLARE(navigationtest);
// GST_PLUGIN_STATIC_DECLARE(ogg);
// GST_PLUGIN_STATIC_DECLARE(opengl);
GST_PLUGIN_STATIC_DECLARE(pcapparse);
GST_PLUGIN_STATIC_DECLARE(playback);
GST_PLUGIN_STATIC_DECLARE(png);
GST_PLUGIN_STATIC_DECLARE(pnm);
GST_PLUGIN_STATIC_DECLARE(rawparse);
GST_PLUGIN_STATIC_DECLARE(removesilence);
GST_PLUGIN_STATIC_DECLARE(replaygain);
GST_PLUGIN_STATIC_DECLARE(rfbsrc);
GST_PLUGIN_STATIC_DECLARE(rtpmanager);
GST_PLUGIN_STATIC_DECLARE(rtsp);
GST_PLUGIN_STATIC_DECLARE(sdp);
GST_PLUGIN_STATIC_DECLARE(segmentclip);
GST_PLUGIN_STATIC_DECLARE(shapewipe);
GST_PLUGIN_STATIC_DECLARE(smooth);
GST_PLUGIN_STATIC_DECLARE(smpte);
// GST_PLUGIN_STATIC_DECLARE(sndfile);
// GST_PLUGIN_STATIC_DECLARE(spectrum);
GST_PLUGIN_STATIC_DECLARE(speed);
GST_PLUGIN_STATIC_DECLARE(stereo);
GST_PLUGIN_STATIC_DECLARE(subenc);
GST_PLUGIN_STATIC_DECLARE(subparse);
GST_PLUGIN_STATIC_DECLARE(tcp);
GST_PLUGIN_STATIC_DECLARE(typefindfunctions);
GST_PLUGIN_STATIC_DECLARE(udp);
GST_PLUGIN_STATIC_DECLARE(videobox);
GST_PLUGIN_STATIC_DECLARE(videoconvert);
GST_PLUGIN_STATIC_DECLARE(videocrop);
GST_PLUGIN_STATIC_DECLARE(videofilter);
GST_PLUGIN_STATIC_DECLARE(videofiltersbad);
GST_PLUGIN_STATIC_DECLARE(videomixer);
GST_PLUGIN_STATIC_DECLARE(videoparsersbad);
GST_PLUGIN_STATIC_DECLARE(videorate);
GST_PLUGIN_STATIC_DECLARE(videoscale);
GST_PLUGIN_STATIC_DECLARE(videosignal);
GST_PLUGIN_STATIC_DECLARE(videotestsrc);
GST_PLUGIN_STATIC_DECLARE(vmnc);
GST_PLUGIN_STATIC_DECLARE(volume);
// GST_PLUGIN_STATIC_DECLARE(vorbis);
GST_PLUGIN_STATIC_DECLARE(wasapi);
GST_PLUGIN_STATIC_DECLARE(waveform);
GST_PLUGIN_STATIC_DECLARE(wavenc);
GST_PLUGIN_STATIC_DECLARE(wavparse);
GST_PLUGIN_STATIC_DECLARE(winks);
GST_PLUGIN_STATIC_DECLARE(winscreencap);
GST_PLUGIN_STATIC_DECLARE(y4mdec);
GST_PLUGIN_STATIC_DECLARE(y4menc);
GST_PLUGIN_STATIC_DECLARE(yadif);
} // extern "C"

static void registerGstStaticPlugins()
{
    //    GST_PLUGIN_STATIC_REGISTER(accurip);
    GST_PLUGIN_STATIC_REGISTER(adder);
    GST_PLUGIN_STATIC_REGISTER(adpcmdec);
    GST_PLUGIN_STATIC_REGISTER(adpcmenc);
    GST_PLUGIN_STATIC_REGISTER(aiff);
    GST_PLUGIN_STATIC_REGISTER(alaw);
    GST_PLUGIN_STATIC_REGISTER(alpha);
    GST_PLUGIN_STATIC_REGISTER(alphacolor);
    GST_PLUGIN_STATIC_REGISTER(app);
    GST_PLUGIN_STATIC_REGISTER(asfmux);
    GST_PLUGIN_STATIC_REGISTER(audioconvert);
    GST_PLUGIN_STATIC_REGISTER(audiofxbad);
    GST_PLUGIN_STATIC_REGISTER(audiomixer);
    GST_PLUGIN_STATIC_REGISTER(audioparsers);
    GST_PLUGIN_STATIC_REGISTER(audiorate);
    GST_PLUGIN_STATIC_REGISTER(audioresample);
    GST_PLUGIN_STATIC_REGISTER(audiotestsrc);
    //    GST_PLUGIN_STATIC_REGISTER(audiovisualizers);
    GST_PLUGIN_STATIC_REGISTER(auparse);
    GST_PLUGIN_STATIC_REGISTER(autoconvert);
    GST_PLUGIN_STATIC_REGISTER(autodetect);
    GST_PLUGIN_STATIC_REGISTER(avi);
    GST_PLUGIN_STATIC_REGISTER(bayer);
    //    GST_PLUGIN_STATIC_REGISTER(bz2);
    //    GST_PLUGIN_STATIC_REGISTER(cairo);
    GST_PLUGIN_STATIC_REGISTER(camerabin);
    GST_PLUGIN_STATIC_REGISTER(coloreffects);
    GST_PLUGIN_STATIC_REGISTER(compositor);
    GST_PLUGIN_STATIC_REGISTER(coreelements);
    GST_PLUGIN_STATIC_REGISTER(cutter);
    GST_PLUGIN_STATIC_REGISTER(d3dsinkwrapper);
    //    GST_PLUGIN_STATIC_REGISTER(dataurisrc);
    //    GST_PLUGIN_STATIC_REGISTER(debug);
    //    GST_PLUGIN_STATIC_REGISTER(debugutilsbad);
    GST_PLUGIN_STATIC_REGISTER(directsound);
    GST_PLUGIN_STATIC_REGISTER(directsoundsrc);
    GST_PLUGIN_STATIC_REGISTER(dtmf);
    GST_PLUGIN_STATIC_REGISTER(dvbsuboverlay);
    GST_PLUGIN_STATIC_REGISTER(dvdspu);
    GST_PLUGIN_STATIC_REGISTER(effectv);
    GST_PLUGIN_STATIC_REGISTER(encoding);
    GST_PLUGIN_STATIC_REGISTER(festival);
    GST_PLUGIN_STATIC_REGISTER(fieldanalysis);
    GST_PLUGIN_STATIC_REGISTER(flv);
    GST_PLUGIN_STATIC_REGISTER(flxdec);
    GST_PLUGIN_STATIC_REGISTER(freeverb);
    GST_PLUGIN_STATIC_REGISTER(frei0r);
    GST_PLUGIN_STATIC_REGISTER(gaudieffects);
    GST_PLUGIN_STATIC_REGISTER(gdp);
    GST_PLUGIN_STATIC_REGISTER(geometrictransform);
    GST_PLUGIN_STATIC_REGISTER(gio);
    GST_PLUGIN_STATIC_REGISTER(goom2k1);
    GST_PLUGIN_STATIC_REGISTER(goom);
    GST_PLUGIN_STATIC_REGISTER(icydemux);
    GST_PLUGIN_STATIC_REGISTER(id3demux);
    GST_PLUGIN_STATIC_REGISTER(id3tag);
    GST_PLUGIN_STATIC_REGISTER(imagefreeze);
    GST_PLUGIN_STATIC_REGISTER(inter);
    GST_PLUGIN_STATIC_REGISTER(interlace);
    GST_PLUGIN_STATIC_REGISTER(interleave);
    GST_PLUGIN_STATIC_REGISTER(isomp4);
    GST_PLUGIN_STATIC_REGISTER(ivfparse);
    GST_PLUGIN_STATIC_REGISTER(ivtc);
    GST_PLUGIN_STATIC_REGISTER(jp2kdecimator);
    GST_PLUGIN_STATIC_REGISTER(jpeg);
    GST_PLUGIN_STATIC_REGISTER(jpegformat);
    GST_PLUGIN_STATIC_REGISTER(level);
    GST_PLUGIN_STATIC_REGISTER(libav);
    GST_PLUGIN_STATIC_REGISTER(liveadder);
    GST_PLUGIN_STATIC_REGISTER(matroska);
    GST_PLUGIN_STATIC_REGISTER(midi);
    //    GST_PLUGIN_STATIC_REGISTER(modplug);
    GST_PLUGIN_STATIC_REGISTER(mpegpsdemux);
    GST_PLUGIN_STATIC_REGISTER(mpegpsmux);
    GST_PLUGIN_STATIC_REGISTER(mpegtsdemux);
    GST_PLUGIN_STATIC_REGISTER(mpegtsmux);
    //    GST_PLUGIN_STATIC_REGISTER(mpg123);
    GST_PLUGIN_STATIC_REGISTER(mulaw);
    GST_PLUGIN_STATIC_REGISTER(multifile);
    GST_PLUGIN_STATIC_REGISTER(multipart);
    GST_PLUGIN_STATIC_REGISTER(mxf);
    GST_PLUGIN_STATIC_REGISTER(navigationtest);
    //    GST_PLUGIN_STATIC_REGISTER(ogg);
    //    GST_PLUGIN_STATIC_REGISTER(opengl);
    GST_PLUGIN_STATIC_REGISTER(pcapparse);
    GST_PLUGIN_STATIC_REGISTER(playback);
    GST_PLUGIN_STATIC_REGISTER(png);
    GST_PLUGIN_STATIC_REGISTER(pnm);
    GST_PLUGIN_STATIC_REGISTER(rawparse);
    GST_PLUGIN_STATIC_REGISTER(removesilence);
    GST_PLUGIN_STATIC_REGISTER(replaygain);
    GST_PLUGIN_STATIC_REGISTER(rfbsrc);
    GST_PLUGIN_STATIC_REGISTER(rtpmanager);
    GST_PLUGIN_STATIC_REGISTER(rtsp);
    GST_PLUGIN_STATIC_REGISTER(sdp);
    GST_PLUGIN_STATIC_REGISTER(segmentclip);
    GST_PLUGIN_STATIC_REGISTER(shapewipe);
    GST_PLUGIN_STATIC_REGISTER(smooth);
    GST_PLUGIN_STATIC_REGISTER(smpte);
    //    GST_PLUGIN_STATIC_REGISTER(sndfile);
    //    GST_PLUGIN_STATIC_REGISTER(spectrum);
    GST_PLUGIN_STATIC_REGISTER(speed);
    GST_PLUGIN_STATIC_REGISTER(stereo);
    GST_PLUGIN_STATIC_REGISTER(subenc);
    GST_PLUGIN_STATIC_REGISTER(subparse);
    GST_PLUGIN_STATIC_REGISTER(tcp);
    GST_PLUGIN_STATIC_REGISTER(typefindfunctions);
    GST_PLUGIN_STATIC_REGISTER(udp);
    GST_PLUGIN_STATIC_REGISTER(videobox);
    GST_PLUGIN_STATIC_REGISTER(videoconvert);
    GST_PLUGIN_STATIC_REGISTER(videocrop);
    GST_PLUGIN_STATIC_REGISTER(videofilter);
    GST_PLUGIN_STATIC_REGISTER(videofiltersbad);
    GST_PLUGIN_STATIC_REGISTER(videomixer);
    GST_PLUGIN_STATIC_REGISTER(videoparsersbad);
    GST_PLUGIN_STATIC_REGISTER(videorate);
    GST_PLUGIN_STATIC_REGISTER(videoscale);
    GST_PLUGIN_STATIC_REGISTER(videosignal);
    GST_PLUGIN_STATIC_REGISTER(videotestsrc);
    GST_PLUGIN_STATIC_REGISTER(vmnc);
    GST_PLUGIN_STATIC_REGISTER(volume);
    //    GST_PLUGIN_STATIC_REGISTER(vorbis);
    GST_PLUGIN_STATIC_REGISTER(wasapi);
    GST_PLUGIN_STATIC_REGISTER(waveform);
    GST_PLUGIN_STATIC_REGISTER(wavenc);
    GST_PLUGIN_STATIC_REGISTER(wavparse);
    GST_PLUGIN_STATIC_REGISTER(winks);
    GST_PLUGIN_STATIC_REGISTER(winscreencap);
    GST_PLUGIN_STATIC_REGISTER(y4mdec);
    GST_PLUGIN_STATIC_REGISTER(y4menc);
    GST_PLUGIN_STATIC_REGISTER(yadif);
}
#endif // Q_OS_WIN

void initVideoEngine(int& argc, char* argv[])
{
    GError* gstError = nullptr;

    if (not gst_init_check(&argc, &argv, &gstError)) {
        QString errMsg(
            QObject::tr("Unable to use GStreamer. Video support will be "
                        "disabled."));
        if (gstError->message != nullptr && qstrlen(gstError->message) > 0) {
            errMsg += QObject::tr("The GStreamer error was: ")
                      + QString::fromLocal8Bit(gstError->message);
        }
        g_error_free(gstError);
        QMessageBox::critical(nullptr, HApplication::applicationName(), errMsg);
        hApp->settings()->videoSysError = true;
    }

#ifdef Q_OS_WIN
    registerGstStaticPlugins();
#endif
}

void closeVideoEngine()
{}
