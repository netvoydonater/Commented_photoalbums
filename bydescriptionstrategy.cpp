#include "bydescriptionstrategy.h"

QList<Photo*> ByDescriptionStrategy::search(const QList<Photo*>& photos, const QString& param) const {
    QList<Photo*> result;
    for (auto* photo : photos) {
        if (photo->getDescription().contains(param, Qt::CaseInsensitive)) {
            result.append(photo);
        }
    }
    return result;
}
