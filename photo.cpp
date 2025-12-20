#include "photo.h"
#include <QImage>
#include <QJsonObject>
#include <QJsonArray>

Photo::Photo(const QString &filePath, const QString &description, const QDateTime &date)
    : filePath(filePath), description(description), date(date) {}

QString Photo::getFilePath() const { return filePath; }

QString Photo::getDescription() const { return description; }

QDateTime Photo::getDate() const { return date; }

QList<Tag> Photo::getTags() const { return tags; }

void Photo::addTag(const Tag &tag) { tags.append(tag); }

void Photo::removeTag(const QString &name)
{
    for (int i = tags.size() - 1; i >= 0; --i)
    {
        if (tags[i].getName() == name)
        {
            tags.removeAt(i);
        }
    }
}

void Photo::setDescription(const QString &desc) { description = desc; }

void Photo::setDate(const QDateTime &date) { this->date = date; }

void Photo::setFilePath(const QString &path) { filePath = path; } // Реализация setter

void Photo::crop(const QRect &rect)
{
    QImage image(filePath);
    QImage cropped = image.copy(rect);
    cropped.save(filePath); // Перезаписываем файл
}

QJsonObject Photo::toJson() const
{
    QJsonObject obj;
    obj["filePath"] = filePath;
    obj["description"] = description;
    obj["date"] = date.toString(Qt::ISODate);
    QJsonArray tagsArray;
    for (const auto &tag : tags)
    {
        tagsArray.append(tag.toJson());
    }
    obj["tags"] = tagsArray;
    return obj;
}

Photo *Photo::fromJson(const QJsonObject &obj)
{
    Photo *photo = new Photo(obj["filePath"].toString(), obj["description"].toString(),
                             QDateTime::fromString(obj["date"].toString(), Qt::ISODate));
    QJsonArray tagsArray = obj["tags"].toArray();
    for (const auto &tagVal : tagsArray)
    {
        photo->addTag(Tag::fromJson(tagVal.toObject()));
    }
    return photo;
}
