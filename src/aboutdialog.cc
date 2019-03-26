// This is copyrighted software. More information is at the end of this file.
#include "aboutdialog.h"

extern "C" {
#include "heheader.h"
}
#include "ui_aboutdialog.h"

AboutDialog::AboutDialog(QWidget* parent)
    : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint)
    , ui_(new Ui::AboutDialog)
{
    ui_->setupUi(this);

    // Construct a string holding all version info.
    QString str("<p>Hugor v");
    str += HUGOR_VERSION;
    str += QLatin1String("<br>Hugo engine v");
    str += QString::number(HEVERSION) + "." + QString::number(HEREVISION) + HEINTERIM + "</p>";

    str +=
        "<p>For bug reports or any other form of feedback, you can use the "
        "<a href=\"https://intfiction.org\">intfiction.org forum</a>, the "
        "<a href=\"https://www.joltcountry.com/phpBB3\">Jolt Country forum</a>, or send email to "
        "<a href=\"mailto:realnc@gmail.com\">realnc@gmail.com</a></p>";

    ui_->aboutLabel->setText(ui_->aboutLabel->text() + str);
}

AboutDialog::~AboutDialog()
{
    delete ui_;
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
