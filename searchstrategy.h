#ifndef SEARCHSTRATEGY_H
#define SEARCHSTRATEGY_H

#include <QList>
#include "photo.h"

class SearchStrategy {
public:
    virtual ~SearchStrategy() = default;
    virtual QList<Photo*> search(const QList<Photo*>& photos, const QString& param) const = 0;
};

#endif // SEARCHSTRATEGY_H
