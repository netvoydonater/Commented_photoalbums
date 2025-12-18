#include "photoeditdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QDateTimeEdit>
#include <QGraphicsRectItem>
#include <QFileInfo>
#include <QMessageBox>
#include <QTextEdit>
#include <QToolButton>
#include <QMenu>
#include <QRubberBand>
#include <QMouseEvent>

PhotoEditDialog::PhotoEditDialog(Photo* photo, QWidget* parent) : QDialog(parent), photo(photo) {
    setWindowTitle("Редактирование");
    resize(1000, 700);
    setupUI();
    applyStyles();
}

PhotoEditDialog::~PhotoEditDialog() {}

void PhotoEditDialog::setupUI() {
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // === ЛЕВАЯ ЧАСТЬ - Большое превью фото ===
    QWidget* leftWidget = new QWidget(this);
    leftWidget->setObjectName("leftPanel");
    QVBoxLayout* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    // Верхняя панель с заголовком и кнопкой закрытия
    QWidget* topBar = new QWidget(leftWidget);
    topBar->setObjectName("topBar");
    QHBoxLayout* topBarLayout = new QHBoxLayout(topBar);

    QLabel* titleLabel = new QLabel("Редактирование", topBar);
    titleLabel->setObjectName("dialogTitle");
    topBarLayout->addWidget(titleLabel);
    topBarLayout->addStretch();

    leftLayout->addWidget(topBar);

    // Превью фото с возможностью выделения области
    previewLabel = new QLabel(leftWidget);
    previewLabel->setObjectName("photoPreview");
    previewLabel->setAlignment(Qt::AlignCenter);
    previewLabel->setScaledContents(false);

    originalPixmap = QPixmap(photo->getFilePath());
    int maxWidth = 600;
    int maxHeight = 500;
    QPixmap scaled = originalPixmap.scaled(maxWidth, maxHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    previewLabel->setPixmap(scaled);
    previewLabel->setFixedSize(scaled.size());
    previewLabel->installEventFilter(this);

    leftLayout->addWidget(previewLabel, 1);

    // Нижняя панель с дополнительными действиями
    QWidget* bottomBar = new QWidget(leftWidget);
    QHBoxLayout* bottomBarLayout = new QHBoxLayout(bottomBar);

    QLabel* filenameLabel = new QLabel(QFileInfo(photo->getFilePath()).fileName(), bottomBar);
    filenameLabel->setObjectName("filenameLabel");
    bottomBarLayout->addWidget(filenameLabel);
    bottomBarLayout->addStretch();

    leftLayout->addWidget(bottomBar);

    mainLayout->addWidget(leftWidget, 2);

    // === ПРАВАЯ ЧАСТЬ - Информация и редактирование ===
    QWidget* rightWidget = new QWidget(this);
    rightWidget->setObjectName("rightPanel");
    rightWidget->setMinimumWidth(350);
    rightWidget->setMaximumWidth(350);
    QVBoxLayout* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(20, 20, 20, 20);
    rightLayout->setSpacing(15);

    // Меню действий
    QHBoxLayout* actionsLayout = new QHBoxLayout();

    QPushButton* favBtn = new QPushButton("В избранное", rightWidget);
    favBtn->setObjectName("actionButton");
    connect(favBtn, &QPushButton::clicked, this, &PhotoEditDialog::addToFavorites);
    actionsLayout->addWidget(favBtn);

    QPushButton* cropBtn = new QPushButton("Обрезать", rightWidget);
    cropBtn->setObjectName("actionButton");
    connect(cropBtn, &QPushButton::clicked, this, &PhotoEditDialog::applyCrop);
    actionsLayout->addWidget(cropBtn);

    QPushButton* deleteBtn = new QPushButton("Удалить", rightWidget);
    deleteBtn->setObjectName("actionButton");
    connect(deleteBtn, &QPushButton::clicked, this, &PhotoEditDialog::deletePhoto);
    actionsLayout->addWidget(deleteBtn);

    rightLayout->addLayout(actionsLayout);

    // Миниатюра
    QLabel* thumbnail = new QLabel(rightWidget);
    thumbnail->setPixmap(scaled.scaled(280, 280, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    thumbnail->setAlignment(Qt::AlignCenter);
    rightLayout->addWidget(thumbnail);

    // Информация о фото
    QLabel* typeLabel = new QLabel("Тип: фото", rightWidget);
    typeLabel->setObjectName("infoLabel");
    rightLayout->addWidget(typeLabel);

    QLabel* nameLabel = new QLabel("Имя файла: " + QFileInfo(photo->getFilePath()).fileName(), rightWidget);
    nameLabel->setObjectName("infoLabel");
    nameLabel->setWordWrap(true);
    rightLayout->addWidget(nameLabel);

    QLabel* dateLabel = new QLabel("Дата добавления: " + photo->getDate().toString("dd MMMM yyyy"), rightWidget);
    dateLabel->setObjectName("infoLabel");
    dateLabel->setWordWrap(true);
    rightLayout->addWidget(dateLabel);

    // Теги
    QLabel* tagsLabel = new QLabel("Теги: (добавьте)", rightWidget);
    tagsLabel->setObjectName("fieldLabel");
    rightLayout->addWidget(tagsLabel);

    tagEdit = new QLineEdit(rightWidget);
    tagEdit->setObjectName("inputField");
    tagEdit->setPlaceholderText("Введите теги через запятую...");

    QStringList existingTags;
    for (const Tag& tag : photo->getTags()) {
        existingTags << tag.getName();
    }
    if (!existingTags.isEmpty()) {
        tagEdit->setText(existingTags.join(", "));
    }

    rightLayout->addWidget(tagEdit);

    // Комментарии
    QLabel* commentLabel = new QLabel("Комментарии:", rightWidget);
    commentLabel->setObjectName("fieldLabel");
    rightLayout->addWidget(commentLabel);

    commentEdit = new QTextEdit(rightWidget);
    commentEdit->setObjectName("inputField");
    commentEdit->setPlaceholderText("Добавьте комментарий...");
    commentEdit->setText(photo->getDescription());
    commentEdit->setMaximumHeight(100);
    rightLayout->addWidget(commentEdit);

    rightLayout->addStretch();

    // Кнопка сохранения
    QPushButton* saveBtn = new QPushButton("Сохранить изменения", rightWidget);
    saveBtn->setObjectName("saveButton");
    connect(saveBtn, &QPushButton::clicked, this, &PhotoEditDialog::saveChanges);
    rightLayout->addWidget(saveBtn);

    mainLayout->addWidget(rightWidget);
}

void PhotoEditDialog::saveChanges() {
    // Сохранение тегов
    QString tagsText = tagEdit->text();
    if (!tagsText.isEmpty()) {
        QStringList tags = tagsText.split(",", QString::SkipEmptyParts);
        // Очищаем существующие теги (опционально)
        // photo->clearTags(); // Если есть такой метод
        for (const QString& tagName : tags) {
            QString trimmedTag = tagName.trimmed();
            if (!trimmedTag.isEmpty()) {
                photo->addTag(Tag(trimmedTag));
            }
        }
    }

    // Сохранение комментария
    photo->setDescription(commentEdit->toPlainText());

    QMessageBox::information(this, "Сохранение", "Изменения успешно сохранены");
    accept();
}

void PhotoEditDialog::addToFavorites() {
    emit addToFavoritesRequested(photo);
    QMessageBox::information(this, "Избранное", "Фото добавлено в избранное");
}

void PhotoEditDialog::deletePhoto() {
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Удаление",
        "Вы уверены, что хотите удалить это фото?",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        // Удаление будет обработано в главном окне
        reject();
    }
}

void PhotoEditDialog::applyCrop() {
    if (selectionRect.isNull() || selectionRect.width() < 5 || selectionRect.height() < 5) {
        QMessageBox::information(this, "Обрезка", "Выделите область для обрезки мышью по фото.");
        return;
    }

    QImage image(photo->getFilePath());
    if (image.isNull()) {
        QMessageBox::warning(this, "Обрезка", "Не удалось загрузить изображение для обрезки");
        return;
    }

    // Соотношение масштабирования между оригиналом и превью
    const QPixmap* scaledPtr = previewLabel->pixmap();
    if (!scaledPtr || scaledPtr->isNull()) {
        QMessageBox::warning(this, "Обрезка", "Не удалось получить превью изображения");
        return;
    }
    QPixmap scaled = *scaledPtr;

    double scaleX = static_cast<double>(image.width()) / scaled.width();
    double scaleY = static_cast<double>(image.height()) / scaled.height();

    QRect mappedRect(
        static_cast<int>(selectionRect.x() * scaleX),
        static_cast<int>(selectionRect.y() * scaleY),
        static_cast<int>(selectionRect.width() * scaleX),
        static_cast<int>(selectionRect.height() * scaleY)
    );
    mappedRect = mappedRect.intersected(image.rect());

    if (mappedRect.isEmpty()) {
        QMessageBox::warning(this, "Обрезка", "Некорректная область обрезки");
        return;
    }

    photo->crop(mappedRect);

    // Обновляем оригинальный и масштабированный pixmap
    originalPixmap = QPixmap(photo->getFilePath());
    QPixmap newScaled = originalPixmap.scaled(previewLabel->width(), previewLabel->height(),
                                             Qt::KeepAspectRatio, Qt::SmoothTransformation);
    previewLabel->setPixmap(newScaled);

    if (rubberBand) {
        rubberBand->hide();
        selectionRect = QRect();
    }

    QMessageBox::information(this, "Обрезка", "Фото успешно обрезано.");
}

void PhotoEditDialog::applyStyles() {
    QString styleSheet = R"(
        QDialog {
            background-color: #ffffff;
        }

        #leftPanel {
            background-color: #f5f5f5;
        }

        #topBar {
            background-color: #ffffff;
            padding: 10px;
            border-bottom: 1px solid #e0e0e0;
        }

        #dialogTitle {
            color: #333333;
            font-size: 16px;
            font-weight: bold;
        }

        #photoPreview {
            background-color: #f5f5f5;
            padding: 20px;
        }

        #filenameLabel {
            color: #666666;
            padding: 10px;
        }

        #rightPanel {
            background-color: #ffffff;
        }

        #actionButton {
            padding: 6px 12px;
            border: 1px solid #dddddd;
            border-radius: 4px;
            background-color: #ffffff;
            font-size: 11px;
        }

        #actionButton:hover {
            background-color: #f5f5f5;
        }

        #infoLabel {
            font-size: 13px;
            color: #333333;
        }

        #fieldLabel {
            font-weight: bold;
            font-size: 13px;
            color: #666666;
        }

        #inputField {
            padding: 8px;
            border: 1px solid #dddddd;
            border-radius: 4px;
            background-color: #ffffff;
        }

        #saveButton {
            background-color: #3498db;
            color: white;
            border: none;
            border-radius: 4px;
            padding: 12px;
            font-weight: bold;
        }

        #saveButton:hover {
            background-color: #2980b9;
        }
    )";

    setStyleSheet(styleSheet);
}

bool PhotoEditDialog::eventFilter(QObject* obj, QEvent* event) {
    if (obj == previewLabel) {
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent* me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton) {
                selecting = true;
                selectionOrigin = me->pos();
                if (!rubberBand) {
                    rubberBand = new QRubberBand(QRubberBand::Rectangle, previewLabel);
                }
                rubberBand->setGeometry(QRect(selectionOrigin, QSize()));
                rubberBand->show();
            }
        } else if (event->type() == QEvent::MouseMove) {
            if (selecting && rubberBand) {
                QMouseEvent* me = static_cast<QMouseEvent*>(event);
                rubberBand->setGeometry(QRect(selectionOrigin, me->pos()).normalized());
            }
        } else if (event->type() == QEvent::MouseButtonRelease) {
            if (selecting && rubberBand) {
                selecting = false;
                selectionRect = rubberBand->geometry();
            }
        }
    }
    return QDialog::eventFilter(obj, event);
}
