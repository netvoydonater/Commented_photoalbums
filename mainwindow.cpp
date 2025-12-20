#include "mainwindow.h"
#include "photoeditdialog.h"
#include "logindialog.h"
#include "bytagstrategy.h"
#include "bydatestrategy.h"
#include "bydescriptionstrategy.h"
#include <QMenu>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QGraphicsView>
#include <QGuiApplication>
#include <QScreen>
#include <QDate>
#include <QInputDialog>
#include <QMenuBar>
#include <QStringList>
#include <QComboBox>
#include <QSet>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QUrl>
#include <QFileInfo>
#include <QFile>
#include <QCursor>
#include <QDialog>
#include <QPushButton>
#include <QDragLeaveEvent>
#include <QKeyEvent>
#include <algorithm>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFileInfo>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      currentUser(nullptr),
      currentStrategy(new ByTagStrategy()),
      selectedAlbum(nullptr),
      selectedPhoto(nullptr),
      currentSection(AllPhotos)
{
    LoginDialog login(this);
    if (login.exec() == QDialog::Accepted)
    {
        currentUser = User::loadFromJson(login.getUserName() + ".json");
        if (!currentUser)
            currentUser = new User(login.getUserName());
    }
    else
    {
        qApp->quit();
    }
    setupUI();
    applyStyleSheet();
    // Заполняем дерево альбомов (скрываем технический корень)
    albumsTree->clear();
    for (Album *sub : currentUser->getRootAlbum()->getSubAlbums())
    {
        populateTree(sub);
    }

    // Загружаем избранное из отдельного файла
    loadFavorites();

    // Открываем приложение в полноэкранном режиме
    showMaximized();

    // Начальный экран — "Недавно добавленное"
    navigationList->setCurrentRow(2); // 0: Все фото, 1: Избранное, 2: Недавно добавленное
    populateRecent();
}

MainWindow::~MainWindow()
{
    if (currentUser)
    {
        currentUser->saveToJson(currentUser->getName() + ".json");
        delete currentUser;
        currentUser = nullptr;
    }
}

void MainWindow::setupUI()
{
    setWindowTitle("Фотоальбомы с комментариями");
    resize(1400, 900);

    // Включаем поддержку drag & drop
    setAcceptDrops(true);

    // Главный виджет
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainVerticalLayout = new QVBoxLayout(centralWidget);
    mainVerticalLayout->setContentsMargins(0, 0, 0, 0);
    mainVerticalLayout->setSpacing(0);

    // === ВЕРХНЯЯ ПАНЕЛЬ (Шапка) ===
    QWidget *headerWidget = new QWidget(this);
    headerWidget->setObjectName("header");
    headerWidget->setMinimumHeight(60);
    QHBoxLayout *headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(20, 10, 20, 10);

    // Информация о пользователе с аватаром
    QWidget *userWidget = new QWidget(this);
    QHBoxLayout *userLayout = new QHBoxLayout(userWidget);
    userLayout->setContentsMargins(0, 0, 0, 0);
    userLayout->setSpacing(10);

    // Аватар (первая буква имени)
    avatarLabel = new QLabel(this);
    avatarLabel->setObjectName("avatarLabel");
    avatarLabel->setFixedSize(45, 45);
    avatarLabel->setAlignment(Qt::AlignCenter);
    QString firstLetter = currentUser->getName().left(1).toUpper();
    avatarLabel->setText(firstLetter);
    userLayout->addWidget(avatarLabel);

    // Информация о пользователе
    userLabel = new QLabel(this);
    userLabel->setObjectName("userLabel");
    int photoCount = manager.getAllPhotos(currentUser->getRootAlbum()).size();
    userLabel->setText(currentUser->getName() + "\nЛокальное хранилище: " +
                       QString::number(photoCount) + " фото");
    userLayout->addWidget(userLabel);

    // Поисковая строка
    searchBar = new QLineEdit(this);
    searchBar->setObjectName("searchBar");
    searchBar->setPlaceholderText("🔎 Поиск фотографий...");
    searchBar->setMinimumWidth(400);
    connect(searchBar, &QLineEdit::returnPressed, this, &MainWindow::search);

    // Кнопка параметров поиска
    paramsButton = new QToolButton(this);
    paramsButton->setObjectName("paramsButton");
    paramsButton->setIcon(QIcon::fromTheme("configure"));
    paramsButton->setText("🎚");
    paramsButton->setToolTip("Параметры поиска");
    connect(paramsButton, &QToolButton::clicked, this, &MainWindow::showSearchParams);

    // Кнопка добавления
    addButton = new QPushButton("+", this);
    addButton->setObjectName("addButton");
    addButton->setFixedSize(50, 50);
    addButton->setToolTip("Добавить");

    QMenu *addMenu = new QMenu(this);
    addMenu->setObjectName("addMenu");

    QAction *importAction = addMenu->addAction("Импортировать фото...");
    connect(importAction, &QAction::triggered, this, &MainWindow::addPhoto);

    QAction *albumAction = addMenu->addAction("Создать альбом...");
    connect(albumAction, &QAction::triggered, this, &MainWindow::createAlbum);

    QAction *folderAction = addMenu->addAction("Создать папку...");
    connect(folderAction, &QAction::triggered, this, &MainWindow::createFolder);

    addButton->setMenu(addMenu);

    headerLayout->addWidget(userWidget);
    headerLayout->addStretch();
    headerLayout->addWidget(searchBar);
    headerLayout->addWidget(paramsButton);
    headerLayout->addWidget(addButton);

    mainVerticalLayout->addWidget(headerWidget);

    // === ОСНОВНАЯ ЧАСТЬ (Три панели) ===
    QWidget *contentWidget = new QWidget(this);
    QHBoxLayout *contentLayout = new QHBoxLayout(contentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);

    // --- ЛЕВАЯ ПАНЕЛЬ НАВИГАЦИИ ---
    leftPanel = new QWidget(this);
    leftPanel->setObjectName("leftPanel");
    leftPanel->setMinimumWidth(240);
    leftPanel->setMaximumWidth(240);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(10, 10, 10, 10);

    // Список навигации
    navigationList = new QListWidget(this);
    navigationList->setObjectName("navigationList");

    // Добавляем пункты навигации
    QListWidgetItem *allPhotosItem = new QListWidgetItem("📷 Все фотографии");
    allPhotosItem->setData(Qt::UserRole, AllPhotos);
    navigationList->addItem(allPhotosItem);

    QListWidgetItem *favoritesItem = new QListWidgetItem("⭐ Избранное");
    favoritesItem->setData(Qt::UserRole, Favorites);
    navigationList->addItem(favoritesItem);

    QListWidgetItem *recentItem = new QListWidgetItem("🕐 Недавно добавленное");
    recentItem->setData(Qt::UserRole, Recent);
    navigationList->addItem(recentItem);

    QListWidgetItem *tagsItem = new QListWidgetItem("📍 Теги");
    tagsItem->setData(Qt::UserRole, Tags);
    navigationList->addItem(tagsItem);

    connect(navigationList, &QListWidget::itemClicked, this, &MainWindow::onLeftPanelItemClicked);

    leftLayout->addWidget(navigationList);

    // Дерево альбомов
    QListWidgetItem *albumsItem = new QListWidgetItem("📁 Альбомы");
    albumsItem->setData(Qt::UserRole, Albums);
    navigationList->addItem(albumsItem);

    albumsTree = new QTreeWidget(this);
    albumsTree->setObjectName("albumsTree");
    albumsTree->setHeaderHidden(true);
    albumsTree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(albumsTree, &QTreeWidget::itemClicked, this, &MainWindow::onAlbumTreeItemClicked);
    connect(albumsTree, &QTreeWidget::itemDoubleClicked, this, &MainWindow::onAlbumTreeItemDoubleClicked);
    connect(albumsTree, &QTreeWidget::customContextMenuRequested, this, &MainWindow::onAlbumTreeContextMenu);

    leftLayout->addWidget(albumsTree);

    // Копирайт внизу
    QLabel *copyrightLabel = new QLabel("© Все права защищены\n2025 год", this);
    copyrightLabel->setObjectName("copyrightLabel");
    copyrightLabel->setAlignment(Qt::AlignCenter);
    leftLayout->addWidget(copyrightLabel);

    contentLayout->addWidget(leftPanel);

    // --- ЦЕНТРАЛЬНАЯ ПАНЕЛЬ (Вкладки) ---
    QWidget *centerPanel = new QWidget(this);
    centerPanel->setObjectName("centerPanel");
    QVBoxLayout *centerLayout = new QVBoxLayout(centerPanel);
    centerLayout->setContentsMargins(20, 10, 20, 10);

    tabWidget = new QTabWidget(this);
    tabWidget->setObjectName("tabWidget");

    // Создаем вкладки (только Лента и Сетка)
    QWidget *feedTab = new QWidget();
    feedTab->setObjectName("feedTab");
    tabWidget->addTab(feedTab, "Лента");

    QWidget *gridTab = new QWidget();
    gridTab->setObjectName("gridTab");
    tabWidget->addTab(gridTab, "Сетка");

    connect(tabWidget, &QTabWidget::currentChanged, this, &MainWindow::updateCenterPanel);

    centerLayout->addWidget(tabWidget);
    contentLayout->addWidget(centerPanel, 1);

    // --- ПРАВАЯ ПАНЕЛЬ (Свойства) ---
    propertiesPanel = new QWidget(this);
    propertiesPanel->setObjectName("propertiesPanel");
    propertiesPanel->setMinimumWidth(300);
    propertiesPanel->setMaximumWidth(300);
    QVBoxLayout *propsLayout = new QVBoxLayout(propertiesPanel);
    propsLayout->setContentsMargins(10, 10, 10, 10);

    QLabel *propsPlaceholder = new QLabel("Откройте любой элемент, чтобы\nпосмотреть сведения", this);
    propsPlaceholder->setAlignment(Qt::AlignCenter);
    propsPlaceholder->setWordWrap(true);
    propsLayout->addWidget(propsPlaceholder);
    propsLayout->addStretch();

    contentLayout->addWidget(propertiesPanel);

    mainVerticalLayout->addWidget(contentWidget, 1);

    setCentralWidget(centralWidget);

    // Меню
    QMenuBar *menuBar = new QMenuBar(this);
    QMenu *userMenu = menuBar->addMenu("Пользователь");
    userMenu->addAction("Сменить пользователя", this, &MainWindow::switchUser);
    setMenuBar(menuBar);
}

