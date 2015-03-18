#ifndef ENGINERUNNER_H
#define ENGINERUNNER_H

#include <QObject>


class EngineRunner: public QObject {
    Q_OBJECT

  public:
    EngineRunner(QString gameFile, QObject* parent = 0)
        : QObject(parent),
          fGameFile(gameFile)
    { }

  signals:
    void finished();

  public slots:
    void runEngine();

  private:
    QString fGameFile;
};


#endif
