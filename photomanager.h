#ifndef PHOTOMANAGER_H
#define PHOTOMANAGER_H

#include <QList>
#include "photo.h"
#include "album.h"
#include "searchstrategy.h"

class PhotoManager {
public:
    void loadPhotos(Album* album, const QString& dirPath); // Импорт
    void exportAlbum(const Album* album, const QString& dirPath); // Экспорт со структурой
    QList<Photo*> searchPhotos(const Album* album, SearchStrategy* strategy, const QString& param) const;
    QList<Photo*> getAllPhotos(const Album* album) const; // Рекурсивно для хронологии

private:
    void collectPhotos(const Album* album, QList<Photo*>& list) const;
};

#endif // PHOTOMANAGER_H