void MainWindow::populateTree(Album *album, QTreeWidgetItem *parentItem)
{
    QTreeWidgetItem *item = parentItem ? new QTreeWidgetItem(parentItem) : new QTreeWidgetItem(albumsTree);
    item->setText(0, album->getName());
    item->setData(0, Qt::UserRole, QVariant::fromValue(album));
    for (const auto *sub : album->getSubAlbums())
    {
        populateTree(const_cast<Album *>(sub), item);
    }
}

void MainWindow::addPhoto()
{
    QStringList files = QFileDialog::getOpenFileNames(
        this,
        "Импортировать фото",
        "",
        "Изображения (*.png *.jpg *.jpeg *.bmp *.gif *.webp);;Все файлы (*.*)");

    if (!files.isEmpty())
    {
        Album *targetAlbum = selectedAlbum ? selectedAlbum : currentUser->getRootAlbum();

        for (const QString &file : files)
        {
            Photo *newPhoto = new Photo(file, "", QDateTime::currentDateTime());
            targetAlbum->addPhoto(newPhoto);
        }

        // Обновляем интерфейс
        updateCenterPanel();
        updatePropertiesPanel();

        // Обновляем счётчик фото
        int photoCount = manager.getAllPhotos(currentUser->getRootAlbum()).size();

        userLabel->setText(currentUser->getName() + "\nЛокальное хранилище: " +
                           QString::number(photoCount) + " фото");

        // Уведомление
        QString message = files.size() == 1
                              ? "Добавлено 1 фото"
                              : "Добавлено " + QString::number(files.size()) + " фото";
        QMessageBox::information(this, "Импорт фото", message);
    }
}

void MainWindow::createAlbum()
{
    bool ok;
    QString name = QInputDialog::getText(
        this, "Новый альбом", "Название:", QLineEdit::Normal, "", &ok);

    if (!ok || name.trimmed().isEmpty())
        return;

    // Создаём альбом в корне (год — техническая метка, не место для добавления)
    Album *root = currentUser->getRootAlbum();
    root->addSubAlbum(new Album(name));

    rebuildAlbumsTree();
    updateCenterPanel();
}

void MainWindow::createFolder()
{
    createAlbum(); // Папка - то же, что альбом
}

void MainWindow::search()
{
    QString query = searchBar->text().trimmed();
    if (query.isEmpty())
        return;

    // Сброс предыдущего режима поиска
    searchResults.clear();
    inSearchMode = false;

    // Разбираем запрос: имя (подстрока), опционально дата yyyy-MM-dd и/или тег (tag:имя или #имя)
    QStringList tokens = query.split(QRegExp("\\s+"), QString::SkipEmptyParts);
    QString namePart;
    QDate datePart;
    QString tagPart;
    for (const QString &t : tokens)
    {
        if (QDate::fromString(t, "yyyy-MM-dd").isValid())
        {
            datePart = QDate::fromString(t, "yyyy-MM-dd");
        }
        else if (t.startsWith("tag:", Qt::CaseInsensitive))
        {
            tagPart = t.mid(4).trimmed();
        }
        else if (t.startsWith('#'))
        {
            tagPart = t.mid(1).trimmed();
        }
        else if (namePart.isEmpty())
        {
            namePart = t;
        }
        else if (tagPart.isEmpty())
        {
            tagPart = t;
        }
    }

    QList<Photo *> all = manager.getAllPhotos(currentUser->getRootAlbum());
    for (Photo *p : all)
    {
        bool ok = true;
        if (!namePart.isEmpty())
        {
            if (!QFileInfo(p->getFilePath()).fileName().contains(namePart, Qt::CaseInsensitive))
                ok = false;
        }
        if (ok && datePart.isValid())
        {
            if (p->getDate().date() != datePart)
                ok = false;
        }
        if (ok && !tagPart.isEmpty())
        {
            bool has = false;
            for (const Tag &tg : p->getTags())
            {
                if (tg.getName().contains(tagPart, Qt::CaseInsensitive))
                {
                    has = true;
                    break;
                }
            }
            if (!has)
                ok = false;
        }
        if (ok)
            searchResults.append(p);
    }

    if (searchResults.isEmpty())
    {
        QMessageBox::information(this, "Поиск", "Элемент не найден");
        return;
    }

    inSearchMode = true;
    currentSection = AllPhotos;
    selectedAlbum = nullptr;
    updateCenterPanel();
}

void MainWindow::switchUser()
{
    currentUser->saveToJson(currentUser->getName() + ".json");
    LoginDialog login(this);
    if (login.exec() == QDialog::Accepted)
    {
        // Close any fullscreen viewer before switching user to avoid dangling photo pointers
        if (fullScreenDialog)
        {
            fullScreenDialog->close();
            fullScreenDialog = nullptr;
            fullScreenImageLabel = nullptr;
            fullScreenPhotos.clear();
            fullScreenIndex = -1;
        }

        delete currentUser;
        currentUser = User::loadFromJson(login.getUserName() + ".json");
        if (!currentUser)
            currentUser = new User(login.getUserName());

        // Сброс состояния предыдущего пользователя
        selectedAlbum = nullptr;
        selectedPhoto = nullptr;
        favorites.clear();
        searchResults.clear();
        inSearchMode = false;

        // Перестраиваем дерево альбомов без технического корня
        albumsTree->clear();
        for (Album *sub : currentUser->getRootAlbum()->getSubAlbums())
        {
            populateTree(sub);
        }

        // Обновляем шапку (аватар и счётчик)
        QString firstLetter = currentUser->getName().left(1).toUpper();
        if (avatarLabel)
            avatarLabel->setText(firstLetter);
        int photoCount = manager.getAllPhotos(currentUser->getRootAlbum()).size();
        userLabel->setText(currentUser->getName() + "\nЛокальное хранилище: " +
                           QString::number(photoCount) + " фото");

        updateCenterPanel();
    }
}

void MainWindow::deleteItem()
{
    QString name;
    if (selectedPhoto)
    {
        name = QFileInfo(selectedPhoto->getFilePath()).fileName();
    }
    else if (selectedAlbum)
    {
        name = selectedAlbum->getName();
    }
    else
    {
        return;
    }

    // Системное диалоговое окно подтверждения
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Удаление",
        "Удалить " + name + "?",
        QMessageBox::Yes | QMessageBox::No);

    Photo *toDelPhoto = nullptr;

    if (reply == QMessageBox::Yes)
    {
        if (selectedPhoto)
        {
            // Находим альбом, в котором находится это фото
            Album *albumWithPhoto = nullptr;
            QList<Album *> stack;
            stack.append(currentUser->getRootAlbum());
            while (!stack.isEmpty() && !albumWithPhoto)
            {
                Album *a = stack.takeLast();
                if (a->getPhotos().contains(selectedPhoto))
                {
                    albumWithPhoto = a;
                    break;
                }
                for (Album *sub : a->getSubAlbums())
                {
                    stack.append(sub);
                }
            }

            if (albumWithPhoto)
            {
                albumWithPhoto->removePhoto(selectedPhoto);
            }
            // Отложим удаление объекта до обновления UI
            toDelPhoto = selectedPhoto;
            selectedPhoto = nullptr;
        }
        else if (selectedAlbum)
        {
            // Удаляем альбом рекурсивно из родителя — удаляем объект после обновления UI
            Album *toDelAlbum = selectedAlbum;
            if (removeAlbumFromParent(toDelAlbum))
            {
                // Удаляем ссылки на фото из других коллекций перед удалением объекта
                QList<Photo *> photosInAlbum = manager.getAllPhotos(toDelAlbum);
                removePhotosReferencesFromList(photosInAlbum);

                selectedAlbum = nullptr;
                rebuildAlbumsTree();
                updateCenterPanel();
                delete toDelAlbum;

                // Сохраним состояние пользователя сразу после удаления
                currentUser->saveToJson(currentUser->getName() + ".json");
            }
        }
        // Перестраиваем дерево альбомов без технического корня и обновляем UI
        albumsTree->clear();
        for (Album *sub : currentUser->getRootAlbum()->getSubAlbums())
        {
            populateTree(sub);
        }
        updateCenterPanel();

        // Теперь можно удалить объект фото (если был)
        if (toDelPhoto)
        {
            // Удаляем ссылки на фото из коллекций
            removePhotoReferences(toDelPhoto);
            delete toDelPhoto;
            toDelPhoto = nullptr;

            // Сохраним состояние пользователя
            currentUser->saveToJson(currentUser->getName() + ".json");
        }
    }
}

