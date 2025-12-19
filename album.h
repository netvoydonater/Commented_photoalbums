#ifndef ALBUM_H
#define ALBUM_H

#include <QString>
#include <QList>
#include <QString>
#include <QDateTime>
#include "photo.h"
#include "tag.h"

// Composite pattern: Album может содержать фото или под-альбомы
class Album
{
public:
    Album(const QString &name);
    QString getName() const;
    void addPhoto(Photo *photo);
    void addSubAlbum(Album *album);
    void removePhoto(Photo *photo);
    void removeSubAlbum(Album *album);
    QList<Photo *> getPhotos() const;
    QList<Album *> getSubAlbums() const;
    void addTag(const Tag &tag);
    QList<Tag> getTags() const;
    QDateTime getEarliestDate() const;
    int getYear() const;
    QJsonObject toJson() const;
    static Album *fromJson(const QJsonObject &obj);

    // Упрощённый сеттер имени альбома (для переименования из UI)
    void setName(const QString &newName) { name = newName; }

private:
    QString name;
    QList<Photo *> photos;
    QList<Album *> subAlbums;
    QList<Tag> tags;
};

Q_DECLARE_METATYPE(Album *)

#endif // ALBUM_H
