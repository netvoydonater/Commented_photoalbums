#ifndef TAG_H
#define TAG_H

#include <QString>
#include <QJsonObject>

class Tag {
public:
    Tag(const QString& name);
    QString getName() const;
    QJsonObject toJson() const;
    static Tag fromJson(const QJsonObject& obj);

private:
    QString name;
};

#endif // TAG_H
