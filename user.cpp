#include "user.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>

User::User(const QString& name) : name(name), rootAlbum(new Album("Root")) {}

QString User::getName() const { return name; }

void User::setRootAlbum(Album* album) { rootAlbum = album; }

Album* User::getRootAlbum() const { return rootAlbum; }

void User::saveToJson(const QString& filePath) const {
    QJsonObject userObj;
    userObj["name"] = name;
    userObj["rootAlbum"] = rootAlbum->toJson();
    QJsonDocument doc(userObj);
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
    }
}

User* User::loadFromJson(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return nullptr;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject userObj = doc.object();
    User* user = new User(userObj["name"].toString());
    user->rootAlbum = Album::fromJson(userObj["rootAlbum"].toObject());
    return user;
}
