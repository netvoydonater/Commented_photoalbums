#include "photomanager.h"
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QFile>

void PhotoManager::loadPhotos(Album* album, const QString& dirPath) {
    QDir dir(dirPath);
    for (const auto& fileInfo : dir.entryInfoList(QDir::Files)) {
        album->addPhoto(new Photo(fileInfo.absoluteFilePath(), "", QDateTime::currentDateTime()));
    }
    for (const auto& subDir : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        Album* sub = new Album(subDir);
        loadPhotos(sub, dirPath + "/" + subDir);
        album->addSubAlbum(sub);
    }
}

void PhotoManager::exportAlbum(const Album* album, const QString& dirPath) {
    QDir().mkpath(dirPath);
    for (auto* photo : album->getPhotos()) {
        QFile::copy(photo->getFilePath(), dirPath + "/" + QFileInfo(photo->getFilePath()).fileName());
    }
    QFile metaFile(dirPath + "/meta.json");
    if (metaFile.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(album->toJson());
        metaFile.write(doc.toJson());
    }
    for (auto* sub : album->getSubAlbums()) {
        exportAlbum(sub, dirPath + "/" + sub->getName());
    }
}

QList<Photo*> PhotoManager::searchPhotos(const Album* album, SearchStrategy* strategy, const QString& param) const {
    QList<Photo*> allPhotos = getAllPhotos(album);
    return strategy->search(allPhotos, param);
}

QList<Photo*> PhotoManager::getAllPhotos(const Album* album) const {
    QList<Photo*> list;
    collectPhotos(album, list);
    // Сортировка по дате (хронологический порядок, от нового к старому)
    std::sort(list.begin(), list.end(), [](Photo* a, Photo* b) {
        return a->getDate() > b->getDate();
    });
    return list;
}

void PhotoManager::collectPhotos(const Album* album, QList<Photo*>& list) const {
    list.append(album->getPhotos());
    for (auto* sub : album->getSubAlbums()) {
        collectPhotos(sub, list);
    }
}
