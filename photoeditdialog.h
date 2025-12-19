#ifndef PHOTOEDITDIALOG_H
#define PHOTOEDITDIALOG_H

#include <QDialog>
#include <QGraphicsView>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QLineEdit>
#include "photo.h"

class Album;     // forward
class QComboBox; // forward

class PhotoEditDialog : public QDialog
{
    Q_OBJECT
public:
    // rootAlbum — корневой альбом (локальное хранилище), нужен чтобы перемещать фото между альбомами
    PhotoEditDialog(Photo *photo, Album *rootAlbum, QWidget *parent = nullptr);
    ~PhotoEditDialog();

signals:
    // Сообщаем главному окну, что пользователь хочет добавить фото в избранное
    void addToFavoritesRequested(Photo *photo);

private slots:
    void applyCrop();
    void saveChanges();
    void addToFavorites();

private:
    Photo *photo;
    QGraphicsView *view;
    QGraphicsScene *scene;
    QRect cropRect;

    QLabel *previewLabel;
    QLabel *infoLabel;
    QTextEdit *commentEdit;
    QLineEdit *tagEdit;
    QComboBox *moveCombo; // выбор альбома для перемещения
    Album *rootAlbum = nullptr;

    // Для динамической обрезки
    QPixmap originalPixmap;
    QRect selectionRect;
    bool selecting = false;
    QPoint selectionOrigin;
    class QRubberBand *rubberBand = nullptr;

    void setupUI();
    void applyStyles();
    void collectAlbums(Album *album, QList<Album *> &list);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
};

#endif // PHOTOEDITDIALOG_H
