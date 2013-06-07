extern "C" {
#include "heheader.h"
}
#include "version.h"
#include "aboutdialog.h"
#include "ui_aboutdialog.h"


AboutDialog::AboutDialog( QWidget* parent )
    : QDialog(parent, Qt::MSWindowsFixedSizeDialogHint | Qt::CustomizeWindowHint | Qt::WindowTitleHint
#if QT_VERSION >= 0x040500
              | Qt::WindowCloseButtonHint
#endif
             ),
      ui(new Ui::AboutDialog)
{
    ui->setupUi(this);

    // Make the dialog fixed-size.
    this->layout()->setSizeConstraint(QLayout::SetFixedSize);

    // Construct a string holding all version info.
    QString str("<p>Hugor v");
    str += HUGOR_VERSION;
    str += ("<br>Hugo engine v");
    str += QString::number(HEVERSION) + "." + QString::number(HEREVISION)
           + HEINTERIM + "</p>";

    str += "<p>For bug reports or any other form of feedback, you can send email"
            " to <a href=\"mailto:realnc@gmail.com\">realnc@gmail.com</a></p>";

    ui->aboutLabel->setText(ui->aboutLabel->text() + str);
}


AboutDialog::~AboutDialog()
{
    delete ui;
}
