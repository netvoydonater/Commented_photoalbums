#ifndef USER_H
#define USER_H

#include <QString>
#include <QList>
#include "album.h"

class User {
public:
    User(const QString& name);
    QString getName() const;
    void setRootAlbum(Album* album);
    Album* getRootAlbum() const;
    void saveToJson(const QString& filePath) const;
    static User* loadFromJson(const QString& filePath);

private:
    QString name;
    Album* rootAlbum; // Корневой альбом для Composite
};

#endif // USER_H
