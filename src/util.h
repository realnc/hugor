#pragma once
#include <QApplication>

template <typename F>
static void runInMainThread(F&& fun)
{
    QObject tmp;
    QObject::connect(&tmp, &QObject::destroyed, qApp, std::forward<F>(fun),
                     Qt::BlockingQueuedConnection);
}