void MainWindow::editPhoto()
{
    if (!selectedPhoto)
        return;
    PhotoEditDialog edit(selectedPhoto, currentUser->getRootAlbum(), this);
    QObject::connect(&edit, &PhotoEditDialog::addToFavoritesRequested,
                     this, &MainWindow::addToFavorites);
    edit.exec();
    // После возможного перемещения/переименования обновляем интерфейс
    updateCenterPanel();
    updatePropertiesPanel();
}

void MainWindow::showFullScreen(Photo *photo)
{
    if (!photo)
        return;

    // Собираем текущий список фотографий для навигации
    fullScreenPhotos.clear();
    if (inSearchMode)
        fullScreenPhotos = searchResults;
    else if (currentSection == Favorites)
        fullScreenPhotos = favorites;
    else if (currentSection == Recent)
    {
        QList<Photo *> all = manager.getAllPhotos(currentUser->getRootAlbum());
        std::sort(all.begin(), all.end(), [](Photo *a, Photo *b)
                  { return a->getDate() > b->getDate(); });
        int count = qMin(20, all.size());
        for (int i = 0; i < count; ++i)
            fullScreenPhotos.append(all[i]);
    }
    else
    {
        Album *album = selectedAlbum ? selectedAlbum : currentUser->getRootAlbum();
        fullScreenPhotos = manager.getAllPhotos(album);
    }

    // Найдём индекс текущего фото
    fullScreenIndex = fullScreenPhotos.indexOf(photo);
    if (fullScreenIndex < 0)
        fullScreenIndex = 0;

    // Создаём диалог (и пересоздаём при повторном открытии)
    if (fullScreenDialog)
    {
        fullScreenDialog->close();
        delete fullScreenDialog;
        fullScreenDialog = nullptr;
    }

    fullScreenDialog = new QDialog(this);
    fullScreenDialog->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    fullScreenDialog->setAttribute(Qt::WA_DeleteOnClose);
    fullScreenDialog->setStyleSheet("background-color: black;");

    // Обнулять указатели при удалении диалога (WA_DeleteOnClose вызовет destroyed)
    connect(fullScreenDialog, &QObject::destroyed, this, [this]()
            {
        fullScreenDialog = nullptr;
        fullScreenImageLabel = nullptr;
        fullScreenPhotos.clear();
        fullScreenIndex = -1; });

    QVBoxLayout *layout = new QVBoxLayout(fullScreenDialog);
    layout->setContentsMargins(0, 0, 0, 0);

    // Top bar with close
    QWidget *topBar = new QWidget(fullScreenDialog);
    topBar->setStyleSheet("background-color: rgba(0,0,0,150);");
    QHBoxLayout *topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(10, 10, 10, 10);
    topLayout->addStretch();
    QPushButton *closeBtn = new QPushButton("✕", topBar);
    closeBtn->setFixedSize(44, 44);
    connect(closeBtn, &QPushButton::clicked, fullScreenDialog, &QDialog::close);
    topLayout->addWidget(closeBtn);
    layout->addWidget(topBar);

    // Image label (member)
    fullScreenImageLabel = new QLabel(fullScreenDialog);
    fullScreenImageLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(fullScreenImageLabel, 1);

    // Navigation arrows overlay (left/right)
    QPushButton *leftBtn = new QPushButton("◀", fullScreenDialog);
    QPushButton *rightBtn = new QPushButton("▶", fullScreenDialog);
    leftBtn->setFixedSize(64, 64);
    rightBtn->setFixedSize(64, 64);
    leftBtn->setStyleSheet("background-color: rgba(255,255,255,0.7); border-radius:32px; font-size:24px;");
    rightBtn->setStyleSheet("background-color: rgba(255,255,255,0.7); border-radius:32px; font-size:24px;");
    leftBtn->setParent(fullScreenDialog);
    rightBtn->setParent(fullScreenDialog);
    leftBtn->move(40, QGuiApplication::primaryScreen()->geometry().center().y());
    rightBtn->move(QGuiApplication::primaryScreen()->geometry().width() - 120, QGuiApplication::primaryScreen()->geometry().center().y());
    leftBtn->show();
    rightBtn->show();
    connect(leftBtn, &QPushButton::clicked, this, [this]()
            { navigateFullScreen(-1); });
    connect(rightBtn, &QPushButton::clicked, this, [this]()
            { navigateFullScreen(1); });

    // Показываем изображение
    navigateFullScreen(0);

    fullScreenDialog->showMaximized();
}

void MainWindow::onLeftPanelItemClicked(QListWidgetItem *item)
{
    currentSection = static_cast<NavigationSection>(item->data(Qt::UserRole).toInt());
    selectedAlbum = nullptr;

    switch (currentSection)
    {
    case AllPhotos:
        populateAllPhotos();
        break;
    case Favorites:
        populateFavorites();
        break;
    case Recent:
        populateRecent();
        break;
    case Tags:
        populateTags();
        break;
    case Albums:
        selectedAlbum = nullptr;
        updateCenterPanel();
        break;
    }
}

void MainWindow::onAlbumTreeItemClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    Album *album = qvariant_cast<Album *>(item->data(0, Qt::UserRole));
    if (!album)
        return;

    // Одинарный клик — просто выделение и показ свойств
    selectedAlbum = album;
    selectedPhoto = nullptr;
    currentSection = Albums;
    updatePropertiesPanel();
}

void MainWindow::onAlbumTreeItemDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    Album *album = qvariant_cast<Album *>(item->data(0, Qt::UserRole));
    if (!album)
        return;

    // Двойной клик — вход в альбом (показываем вкладки: Лента/Сетка)
    selectedAlbum = album;
    selectedPhoto = nullptr;
    currentSection = Albums;
    tabWidget->tabBar()->show();
    updateCenterPanel();
    updatePropertiesPanel();
}

void MainWindow::onAlbumTreeContextMenu(const QPoint &pos)
{
    QTreeWidgetItem *item = albumsTree->itemAt(pos);
    if (!item)
        return;
    Album *album = qvariant_cast<Album *>(item->data(0, Qt::UserRole));
    if (!album)
        return;

    QMenu menu(this);
    QAction *renameAct = menu.addAction("Переименовать");
    QAction *delAct = menu.addAction("Удалить");

    QAction *act = menu.exec(albumsTree->viewport()->mapToGlobal(pos));
    if (act == renameAct)
    {
        bool ok;
        QString name = QInputDialog::getText(this, "Переименовать альбом", "Новое имя:", QLineEdit::Normal, album->getName(), &ok);
        if (ok && !name.trimmed().isEmpty())
        {
            album->setName(name);
            rebuildAlbumsTree();
            updateCenterPanel();
        }
    }
    else if (act == delAct)
    {
        QMessageBox::StandardButton reply = QMessageBox::question(this, "Удаление", "Удалить альбом " + album->getName() + "?", QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes)
        {
            if (removeAlbumFromParent(album))
            {
                // Удаляем ссылки на фото из других коллекций перед удалением объекта
                QList<Photo *> photosInAlbum = manager.getAllPhotos(album);
                removePhotosReferencesFromList(photosInAlbum);

                // Обновляем UI прежде чем удалять объект, чтобы виджеты не ссылались на удалённый объект
                selectedAlbum = nullptr;
                rebuildAlbumsTree();
                updateCenterPanel();
                delete album;

                // Сохраняем состояние
                currentUser->saveToJson(currentUser->getName() + ".json");
            }
        }
    }
}

