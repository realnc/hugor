extern "C" {
#include "heheader.h"
}
#include "version.h"
#include "aboutdialog.h"
#include "ui_aboutdialog.h"

AboutDialog::AboutDialog( QWidget* parent )
    : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
      ui(new Ui::AboutDialog)
{
    ui->setupUi(this);

    // Make the dialog fixed-size.
    this->setMaximumSize(this->minimumSize());

    // Construct a string holding all version info.
    QString str("<p>Hugor v");
    str += HUGOR_VERSION;
    str += ("<br>Hugo engine v");
    str += QString::number(HEVERSION) + "." + QString::number(HEREVISION)
           + HEINTERIM + "</p>";

    // FMOD license requirement.
#ifdef SOUND_FMOD
    str += "<p>Audio engine: FMOD Sound System by Firelight Technologies</p>";
#endif
    str += "<p>For bug reports or any other form of feedback, you can send email"
            " to <a href=\"mailto:realnc@gmail.com\">realnc@gmail.com</a></p>";

    ui->label->setText(ui->label->text() + str);
}


AboutDialog::~AboutDialog()
{
    delete ui;
}
