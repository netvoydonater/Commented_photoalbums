#include "logindialog.h"
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

LoginDialog::LoginDialog(QWidget* parent) : QDialog(parent) {
    QVBoxLayout* layout = new QVBoxLayout(this);
    QLineEdit* nameEdit = new QLineEdit(this);
    layout->addWidget(nameEdit);
    QPushButton* ok = new QPushButton("Войти", this);
    connect(ok, &QPushButton::clicked, [this, nameEdit]() {
        userName = nameEdit->text();
        accept();
    });
    layout->addWidget(ok);
}

QString LoginDialog::getUserName() const { return userName; }