void MainWindow::updateCenterPanel()
{
    int index = tabWidget->currentIndex();

    bool showTabs =
        currentSection == AllPhotos ||
        currentSection == Favorites ||
        currentSection == Recent ||
        (currentSection == Albums && selectedAlbum != nullptr);

    tabWidget->tabBar()->setVisible(showTabs);

    QWidget *currentTab = tabWidget->widget(index);

    // Очистка
    if (QLayout *oldLayout = currentTab->layout())
    {
        QLayoutItem *it;
        while ((it = oldLayout->takeAt(0)))
        {
            if (it->widget())
                delete it->widget();
            delete it;
        }
        delete oldLayout;
    }

    // Если раздел — Альбомы и альбом не выбран => отображаем список альбомов (без вкладок)
    if (currentSection == Albums && !selectedAlbum)
    {
        tabWidget->tabBar()->setVisible(false);
        renderAlbums(currentTab);
        updatePropertiesPanel();
        return;
    }

    // --- ТЕГИ ---
    if (currentSection == Tags)
    {
        QVBoxLayout *layout = new QVBoxLayout(currentTab);
        layout->setContentsMargins(20, 20, 20, 20);

        QLabel *title = new QLabel("Теги", currentTab);
        title->setObjectName("sectionTitle");
        layout->addWidget(title);

        QLineEdit *tagSearch = new QLineEdit(currentTab);
        tagSearch->setPlaceholderText("Поиск по тегам…");
        layout->addWidget(tagSearch);

        QScrollArea *scroll = new QScrollArea(currentTab);
        scroll->setWidgetResizable(true);

        QWidget *content = new QWidget();
        QVBoxLayout *contentLayout = new QVBoxLayout(content);

        QSet<QString> tags;
        QList<Photo *> allPhotos = manager.getAllPhotos(currentUser->getRootAlbum());
        for (Photo *p : allPhotos)
            for (const Tag &t : p->getTags())
                tags.insert(t.getName());

        if (tags.isEmpty())
        {
            QLabel *empty = new QLabel("Теги ещё не созданы", content);
            empty->setObjectName("infoLabel");
            contentLayout->addWidget(empty);
        }

        QStringList sorted = tags.values();
        sorted.sort(Qt::CaseInsensitive);

        for (const QString &t : sorted)
        {
            QWidget *row = new QWidget(content);
            QHBoxLayout *rowLayout = new QHBoxLayout(row);
            rowLayout->setContentsMargins(0, 0, 0, 0);
            QLabel *lbl = new QLabel("🏷 " + t, row);
            lbl->setObjectName("tagLabel");
            rowLayout->addWidget(lbl);
            rowLayout->addStretch();
            QPushButton *delBtn = new QPushButton("Удалить", row);
            delBtn->setObjectName("tagDeleteButton");
            rowLayout->addWidget(delBtn);
            contentLayout->addWidget(row);

            connect(delBtn, &QPushButton::clicked, this, [this, t, allPhotos]()
                    {
                QMessageBox::StandardButton reply = QMessageBox::question(this, "Удаление тега", "Удалить тег " + t + " у всех фото?", QMessageBox::Yes | QMessageBox::No);
                if (reply == QMessageBox::Yes) {
                    for (Photo *p : allPhotos) {
                        p->removeTag(t);
                    }
                    // Обновляем UI и сохраняем
                    currentUser->saveToJson(currentUser->getName() + ".json");
                    populateTags();
                    updateCenterPanel();
                    updatePropertiesPanel();
                } });
        }

        contentLayout->addStretch();
        scroll->setWidget(content);
        layout->addWidget(scroll);

        updatePropertiesPanel();
        return;
    }

    // Если раздел Recent и в приложении нет фото — показываем приветственный экран
    if (currentSection == Recent && manager.getAllPhotos(currentUser->getRootAlbum()).isEmpty() && !inSearchMode)
    {
        showEmptyState();
        updatePropertiesPanel();
        return;
    }

    // --- ОБЫЧНЫЕ РЕЖИМЫ ---
    QVBoxLayout *layout = new QVBoxLayout(currentTab);
    layout->setContentsMargins(10, 10, 10, 10);

    if (index == 0)
        renderFeed(currentTab);
    else if (index == 1)
        renderGrid(currentTab);

    updatePropertiesPanel();
}

