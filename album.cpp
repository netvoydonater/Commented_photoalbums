#include "album.h"
#include <QJsonObject>
#include <QJsonArray>

Album::Album(const QString& name) : name(name) {}

QString Album::getName() const { return name; }

void Album::addPhoto(Photo* photo) { photos.append(photo); }

void Album::addSubAlbum(Album* album) { subAlbums.append(album); }

void Album::removePhoto(Photo* photo) { photos.removeOne(photo); }

void Album::removeSubAlbum(Album* album) { subAlbums.removeOne(album); }

QList<Photo*> Album::getPhotos() const { return photos; }

QList<Album*> Album::getSubAlbums() const { return subAlbums; }

void Album::addTag(const Tag& tag) { tags.append(tag); }

QList<Tag> Album::getTags() const { return tags; }

QDateTime Album::getEarliestDate() const {
    QDateTime minDate = QDateTime::currentDateTime().addYears(1);
    bool hasPhotos = false;
    for (Photo* p : photos) {
        minDate = qMin(minDate, p->getDate());
        hasPhotos = true;
    }
    for (Album* sub : subAlbums) {
        QDateTime subMin = sub->getEarliestDate();
        if (subMin.isValid()) {
            minDate = qMin(minDate, subMin);
            hasPhotos = true;
        }
    }
    return hasPhotos ? minDate : QDateTime();
}

int Album::getYear() const {
    QDateTime d = getEarliestDate();
    return d.isValid() ? d.date().year() : QDateTime::currentDateTime().date().year();
}

QJsonObject Album::toJson() const {
    QJsonObject obj;
    obj["name"] = name;
    QJsonArray photosArray;
    for (const auto* photo : photos) {
        photosArray.append(photo->toJson());
    }
    obj["photos"] = photosArray;
    QJsonArray subAlbumsArray;
    for (const auto* sub : subAlbums) {
        subAlbumsArray.append(sub->toJson());
    }
    obj["subAlbums"] = subAlbumsArray;
    QJsonArray tagsArray;
    for (const auto& tag : tags) {
        tagsArray.append(tag.toJson());
    }
    obj["tags"] = tagsArray;
    return obj;
}

Album* Album::fromJson(const QJsonObject& obj) {
    Album* album = new Album(obj["name"].toString());
    QJsonArray photosArray = obj["photos"].toArray();
    for (const auto& photoVal : photosArray) {
        album->addPhoto(Photo::fromJson(photoVal.toObject()));
    }
    QJsonArray subAlbumsArray = obj["subAlbums"].toArray();
    for (const auto& subVal : subAlbumsArray) {  // Используем const auto&
        album->addSubAlbum(Album::fromJson(subVal.toObject()));
    }
    QJsonArray tagsArray = obj["tags"].toArray();
    for (const auto& tagVal : tagsArray) {
        album->addTag(Tag::fromJson(tagVal.toObject()));
    }
    return album;
}

Album::~Album()
{
    for (Photo* p : photos) {
        delete p;
    }
    photos.clear();
    for (Album* a : subAlbums) {
        delete a;
    }
    subAlbums.clear();
}
