#ifndef PHOTOEDITDIALOG_H
#define PHOTOEDITDIALOG_H

#include <QDialog>
#include <QGraphicsView>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QLineEdit>
#include "photo.h"

class PhotoEditDialog : public QDialog {
    Q_OBJECT
public:
    PhotoEditDialog(Photo* photo, QWidget* parent = nullptr);
    ~PhotoEditDialog();

signals:
    // Сообщаем главному окну, что пользователь хочет добавить фото в избранное
    void addToFavoritesRequested(Photo* photo);

private slots:
    void applyCrop();
    void saveChanges();
    void addToFavorites();
    void deletePhoto();

private:
    Photo* photo;
    QGraphicsView* view;
    QGraphicsScene* scene;
    QRect cropRect;

    QLabel* previewLabel;
    QLabel* infoLabel;
    QTextEdit* commentEdit;
    QLineEdit* tagEdit;

    // Для динамической обрезки
    QPixmap originalPixmap;
    QRect selectionRect;
    bool selecting = false;
    QPoint selectionOrigin;
    class QRubberBand* rubberBand = nullptr;

    void setupUI();
    void applyStyles();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
};

#endif // PHOTOEDITDIALOG_H
