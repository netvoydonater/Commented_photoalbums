#ifndef PHOTO_H
#define PHOTO_H

#include <QString>
#include <QDateTime>
#include <QList>
#include "tag.h"

class Photo
{
public:
    Photo(const QString &filePath, const QString &description, const QDateTime &date);
    QString getFilePath() const;
    QString getDescription() const;
    QDateTime getDate() const;
    QList<Tag> getTags() const;
    void addTag(const Tag &tag);
    void removeTag(const QString &name);
    void setDescription(const QString &desc);
    void setDate(const QDateTime &date);
    void setFilePath(const QString &path); // Добавленный setter
    void crop(const QRect &rect);          // Простая обрезка (использует QImage)
    QJsonObject toJson() const;
    static Photo *fromJson(const QJsonObject &obj);

private:
    QString filePath;
    QString description;
    QDateTime date;
    QList<Tag> tags;
};

Q_DECLARE_METATYPE(Photo *)

#endif // PHOTO_H
