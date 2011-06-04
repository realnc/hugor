#ifndef HDISPITEM_H
#define HDISPITEM_H

#include <QString>
#include <QImage>


class HDispItem {
  private:
    int fXPos;
    int fYPos;

  public:
    HDispItem( int xPos, int yPos )
        : fXPos(xPos),
          fYPos(yPos)
    { }

    int
    xPos() const
    { return this->fXPos; }

    int
    yPos() const
    { return this->fYPos; }

    void
    setXPos( int v )
    { this->fXPos = v; }

    void
    setYPos( int v )
    { this->fYPos = v; }
};


class HDispItemText: public HDispItem {
  private:
    QString fText;
    int fFgColor;
    int fBgColor;

  public:
    HDispItemText( const QString& text, int xPos, int yPos, int fgColor, int bgColor )
        : HDispItem(xPos, yPos),
          fText(text),
          fFgColor(fgColor),
          fBgColor(bgColor)
    { }

    const QString&
    text() const
    { return this->fText; }

    int
    fgColor() const
    { return this->fFgColor; }

    int bgColor() const
    { return this->fBgColor; }
};


class HDispItemImage: public HDispItem {
  private:
    QImage fImage;

  public:
    HDispItemImage( const QImage& image, int xPos, int yPos )
        : HDispItem(xPos, yPos),
          fImage(image)
    { }

    const QImage&
    image() const
    { return this->fImage; }
};


class HDispItemColorFill: public HDispItem {
  private:
    int fColor;
    int fWidth;
    int fHeight;

  public:
    HDispItemColorFill( int color, int xPos, int yPos, int width, int height )
        : HDispItem(xPos, yPos),
          fColor(color),
          fWidth(width),
          fHeight(height)
    { }

    int
    color() const
    { return this->fColor; }

    int
    width() const
    { return this->fWidth; }

    int
    height() const
    { return this->fHeight; }
};


#endif // HDISPITEM_H
