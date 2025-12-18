#include "tag.h"
#include <QJsonObject>

Tag::Tag(const QString& name) : name(name) {}

QString Tag::getName() const { return name; }

QJsonObject Tag::toJson() const {
    QJsonObject obj;
    obj["name"] = name;
    return obj;
}

Tag Tag::fromJson(const QJsonObject& obj) {
    return Tag(obj["name"].toString());
}
