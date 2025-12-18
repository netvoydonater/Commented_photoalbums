#ifndef BYDATESTRATEGY_H
#define BYDATESTRATEGY_H

#include "searchstrategy.h"

class ByDateStrategy : public SearchStrategy {
public:
    QList<Photo*> search(const QList<Photo*>& photos, const QString& param) const override;
};

#endif // BYDATESTRATEGY_H
