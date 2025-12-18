#ifndef BYDESCRIPTIONSTRATEGY_H
#define BYDESCRIPTIONSTRATEGY_H

#include "searchstrategy.h"

class ByDescriptionStrategy : public SearchStrategy {
public:
    QList<Photo*> search(const QList<Photo*>& photos, const QString& param) const override;
};

#endif // BYDESCRIPTIONSTRATEGY_H
