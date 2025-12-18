#include "bytagstrategy.h"

QList<Photo*> ByTagStrategy::search(const QList<Photo*>& photos, const QString& param) const {
    QList<Photo*> result;
    for (auto* photo : photos) {
        for (const auto& tag : photo->getTags()) {
            if (tag.getName().contains(param, Qt::CaseInsensitive)) {
                result.append(photo);
                break;
            }
        }
    }
    return result;
}
