// This is copyrighted software. More information is at the end of this file.
#include "util.h"
#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QUrl>
#include <QtGlobal>
#ifdef Q_OS_MAC
#include <CoreFoundation/CFBundle.h>
#endif

#ifdef Q_OS_MAC
QDir getMacosAppBundleDir()
{
    const auto* const bundle_url = static_cast<CFURLRef>(
        CFAutorelease(static_cast<CFURLRef>(CFBundleCopyBundleURL(CFBundleGetMainBundle()))));
    return QDir(QUrl::fromCFURL(bundle_url).path());
}
#endif

QString getAutoloadPathPrefix()
{
#ifdef Q_OS_MAC
    QDir bundle_dir = getMacosAppBundleDir();
    const QFileInfo info(bundle_dir.absolutePath());
    bundle_dir.cdUp();
    QString bundle_path = bundle_dir.absolutePath();
    if (bundle_path.endsWith('/')) {
        bundle_path.chop(1);
    }
    return bundle_path + '/' + info.baseName();
#else
#ifdef Q_OS_LINUX
    const auto app_image_path = qgetenv("APPIMAGE");
#else
    const QByteArray app_image_path;
#endif
    const QFileInfo app_image_info(app_image_path);
    const bool is_app_image = not app_image_path.isEmpty();
    QString prefix;

    if (is_app_image) {
        prefix = app_image_info.absolutePath();
    } else {
        prefix = QCoreApplication::applicationDirPath();
    }
    if (prefix.endsWith('/')) {
        prefix.chop(1);
    }
    if (is_app_image) {
        prefix += '/' + app_image_info.baseName();
    } else {
        prefix += '/' + QFileInfo(QCoreApplication::applicationFilePath()).baseName();
    }
    return prefix;
#endif
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
