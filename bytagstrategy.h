#ifndef BYTAGSTRATEGY_H
#define BYTAGSTRATEGY_H

#include "searchstrategy.h"

class ByTagStrategy : public SearchStrategy {
public:
    QList<Photo*> search(const QList<Photo*>& photos, const QString& param) const override;
};

#endif // BYTAGSTRATEGY_H
