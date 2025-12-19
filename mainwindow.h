#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTreeWidget>
#include <QTabWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QGridLayout>
#include <QListWidget>
#include <QToolButton>
#include <QCalendarWidget>
#include <QMenu>
#include <QStackedWidget>
#include <QVBoxLayout>
#include "user.h"
#include "photomanager.h"
#include "searchstrategy.h"
#include <QLabel>
#include <QMouseEvent>

// Класс для кликабельных виджетов с фото
class ClickablePhotoWidget : public QWidget
{
    Q_OBJECT
public:
    ClickablePhotoWidget(Photo *photo, QWidget *parent = nullptr)
        : QWidget(parent), m_photo(photo) {}

    Photo *getPhoto() const { return m_photo; }

signals:
    void clicked(Photo *photo);
    void doubleClicked(Photo *photo);
    void rightClicked(Photo *photo, const QPoint &pos);

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton)
        {
            emit clicked(m_photo);
        }
        else if (event->button() == Qt::RightButton)
        {
            emit rightClicked(m_photo, event->globalPos());
        }
        QWidget::mousePressEvent(event);
    }

    void mouseDoubleClickEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton)
        {
            emit doubleClicked(m_photo);
        }
        QWidget::mouseDoubleClickEvent(event);
    }

private:
    Photo *m_photo;
};

// Класс для кликабельных виджетов с альбомами
class ClickableAlbumWidget : public QWidget
{
    Q_OBJECT
public:
    ClickableAlbumWidget(Album *album, QWidget *parent = nullptr)
        : QWidget(parent), m_album(album) {}

    Album *getAlbum() const { return m_album; }

signals:
    void clicked(Album *album);
    void doubleClicked(Album *album);
    void rightClicked(Album *album, const QPoint &pos);

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton)
        {
            emit clicked(m_album);
        }
        else if (event->button() == Qt::RightButton)
        {
            emit rightClicked(m_album, event->globalPos());
        }
        QWidget::mousePressEvent(event);
    }

    void mouseDoubleClickEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton)
        {
            emit doubleClicked(m_album);
        }
        QWidget::mouseDoubleClickEvent(event);
    }

private:
    Album *m_album;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void addPhoto();
    void createAlbum();
    void createFolder();
    void search();
    void switchUser();
    void deleteItem();
    void editPhoto();
    void showFullScreen(Photo *photo);
    void exportTo();
    void updateCenterPanel();
    void populateTree(Album *album, QTreeWidgetItem *parentItem = nullptr);
    void showSearchParams();
    void addToFavorites(Photo *photo);
    void populateFavorites();
    void populateTags();
    void populateAllPhotos();
    void populateRecent();
    void handleRightClick(const QPoint &pos);
    void onLeftPanelItemClicked(QListWidgetItem *item);
    void onAlbumTreeItemClicked(QTreeWidgetItem *item, int column);
    void onAlbumTreeItemDoubleClicked(QTreeWidgetItem *item, int column);
    void onAlbumTreeContextMenu(const QPoint &pos);
    void dropEvent(QDropEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;

private:
    User *currentUser;
    PhotoManager manager;

    // Левая панель навигации
    QListWidget *navigationList; // Список разделов (Все фото, Избранное и т.д.)
    QTreeWidget *albumsTree;     // Дерево альбомов
    QWidget *leftPanel;
    QStackedWidget *leftStack;

    // Центральная панель
    QTabWidget *tabWidget; // Вкладки (Лента, Сетка, Альбомы)

    // Верхняя панель
    QLineEdit *searchBar;      // Поиск
    QToolButton *paramsButton; // Параметры поиска
    QPushButton *addButton;    // +
    QLabel *userLabel;         // Информация о пользователе
    QLabel *avatarLabel;       // Аватар (первая буква имени)

    // Поиск
    QList<Photo *> searchResults;
    bool inSearchMode = false;

    // Правая панель
    QWidget *propertiesPanel; // Свойства

    SearchStrategy *currentStrategy;
    Album *selectedAlbum;
    Photo *selectedPhoto;
    QList<Photo *> favorites; // Для избранного

    enum NavigationSection
    {
        AllPhotos,
        Favorites,
        Recent,
        Tags,
        Albums
    };
    NavigationSection currentSection;

    void setupUI();
    void applyStyleSheet();
    void showEmptyState();
    void renderFeed(QWidget *container);
    void renderGrid(QWidget *container);
    void renderAlbums(QWidget *container);
    void renderAlbumView(QWidget *container, Album *album);
    Album *getOrCreateYearAlbum(int year);
    void rebuildAlbumsTree();
    QWidget *createPhotoCard(Photo *photo, QWidget *parent);
    QWidget *createAlbumCard(Album *album, QWidget *parent);
    void updatePropertiesPanel();

    // Удаляет указанный альбом из родительской структуры (рекурсивно). Возвращает true при удалении.
    bool removeAlbumFromParent(Album *target);

protected:
    void keyPressEvent(QKeyEvent *event) override;
};

#endif // MAINWINDOW_H
