#include "user.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>

User::User(const QString& name) : name(name), rootAlbum(new Album("Root")) {}

User::~User()
{
    delete rootAlbum;
}

QString User::getName() const { return name; }

void User::setRootAlbum(Album* album) { rootAlbum = album; }

Album* User::getRootAlbum() const { return rootAlbum; }

void User::saveToJson(const QString& filePath) const {
    QJsonObject userObj;

    // If the file already exists, try to preserve extra keys (e.g. "favorites")
    QFile inFile(filePath);
    if (inFile.exists() && inFile.open(QIODevice::ReadOnly)) {
        QJsonDocument existingDoc = QJsonDocument::fromJson(inFile.readAll());
        if (existingDoc.isObject()) {
            userObj = existingDoc.object();
        }
        inFile.close();
    }

    // Update mandatory fields
    userObj["name"] = name;
    userObj["rootAlbum"] = rootAlbum->toJson();

    QJsonDocument doc(userObj);
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(doc.toJson());
        file.close();
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
