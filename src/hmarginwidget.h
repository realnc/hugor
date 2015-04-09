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
#ifndef HMARGINWIDGET_H
#define HMARGINWIDGET_H

#include <QWidget>


class HMarginWidget: public QWidget {
    Q_OBJECT

  private:
    QWidget* fBannerWidget;
    class QVBoxLayout* fLayout;
    QColor fColor;

  public:
    HMarginWidget( QWidget* parent = 0 );

    void
    setBannerWidget( QWidget* w );

    QWidget*
    bannerWidget()
    { return this->fBannerWidget; }

    void
    addWidget( QWidget* w );

    void
    removeWidget( QWidget* w );

    void
    setColor(QColor color);

  protected:
    virtual void
    wheelEvent( QWheelEvent* e );

    virtual void
    mouseMoveEvent( QMouseEvent* e );

    virtual void
    paintEvent(QPaintEvent*);
};


#endif