void MainWindow::updatePropertiesPanel()
{
    // Очистка панели свойств
    QLayout *oldLayout = propertiesPanel->layout();
    if (oldLayout)
    {
        QLayoutItem *item;
        while ((item = oldLayout->takeAt(0)) != nullptr)
        {
            if (item->widget())
            {
                delete item->widget();
            }
            delete item;
        }
        delete oldLayout;
    }

    QVBoxLayout *propLayout = new QVBoxLayout(propertiesPanel);
    propLayout->setContentsMargins(10, 10, 10, 10);

    QLabel *titleLabel = new QLabel("Свойства", this);
    titleLabel->setObjectName("propertiesTitle");
    propLayout->addWidget(titleLabel);

    if (selectedPhoto)
    {
        // Превью фото
        QLabel *preview = new QLabel(this);
        preview->setPixmap(QPixmap(selectedPhoto->getFilePath()).scaled(280, 280, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        preview->setAlignment(Qt::AlignCenter);
        propLayout->addWidget(preview);

        // Информация о фото
        QLabel *typeLabel = new QLabel("Тип: фото", this);
        typeLabel->setObjectName("infoLabel");
        propLayout->addWidget(typeLabel);

        QLabel *nameLabel = new QLabel("Имя файла: " + QFileInfo(selectedPhoto->getFilePath()).fileName(), this);
        nameLabel->setObjectName("infoLabel");
        nameLabel->setWordWrap(true);
        propLayout->addWidget(nameLabel);

        QLabel *dateLabel = new QLabel("Дата добавления: " + selectedPhoto->getDate().toString("dd MMMM yyyy"), this);
        dateLabel->setObjectName("infoLabel");
        dateLabel->setWordWrap(true);
        propLayout->addWidget(dateLabel);

        // Теги
        QStringList tagNames;
        for (const Tag &tag : selectedPhoto->getTags())
        {
            tagNames << tag.getName();
        }
        if (!tagNames.isEmpty())
        {
            QLabel *tagsLabel = new QLabel("Теги: " + tagNames.join(", "), this);
            tagsLabel->setObjectName("infoLabel");
            tagsLabel->setWordWrap(true);
            propLayout->addWidget(tagsLabel);
        }
        else
        {
            QLabel *tagsLabel = new QLabel("Теги: (добавьте)", this);
            tagsLabel->setObjectName("infoLabel");
            tagsLabel->setStyleSheet("color: #999;");
            propLayout->addWidget(tagsLabel);
        }

        // Комментарий
        if (!selectedPhoto->getDescription().isEmpty())
        {
            QLabel *commentLabel = new QLabel("Комментарии: " + selectedPhoto->getDescription(), this);
            commentLabel->setObjectName("infoLabel");
            commentLabel->setWordWrap(true);
            propLayout->addWidget(commentLabel);
        }
        else
        {
            QLabel *commentLabel = new QLabel("Комментарии: (добавьте)", this);
            commentLabel->setObjectName("infoLabel");
            commentLabel->setStyleSheet("color: #999;");
            propLayout->addWidget(commentLabel);
        }

        propLayout->addStretch();

        // Кнопки действий
        QPushButton *editBtn = new QPushButton("Редактировать", this);
        connect(editBtn, &QPushButton::clicked, this, &MainWindow::editPhoto);
        propLayout->addWidget(editBtn);

        QPushButton *delBtn = new QPushButton("Удалить", this);
        connect(delBtn, &QPushButton::clicked, this, &MainWindow::deleteItem);
        propLayout->addWidget(delBtn);
    }
    else if (selectedAlbum)
    {
        // Информация об альбоме
        int photoCount = manager.getAllPhotos(selectedAlbum).size();

        // Миниатюры альбома (превью как квадрат)
        QWidget *previewWidget = new QWidget(this);
        previewWidget->setObjectName("albumPreview");
        previewWidget->setFixedSize(280, 180);
        QGridLayout *previewLayout = new QGridLayout(previewWidget);
        previewLayout->setSpacing(2);
        previewLayout->setContentsMargins(0, 0, 0, 0);

        QList<Photo *> albumPhotos = manager.getAllPhotos(selectedAlbum);
        int maxPreviews = qMin(4, albumPhotos.size());

        if (maxPreviews > 0)
        {
            for (int i = 0; i < maxPreviews; ++i)
            {
                QLabel *thumb = new QLabel(previewWidget);
                int size = (maxPreviews == 1) ? 280 : 138;
                thumb->setPixmap(QPixmap(albumPhotos[i]->getFilePath()).scaled(size, size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
                thumb->setFixedSize(size, size);
                thumb->setScaledContents(true);
                previewLayout->addWidget(thumb, i / 2, i % 2);
            }
        }
        else
        {
            QLabel *placeholder = new QLabel("📁", previewWidget);
            placeholder->setAlignment(Qt::AlignCenter);
            placeholder->setStyleSheet("font-size: 60px; color: #ccc;");
            previewLayout->addWidget(placeholder, 0, 0, 2, 2);
        }

        propLayout->addWidget(previewWidget);

        QLabel *typeLabel = new QLabel("Тип: Альбом", this);
        typeLabel->setObjectName("infoLabel");
        propLayout->addWidget(typeLabel);

        QLabel *nameLabel = new QLabel("Название: " + selectedAlbum->getName(), this);
        nameLabel->setObjectName("infoLabel");
        nameLabel->setWordWrap(true);
        propLayout->addWidget(nameLabel);

        // Дата создания (можно получить из первого фото, если есть)
        if (!albumPhotos.isEmpty())
        {
            QLabel *dateLabel = new QLabel("Дата создания: " + albumPhotos.first()->getDate().toString("dd MMMM yyyy"), this);
            dateLabel->setObjectName("infoLabel");
            dateLabel->setWordWrap(true);
            propLayout->addWidget(dateLabel);
        }

        QLabel *countLabel = new QLabel("Количество фото: " + QString::number(photoCount), this);
        countLabel->setObjectName("infoLabel");
        propLayout->addWidget(countLabel);

        propLayout->addStretch();
    }
    else
    {
        QLabel *placeholder = new QLabel("Откройте любой элемент, чтобы\nпосмотреть сведения", this);
        placeholder->setAlignment(Qt::AlignCenter);
        placeholder->setWordWrap(true);
        propLayout->addWidget(placeholder);
        propLayout->addStretch();
    }
}

void MainWindow::showEmptyState()
{
    int index = tabWidget->currentIndex();
    QWidget *currentTab = tabWidget->widget(index);
    QLayout *oldLayout = currentTab->layout();
    if (oldLayout)
    {
        QLayoutItem *item;
        while ((item = oldLayout->takeAt(0)) != nullptr)
        {
            if (item->widget())
                delete item->widget();
            delete item;
        }
        delete oldLayout;
    }

    QVBoxLayout *layout = new QVBoxLayout(currentTab);

    QLabel *emptyLabel = new QLabel("Добро пожаловать,\n" + currentUser->getName() + "!", currentTab);
    emptyLabel->setObjectName("welcomeLabel");
    emptyLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(emptyLabel);

    layout->addStretch();

    QLabel *hintLabel = new QLabel("Импортируйте фото или создайте альбом для начала работы", currentTab);
    hintLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(hintLabel);

    QPushButton *addBtn = new QPushButton("Добавить фото", currentTab);
    addBtn->setObjectName("emptyAddButton");
    addBtn->setMenu(addButton->menu());
    addBtn->setFixedSize(200, 44);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(addBtn);
    btnLayout->addStretch();

    layout->addLayout(btnLayout);
    layout->addStretch();
}

void MainWindow::renderFeed(QWidget *container)
{
    QVBoxLayout *containerLayout = qobject_cast<QVBoxLayout *>(container->layout());
    if (!containerLayout)
    {
        containerLayout = new QVBoxLayout(container);
    }

    QScrollArea *scrollArea = new QScrollArea(container);
    scrollArea->setWidgetResizable(true);
    scrollArea->setObjectName("feedScrollArea");

    QWidget *content = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(content);
    layout->setSpacing(10);

    // Источник фотографий зависит от режима
    QList<Photo *> photos;
    if (inSearchMode)
    {
        photos = searchResults;
    }
    else if (currentSection == Favorites)
    {
        photos = favorites;
    }
    else if (currentSection == Recent)
    {
        QList<Photo *> all = manager.getAllPhotos(currentUser->getRootAlbum());
        std::sort(all.begin(), all.end(), [](Photo *a, Photo *b)
                  { return a->getDate() > b->getDate(); });
        int count = qMin(20, all.size());
        for (int i = 0; i < count; ++i)
            photos.append(all[i]);
    }
    else
    { // AllPhotos or Albums
        Album *targetAlbum = selectedAlbum ? selectedAlbum : currentUser->getRootAlbum();
        photos = manager.getAllPhotos(targetAlbum);
    }

    // Сортируем фото по дате (от нового к старому)
    std::sort(photos.begin(), photos.end(), [](Photo *a, Photo *b)
              { return a->getDate() > b->getDate(); });

    // Если фотографий нет — показываем сообщение
    if (photos.isEmpty())
    {
        QString msg = "Нет фотографий";
        if (currentSection == Albums && selectedAlbum)
            msg = "Альбом пуст";
        QLabel *emptyLabel = new QLabel(msg, content);
        emptyLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(emptyLabel);
    }

    // Группировка по дате
    QMap<QString, QList<Photo *>> photosByDate;
    for (Photo *photo : photos)
    {
        QString yearMonth = photo->getDate().toString("MMMM yyyy");
        photosByDate[yearMonth].append(photo);
    }

    // Отрисовка по группам (в хронологическом порядке)
    QList<QString> keys = photosByDate.keys();
    // Сортируем ключи по дате
    std::sort(keys.begin(), keys.end(), [](const QString &a, const QString &b)
              {
        QDate dateA = QDate::fromString(a, "MMMM yyyy");
        QDate dateB = QDate::fromString(b, "MMMM yyyy");
        return dateA > dateB; });

    for (const QString &key : keys)
    {
        // Заголовок группы
        QLabel *header = new QLabel(key, content);
        header->setObjectName("dateHeader");
        layout->addWidget(header);

        // Фотографии группы
        for (Photo *photo : photosByDate[key])
        {
            QWidget *photoCard = createPhotoCard(photo, content);
            layout->addWidget(photoCard);
        }
    }

    layout->addStretch();
    scrollArea->setWidget(content);
    containerLayout->addWidget(scrollArea);
}

void MainWindow::renderGrid(QWidget *container)
{
    QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout *>(container->layout());
    if (!mainLayout)
        mainLayout = new QVBoxLayout(container);

    QScrollArea *scroll = new QScrollArea(container);
    scroll->setWidgetResizable(true);

    QWidget *content = new QWidget();
    QGridLayout *grid = new QGridLayout(content);
    grid->setSpacing(14);

    QList<Photo *> photos;
    if (inSearchMode)
    {
        photos = searchResults;
    }
    else if (currentSection == Favorites)
    {
        photos = favorites;
    }
    else if (currentSection == Recent)
    {
        QList<Photo *> all = manager.getAllPhotos(currentUser->getRootAlbum());
        std::sort(all.begin(), all.end(), [](Photo *a, Photo *b)
                  { return a->getDate() > b->getDate(); });
        int count = qMin(20, all.size());
        for (int i = 0; i < count; ++i)
            photos.append(all[i]);
    }
    else
    {
        Album *album = selectedAlbum ? selectedAlbum : currentUser->getRootAlbum();
        photos = manager.getAllPhotos(album);
    }

    int col = 0, row = 0;
    const int columns = 3;

    if (photos.isEmpty())
    {
        QString msg = "Нет фотографий";
        if (currentSection == Albums && selectedAlbum)
            msg = "Альбом пуст";
        QLabel *emptyLabel = new QLabel(msg, content);
        emptyLabel->setAlignment(Qt::AlignCenter);
        grid->addWidget(emptyLabel, 0, 0, 1, columns, Qt::AlignCenter);
        scroll->setWidget(content);
        mainLayout->addWidget(scroll);
        return;
    }

    for (Photo *photo : photos)
    {
        ClickablePhotoWidget *card = new ClickablePhotoWidget(photo, content);
        card->setObjectName("photoGridCard");
        card->setCursor(Qt::PointingHandCursor);

        QVBoxLayout *cardLayout = new QVBoxLayout(card);
        cardLayout->setSpacing(6);
        cardLayout->setContentsMargins(6, 6, 6, 6);

        QLabel *img = new QLabel(card);
        img->setPixmap(QPixmap(photo->getFilePath())
                           .scaled(440, 440, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        img->setAlignment(Qt::AlignCenter);
        img->setFixedSize(440, 360);
        cardLayout->addWidget(img);

        QLabel *name = new QLabel(QFileInfo(photo->getFilePath()).fileName(), card);
        name->setAlignment(Qt::AlignCenter);
        name->setWordWrap(true);
        cardLayout->addWidget(name);

        connect(card, &ClickablePhotoWidget::clicked, this, [this, photo]()
                {
            selectedPhoto = photo;
            updatePropertiesPanel(); });
        connect(card, &ClickablePhotoWidget::doubleClicked, this, [this, photo]()
                { showFullScreen(photo); });
        connect(card, &ClickablePhotoWidget::rightClicked, this, [this, photo](Photo *, const QPoint &pos)
                {
            selectedPhoto = photo;
            handleRightClick(pos); });

        grid->addWidget(card, row, col);

        if (++col >= columns)
        {
            col = 0;
            row++;
        }
    }

    scroll->setWidget(content);
    mainLayout->addWidget(scroll);
}

void MainWindow::renderAlbums(QWidget *container)
{
    QVBoxLayout *containerLayout = qobject_cast<QVBoxLayout *>(container->layout());
    if (!containerLayout)
    {
        containerLayout = new QVBoxLayout(container);
    }

    QScrollArea *scrollArea = new QScrollArea(container);
    scrollArea->setWidgetResizable(true);
    scrollArea->setObjectName("albumsScrollArea");

    QWidget *content = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(content);
    layout->setSpacing(20);

    // Группировка альбомов по годам (если есть год в имени) —
    // если в корне есть «годовые» контейнеры (имя ровно 4 цифры),
    // используем их содержимое вместо показа контейнера как альбома.
    QMap<QString, QList<Album *>> albumsByYear;

    QList<Album *> rootSubs = currentUser->getRootAlbum()->getSubAlbums();
    for (Album *sub : rootSubs)
    {
        QString year = "Альбомы";
        QRegExp exactYear("^(\\d{4})$");
        QRegExp anyYear("(\\d{4})");
        if (exactYear.indexIn(sub->getName()) != -1)
        {
            year = exactYear.cap(1);
            // Если это контейнер годов — добавляем его дочерние альбомы под год
            if (!sub->getSubAlbums().isEmpty())
            {
                for (Album *inner : sub->getSubAlbums())
                    albumsByYear[year].append(inner);
                continue;
            }
            // Если контейнер пуст, всё равно не показываем его как обычный альбом — оставим как год без дочерних
        }
        else if (anyYear.indexIn(sub->getName()) != -1)
        {
            year = anyYear.cap(1);
        }
        albumsByYear[year].append(sub);
    }

    // Если альбомов нет — сообщение
    if (rootSubs.isEmpty())
    {
        QLabel *emptyLabel = new QLabel("Альбомы отсутствуют", content);
        emptyLabel->setAlignment(Qt::AlignLeft);
        emptyLabel->setObjectName("infoLabel");
        layout->addWidget(emptyLabel);
    }

    // Отрисовка по годам — сортируем ключи (годы сверху вниз)
    QList<QString> keys = albumsByYear.keys();
    std::sort(keys.begin(), keys.end(), [](const QString &a, const QString &b)
              {
        bool aNum = (a.toInt() > 0);
        bool bNum = (b.toInt() > 0);
        if (aNum && bNum) return a.toInt() > b.toInt();
        if (aNum) return true;
        if (bNum) return false;
        return a.toLower() < b.toLower(); });

    for (const QString &key : keys)
    {
        QLabel *yearLabel = new QLabel(key, content);
        yearLabel->setObjectName("yearHeader");
        layout->addWidget(yearLabel);

        QGridLayout *albumsGrid = new QGridLayout();
        albumsGrid->setSpacing(15);

        int row = 0, col = 0;
        int columns = 3;

        for (Album *album : albumsByYear[key])
        {
            QWidget *albumCard = createAlbumCard(album, content);
            albumsGrid->addWidget(albumCard, row, col);

            col++;
            if (col >= columns)
            {
                col = 0;
                row++;
            }
        }

        layout->addLayout(albumsGrid);
    }

    layout->addStretch();
    scrollArea->setWidget(content);
    containerLayout->addWidget(scrollArea);
}

Album *MainWindow::getOrCreateYearAlbum(int year)
{
    QString yearName = QString::number(year);
    Album *root = currentUser->getRootAlbum();

    for (Album *a : root->getSubAlbums())
        if (a->getName() == yearName)
            return a;

    Album *yearAlbum = new Album(yearName);
    root->addSubAlbum(yearAlbum);
    return yearAlbum;
}

void MainWindow::rebuildAlbumsTree()
{
    albumsTree->clear();
    for (Album *year : currentUser->getRootAlbum()->getSubAlbums())
        populateTree(year);
}

void MainWindow::renderAlbumView(QWidget *container, Album *album)
{
    QVBoxLayout *containerLayout = qobject_cast<QVBoxLayout *>(container->layout());
    if (!containerLayout)
    {
        containerLayout = new QVBoxLayout(container);
    }

    // Заголовок альбома
    QLabel *titleLabel = new QLabel("Альбомы/" + album->getName(), container);
    titleLabel->setObjectName("albumTitle");
    containerLayout->addWidget(titleLabel);

    QScrollArea *scrollArea = new QScrollArea(container);
    scrollArea->setWidgetResizable(true);

    QWidget *content = new QWidget();
    QGridLayout *gridLayout = new QGridLayout(content);
    gridLayout->setSpacing(15);

    QList<Photo *> photos = manager.getAllPhotos(album);

    int row = 0, col = 0;
    int columns = 3;

    for (Photo *photo : photos)
    {
        QWidget *photoCard = createPhotoCard(photo, content);
        gridLayout->addWidget(photoCard, row, col);

        col++;
        if (col >= columns)
        {
            col = 0;
            row++;
        }
    }

    scrollArea->setWidget(content);
    containerLayout->addWidget(scrollArea);
}

QWidget *MainWindow::createPhotoCard(Photo *photo, QWidget *parent)
{
    ClickablePhotoWidget *card = new ClickablePhotoWidget(photo, parent);
    card->setObjectName("photoCard");

    QHBoxLayout *layout = new QHBoxLayout(card);
    layout->setSpacing(10);
    layout->setContentsMargins(10, 10, 10, 10);

    QLabel *img = new QLabel(card);
    img->setFixedSize(320, 320);
    img->setPixmap(QPixmap(photo->getFilePath())
                       .scaled(320, 320, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    layout->addWidget(img);

    QVBoxLayout *info = new QVBoxLayout();

    QLabel *name = new QLabel(QFileInfo(photo->getFilePath()).fileName(), card);
    name->setObjectName("photoName");
    info->addWidget(name);

    QLabel *date = new QLabel(photo->getDate().toString("dd.MM.yyyy"), card);
    info->addWidget(date);

    info->addStretch();
    layout->addLayout(info);

    connect(card, &ClickablePhotoWidget::clicked, this, [this, card, photo]()
            {
           selectedPhoto = photo;
           card->setProperty("selected", true);
           updatePropertiesPanel(); });

    connect(card, &ClickablePhotoWidget::doubleClicked, this, [this, photo]()
            { showFullScreen(photo); });

    connect(card, &ClickablePhotoWidget::rightClicked, this,
            [this, photo](Photo *, const QPoint &pos)
            {
                selectedPhoto = photo;
                handleRightClick(pos);
            });

    return card;
}

QWidget *MainWindow::createAlbumCard(Album *album, QWidget *parent)
{
    ClickableAlbumWidget *card = new ClickableAlbumWidget(album, parent);
    card->setObjectName("albumCard");
    card->setMinimumSize(250, 200);
    card->setCursor(Qt::PointingHandCursor);

    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(10, 10, 10, 10);
    cardLayout->setSpacing(10);

    // Превью (сетка из фотографий)
    QWidget *previewWidget = new QWidget(card);
    previewWidget->setFixedSize(230, 150);
    previewWidget->setObjectName("albumPreview");
    QGridLayout *previewLayout = new QGridLayout(previewWidget);
    previewLayout->setSpacing(2);
    previewLayout->setContentsMargins(0, 0, 0, 0);

    QList<Photo *> photos = manager.getAllPhotos(album);
    int maxPreviews = qMin(4, photos.size());

    if (maxPreviews > 0)
    {
        for (int i = 0; i < maxPreviews; ++i)
        {
            QLabel *thumb = new QLabel(previewWidget);
            int size = (maxPreviews == 1) ? 230 : 114;
            thumb->setPixmap(QPixmap(photos[i]->getFilePath()).scaled(size, size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
            thumb->setFixedSize(size, size);
            thumb->setScaledContents(true);
            previewLayout->addWidget(thumb, i / 2, i % 2);
        }
    }
    else
    {
        // Если нет фото, показываем placeholder
        QLabel *placeholder = new QLabel("📁", previewWidget);
        placeholder->setAlignment(Qt::AlignCenter);
        placeholder->setStyleSheet("font-size: 48px; color: #ccc;");
        previewLayout->addWidget(placeholder, 0, 0, 2, 2);
    }

    cardLayout->addWidget(previewWidget);

    // Название и количество фото
    QLabel *nameLabel = new QLabel(album->getName() + " · " + QString::number(photos.size()) + " фото", card);
    nameLabel->setObjectName("albumName");
    nameLabel->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(nameLabel);

    // Обработка кликов: один клик — выделение, двойной клик — вход в альбом, ПКМ — контекстное меню
    connect(card, &ClickableAlbumWidget::clicked, this, [this, album]()
            {
        selectedAlbum = album;
        selectedPhoto = nullptr;
        currentSection = Albums;
        updatePropertiesPanel(); });

    connect(card, &ClickableAlbumWidget::doubleClicked, this, [this, album]()
            {
        selectedAlbum = album;
        selectedPhoto = nullptr;
        currentSection = Albums;
        tabWidget->tabBar()->show();
        updateCenterPanel();
        updatePropertiesPanel(); });

    connect(card, &ClickableAlbumWidget::rightClicked, this, [this, album](Album *, const QPoint &pos)
            {
        // Контекстное меню для карточки альбома
        QMenu menu(this);
        QAction* renameAct = menu.addAction("Переименовать");
        QAction* delAct = menu.addAction("Удалить");
        QAction* act = menu.exec(pos);
        if (act == renameAct) {
            bool ok;
            QString name = QInputDialog::getText(this, "Переименовать альбом", "Новое имя:", QLineEdit::Normal, album->getName(), &ok);
            if (ok && !name.trimmed().isEmpty()) {
                album->setName(name);
                rebuildAlbumsTree();
                updateCenterPanel();
            }
        } else if (act == delAct) {
            QMessageBox::StandardButton reply = QMessageBox::question(this, "Удаление", "Удалить альбом " + album->getName() + "?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes) {
                if (removeAlbumFromParent(album)) {
                    // Удаляем ссылки на фото из других коллекций перед удалением объекта
                    QList<Photo *> photosInAlbum = manager.getAllPhotos(album);
                    removePhotosReferencesFromList(photosInAlbum);

                    // Обновляем UI до удаления объекта, чтобы виджеты не ссылались на удалённый объект
                    selectedAlbum = nullptr;
                    rebuildAlbumsTree();
                    updateCenterPanel();
                    delete album;

                    // Сохраняем состояние
                    currentUser->saveToJson(currentUser->getName() + ".json");
                }
            }
        } });

    return card;
}

void MainWindow::populateAllPhotos()
{
    currentSection = AllPhotos;
    selectedAlbum = nullptr;
    updateCenterPanel();
}

void MainWindow::populateFavorites()
{
    currentSection = Favorites;
    selectedAlbum = nullptr;
    inSearchMode = false;
    updateCenterPanel();
}

void MainWindow::populateRecent()
{
    currentSection = Recent;
    selectedAlbum = nullptr;
    inSearchMode = false;
    updateCenterPanel();
}

void MainWindow::populateTags()
{
    currentSection = Tags;
    selectedAlbum = nullptr;
    updateCenterPanel();
}

void MainWindow::showSearchParams()
{
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("Параметры поиска");
    dialog->setMinimumSize(500, 400);

    QVBoxLayout *layout = new QVBoxLayout(dialog);

    // Дата
    QLabel *dateLabel = new QLabel("Дата:", dialog);
    layout->addWidget(dateLabel);

    QCalendarWidget *calendar = new QCalendarWidget(dialog);
    layout->addWidget(calendar);

    // Теги
    QLabel *tagLabel = new QLabel("Тег:", dialog);
    layout->addWidget(tagLabel);

    QComboBox *tagCombo = new QComboBox(dialog);

    // Собираем все теги
    QSet<QString> uniqueTags;
    QList<Photo *> allPhotos = manager.getAllPhotos(currentUser->getRootAlbum());
    for (Photo *photo : allPhotos)
    {
        for (const Tag &tag : photo->getTags())
        {
            uniqueTags.insert(tag.getName());
        }
    }

    for (const QString &tagName : uniqueTags)
    {
        tagCombo->addItem(tagName);
    }

    layout->addWidget(tagCombo);

    layout->addStretch();

    // Кнопки
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    QPushButton *searchBtn = new QPushButton("Поиск", dialog);
    connect(searchBtn, &QPushButton::clicked, [this, dialog, calendar, tagCombo]()
            {
        searchBar->setText(calendar->selectedDate().toString("yyyy-MM-dd") + " " + tagCombo->currentText());
        dialog->accept();
        search(); });
    btnLayout->addWidget(searchBtn);

    QPushButton *cancelBtn = new QPushButton("Отмена", dialog);
    connect(cancelBtn, &QPushButton::clicked, dialog, &QDialog::reject);
    btnLayout->addWidget(cancelBtn);

    layout->addLayout(btnLayout);

    dialog->exec();
}

void MainWindow::addToFavorites(Photo *photo)
{
    if (!favorites.contains(photo))
    {
        favorites.append(photo);
        // Обновляем представление и сохраняем изменения в user.json
        updateCenterPanel();
        saveFavorites();
        currentUser->saveToJson(currentUser->getName() + ".json");
        QMessageBox::information(this, "Избранное", "Фото добавлено в избранное");
    }
}

void MainWindow::exportTo()
{
    if (!selectedPhoto)
        return;

    QString destPath = QFileDialog::getSaveFileName(this, "Экспортировать в...", "", "Images (*.png *.jpg)");
    if (!destPath.isEmpty())
    {
        QFile::copy(selectedPhoto->getFilePath(), destPath);
        QMessageBox::information(this, "Экспорт", "Фото успешно экспортировано");
    }
}

void MainWindow::handleRightClick(const QPoint &pos)
{
    QMenu contextMenu(this);
    contextMenu.setStyleSheet(R"(
        QMenu {
            background-color: white;
            border: 1px solid #ddd;
            border-radius: 4px;
            padding: 5px;
        }
        QMenu::item {
            padding: 8px 20px;
            border-radius: 3px;
        }
        QMenu::item:selected {
            background-color: #f0f0f0;
        }
        QMenu::separator {
            height: 1px;
            background-color: #e0e0e0;
            margin: 5px 0;
        }
    )");

    if (selectedPhoto)
    {
        contextMenu.addAction("Экспортировать в...", this, &MainWindow::exportTo);
        contextMenu.addSeparator();
        contextMenu.addAction("Редактировать", this, &MainWindow::editPhoto);
        contextMenu.addAction("Удалить", this, &MainWindow::deleteItem);
    }

    contextMenu.exec(pos);
}

void MainWindow::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasUrls())
    {
        QList<QString> addedFiles;
        int successCount = 0;
        int failCount = 0;

        for (const QUrl &url : event->mimeData()->urls())
        {
            QString filePath = url.toLocalFile();
            QString suffix = QFileInfo(filePath).suffix().toLower();

            // Проверяем поддерживаемые форматы
            if (suffix == "jpg" || suffix == "jpeg" || suffix == "png" ||
                suffix == "bmp" || suffix == "gif" || suffix == "webp")
            {

                // Определяем целевой альбом
                Album *targetAlbum = selectedAlbum ? selectedAlbum : currentUser->getRootAlbum();

                // Создаём новое фото
                Photo *newPhoto = new Photo(filePath, "", QDateTime::currentDateTime());
                targetAlbum->addPhoto(newPhoto);

                addedFiles << QFileInfo(filePath).fileName();
                successCount++;
            }
            else
            {
                failCount++;
            }
        }

        // Обновляем интерфейс
        if (successCount > 0)
        {
            updateCenterPanel();
            updatePropertiesPanel();

            // Обновляем счётчик фото в шапке
            int photoCount = manager.getAllPhotos(currentUser->getRootAlbum()).size();
            userLabel->setText(currentUser->getName() + "\nЛокальное хранилище: " +
                               QString::number(photoCount) + " фото");

            // Показываем уведомление
            QString message;
            if (successCount == 1)
            {
                message = "Добавлено 1 фото: " + addedFiles.first();
            }
            else
            {
                message = "Добавлено " + QString::number(successCount) + " фото";
            }

            if (failCount > 0)
            {
                message += "\nНе удалось добавить: " + QString::number(failCount) + " файлов";
            }

            QMessageBox::information(this, "Импорт фото", message);
        }
        else if (failCount > 0)
        {
            QMessageBox::warning(this, "Импорт фото",
                                 "Не удалось добавить файлы.\nПоддерживаемые форматы: JPG, PNG, BMP, GIF, WEBP");
        }

        event->acceptProposedAction();
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    // Проверяем, есть ли в перетаскиваемых данных файлы
    if (event->mimeData()->hasUrls())
    {
        bool hasImages = false;

        // Проверяем, есть ли хотя бы один файл изображения
        for (const QUrl &url : event->mimeData()->urls())
        {
            QString suffix = QFileInfo(url.toLocalFile()).suffix().toLower();
            if (suffix == "jpg" || suffix == "jpeg" || suffix == "png" ||
                suffix == "bmp" || suffix == "gif" || suffix == "webp")
            {
                hasImages = true;
                break;
            }
        }

        if (hasImages)
        {
            event->acceptProposedAction();

            // Визуальная подсказка - можно добавить стиль
            setStyleSheet(styleSheet() + "\n#centerPanel { background-color: #fff8f0; }");
        }
    }
}

void MainWindow::dragLeaveEvent(QDragLeaveEvent *event)
{
    (void)event;
    // Убираем визуальную подсказку
    applyStyleSheet();
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
    {
        selectedPhoto = nullptr;
        updatePropertiesPanel();
        event->accept();
        return;
    }
    // Навигация в полноэкранном режиме
    if (fullScreenDialog && fullScreenDialog->isVisible())
    {
        if (event->key() == Qt::Key_Left)
        {
            navigateFullScreen(-1);
            event->accept();
            return;
        }
        else if (event->key() == Qt::Key_Right)
        {
            navigateFullScreen(1);
            event->accept();
            return;
        }
    }
    QMainWindow::keyPressEvent(event);
}

bool MainWindow::removeAlbumFromParent(Album *target)
{
    // Рекурсивно ищем родителя от корня
    QList<Album *> stack;
    stack.append(currentUser->getRootAlbum());
    while (!stack.isEmpty())
    {
        Album *a = stack.takeLast();
        for (Album *child : a->getSubAlbums())
        {
            if (child == target)
            {
                a->removeSubAlbum(child);
                return true;
            }
            stack.append(child);
        }
    }
    return false;
}

void MainWindow::navigateFullScreen(int delta)
{
    if (fullScreenPhotos.isEmpty() || !fullScreenDialog)
        return;

    if (delta != 0)
    {
        fullScreenIndex += delta;
        if (fullScreenIndex < 0)
            fullScreenIndex = 0;
        if (fullScreenIndex >= fullScreenPhotos.size())
            fullScreenIndex = fullScreenPhotos.size() - 1;
    }

    Photo *p = fullScreenPhotos.value(fullScreenIndex, nullptr);
    if (!p)
        return;

    QPixmap pix(p->getFilePath());
    QRect screenGeom = QGuiApplication::primaryScreen()->geometry();
    int maxWidth = screenGeom.width() - 100;
    int maxHeight = screenGeom.height() - 150;
    if (fullScreenImageLabel)
        fullScreenImageLabel->setPixmap(pix.scaled(maxWidth, maxHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void MainWindow::saveFavorites() const
{
    if (!currentUser)
        return;
    QString fn = currentUser->getName() + ".json";
    QFile f(fn);
    QJsonObject userObj;
    if (f.exists() && f.open(QIODevice::ReadOnly))
    {
        QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
        userObj = doc.object();
        f.close();
    }
    QJsonArray arr;
    for (Photo *p : favorites)
    {
        if (p)
            arr.append(p->getFilePath());
    }
    userObj["favorites"] = arr;
    QJsonDocument out(userObj);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        f.write(out.toJson());
        f.close();
    }
}

void MainWindow::loadFavorites()
{
    favorites.clear();
    if (!currentUser)
        return;
    QString fn = currentUser->getName() + ".json";
    QFile f(fn);
    if (!f.exists())
        return;
    if (!f.open(QIODevice::ReadOnly))
        return;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    if (!doc.isObject())
        return;
    QJsonObject userObj = doc.object();
    if (!userObj.contains("favorites"))
        return;
    QJsonArray arr = userObj["favorites"].toArray();
    for (const QJsonValue &v : arr)
    {
        QString path = v.toString();
        Photo *p = findPhotoByPath(path);
        if (p && !favorites.contains(p))
            favorites.append(p);
    }
}

Photo *MainWindow::findPhotoByPath(const QString &path) const
{
    if (!currentUser)
        return nullptr;
    QList<Photo *> all = manager.getAllPhotos(currentUser->getRootAlbum());
    for (Photo *p : all)
    {
        if (QFileInfo(p->getFilePath()).absoluteFilePath() == QFileInfo(path).absoluteFilePath())
            return p;
    }
    return nullptr;
}

void MainWindow::removePhotoReferences(Photo *photo)
{
    if (!photo)
        return;

    // Remove from favorites
    while (favorites.removeOne(photo))
    {
    }

    // Remove from search results
    while (searchResults.removeOne(photo))
    {
    }

    // Remove from full screen list and adjust index
    int removedIndex = -1;
    for (int i = fullScreenPhotos.size() - 1; i >= 0; --i)
    {
        if (fullScreenPhotos[i] == photo)
        {
            removedIndex = i;
            fullScreenPhotos.removeAt(i);
        }
    }
    if (removedIndex != -1)
    {
        if (fullScreenIndex >= fullScreenPhotos.size())
            fullScreenIndex = fullScreenPhotos.size() - 1;
    }

    // If selectedPhoto references this, clear it
    if (selectedPhoto == photo)
        selectedPhoto = nullptr;

    // Persist favorites if changed
    saveFavorites();
}

void MainWindow::removePhotosReferencesFromList(const QList<Photo *> &photos)
{
    for (Photo *p : photos)
        removePhotoReferences(p);
}

void MainWindow::applyStyleSheet()
{
    QString styleSheet = R"(
        QMainWindow {
            background-color: #f5f5f5;
        }

        #header {
            background-color: white;
            border-bottom: 1px solid #e0e0e0;
        }

        #avatarLabel {
            background-color: #12CCAD;
            color: white;
            border-radius: 22px;
            font-size: 20px;
            font-weight: bold;
        }

        #userLabel {
            font-size: 13px;
            color: #333;
            font-weight: 600;
            line-height: 1.4;
        }

        #searchBar {
            padding: 10px 20px;
            border: 1px solid #000000;
            border-radius: 25px;
            background-color: #f5f5f5;
            font-size: 14px;
        }

        #searchBar:focus {
            border-color: #000000;
            background-color: white;
        }

        #paramsButton {
            border: none;
            background-color: transparent;
            border-radius: 5px;
            padding: 8px 12px;
            font-size: 16px;
        }

        #paramsButton:hover {
            background-color: #f0f0f0;
        }

        #addButton {
            background-color: #ff9800;
            color: white;
            border: none;
            border-radius: 25px;
            font-size: 28px;
            font-weight: bold;
        }

        #addButton:hover {
            background-color: #f57c00;
        }

        #addButton::menu-indicator {
            image: none;
            width: 0px;
        }

        #emptyAddButton {
            background-color: #ff9800;
            color: white;
            border: none;
            border-radius: 22px;
            font-size: 14px;
            font-weight: 600;
        }

        #emptyAddButton:hover {
            background-color: #f57c00;
        }

        #addMenu {
            background-color: white;
            border: 1px solid #ddd;
            border-radius: 8px;
            padding: 8px;
        }

        #addMenu::item {
            padding: 12px 24px;
            border-radius: 6px;
            font-size: 14px;
        }

        #addMenu::item:selected {
            background-color: #fff3e0;
            color: #ff9800;
        }

        #leftPanel {
            background-color: #f9f9f9;
            border-right: 1px solid #e0e0e0;
        }

        #navigationList {
            border: none;
            background-color: transparent;
            font-size: 14px;
            outline: none;
        }

        #navigationList::item {
            padding: 12px 10px;
            border-radius: 6px;
            margin: 2px 0;
        }

        #navigationList::item:selected {
            background-color: #e8e8e8;
            color: #333;
            font-weight: bold;
        }

        #navigationList::item:hover {
            background-color: #f0f0f0;
        }

        #albumsTree {
            border: none;
            background-color: transparent;
            outline: none;
            font-size: 13px;
        }

        #albumsTree::item {
            padding: 8px 5px;
        }

        #albumsTree::item:selected {
            background-color: #e8e8e8;
            color: #333;
        }

        #albumsTree::item:hover {
            background-color: #f0f0f0;
        }

        #sectionLabel {
            font-weight: bold;
            padding: 15px 5px 8px 5px;
            color: #666;
            font-size: 13px;
        }

        #copyrightLabel {
            font-size: 10px;
            color: #999;
            padding: 15px 10px;
            background-color: transparent;
        }

        #centerPanel {
            background-color: white;
        }

        #tabWidget::pane {
            border: none;
            background-color: white;
        }

        #tabWidget::tab-bar {
            left: 0px;
        }

        QTabBar::tab {
            padding: 12px 28px;
            margin-right: 8px;
            border: none;
            border-bottom: 3px solid transparent;
            background-color: transparent;
            font-size: 15px;
            color: #666;
            min-width: 80px;
        }

        QTabBar::tab:selected {
            border-bottom: 3px solid #ff9800;
            color: #ff9800;
            font-weight: bold;
        }

        QTabBar::tab:hover {
            color: #ff9800;
        }

        #propertiesPanel {
            background-color: white;
            border-left: 1px solid #e0e0e0;
        }

        #propertiesTitle {
            font-weight: bold;
            font-size: 16px;
            padding-bottom: 15px;
            color: #333;
        }

        #infoLabel {
            font-size: 13px;
            color: #555;
            padding: 3px 0;
        }

        #photoCard {
            background-color: white;
            border: 1px solid #e5e5e5;
            border-radius: 8px;
            margin: 5px 0;
        }

        #photoCard:hover {
            border-color: #ff9800;
            background-color: #fffbf5;
        }

        #albumCard {
            background-color: white;
            border: 1px solid #e5e5e5;
            border-radius: 8px;
        }

        #photoCard[selected="true"],
        #albumCard[selected="true"]{
            border: 2px solid #ff9800;
            background-color: #fff3e0;
        }

        #albumCard:hover {
            border-color: #ff9800;
            box-shadow: 0 4px 12px rgba(0,0,0,0.1);
        }

        #albumPreview {
            background-color: #f5f5f5;
            border-radius: 6px;
            overflow: hidden;
        }

        #albumName {
            font-weight: bold;
            font-size: 14px;
            color: #333;
        }

        #photoName {
            font-weight: 600;
            font-size: 14px;
            color: #333;
        }

        #welcomeLabel {
            font-size: 36px;
            font-weight: bold;
            padding: 60px 40px;
            color: #333;
        }

        #sectionTitle {
            font-size: 28px;
            font-weight: bold;
            padding: 15px 0;
            color: #333;
        }

        #albumTitle {
            font-size: 24px;
            font-weight: bold;
            padding: 10px 0;
            color: #333;
        }

        #dateHeader, #yearHeader {
            font-size: 20px;
            font-weight: bold;
            padding: 20px 0 12px 0;
            color: #333;
        }

        #tagWidget {
            background-color: white;
            border: 1px solid #e0e0e0;
            border-radius: 25px;
            padding: 10px 20px;
            margin: 5px 0;
        }

        #tagWidget:hover {
            background-color: #f8f8f8;
        }

        #tagLabel {
            font-size: 15px;
            color: #333;
        }

        QPushButton {
            padding: 10px 18px;
            border: 1px solid #ddd;
            border-radius: 6px;
            background-color: white;
            font-size: 13px;
        }

        QPushButton:hover {
            background-color: #f5f5f5;
            border-color: #bbb;
        }

        QPushButton:pressed {
            background-color: #e8e8e8;
        }

        QScrollBar:vertical {
            border: none;
            background-color: #f5f5f5;
            width: 10px;
            margin: 0px;
        }

        QScrollBar::handle:vertical {
            background-color: #c0c0c0;
            border-radius: 5px;
            min-height: 20px;
        }

        QScrollBar::handle:vertical:hover {
            background-color: #a0a0a0;
        }

        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }

        QScrollBar:horizontal {
            border: none;
            background-color: #f5f5f5;
            height: 10px;
            margin: 0px;
        }

        QScrollBar::handle:horizontal {
            background-color: #c0c0c0;
            border-radius: 5px;
            min-width: 20px;
        }

        QScrollBar::handle:horizontal:hover {
            background-color: #a0a0a0;
        }

        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            width: 0px;
        }

        QMenu {
            background-color: white;
            border: 1px solid #ddd;
            border-radius: 6px;
            padding: 5px;
        }

        QMenu::item {
            padding: 10px 25px;
            border-radius: 4px;
        }

        QMenu::item:selected {
            background-color: #f0f0f0;
        }

        QMenu::separator {
            height: 1px;
            background-color: #e0e0e0;
            margin: 5px 10px;
        }
    )";

    setStyleSheet(styleSheet);
}
