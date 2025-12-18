#include "bydatestrategy.h"

QList<Photo*> ByDateStrategy::search(const QList<Photo*>& photos, const QString& param) const {
    QDateTime date = QDateTime::fromString(param, "yyyy-MM-dd");
    if (!date.isValid()) return {};
    QList<Photo*> result;
    for (auto* photo : photos) {
        if (photo->getDate().date() == date.date()) {
            result.append(photo);
        }
    }
    return result;
}
