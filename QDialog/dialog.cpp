#include "dialog.h"
#include "ui_dialog.h"
#include <QMimeData>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMouseEvent>
#include <QLabel>
#include <QTextBrowser>
#include <QPushButton>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QMessageBox>

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
    this->setFixedSize(400, 300);
    this->setAcceptDrops(true);
    this->setAutoFillBackground(true);

    m_btnLoad = new QPushButton(this);
    m_btnLoad->setGeometry(0, 270, 60, 30);
    m_btnLoad->setText("Load");
    m_btnLoad->show();
    m_btnLoad->setAttribute(Qt::WA_DeleteOnClose);
    connect(m_btnLoad, SIGNAL (released()), this, SLOT (loadFile()));

    m_btnSave = new QPushButton(this);
    m_btnSave->setGeometry(75, 270, 60, 30);
    m_btnSave->setText("Save");
    m_btnSave->show();
    m_btnSave->setAttribute(Qt::WA_DeleteOnClose);
    connect(m_btnSave, SIGNAL (released()), this, SLOT (saveFile()));

    m_btnExit = new QPushButton(this);
    m_btnExit->setGeometry(150, 270, 60, 30);
    m_btnExit->setText("Exit");
    m_btnExit->show();
    m_btnExit->setAttribute(Qt::WA_DeleteOnClose);
    connect(m_btnExit, SIGNAL (clicked()), QApplication::instance(), SLOT (quit()));
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::loadFile() {
    QString fileName = QFileDialog::getOpenFileName(
                this,
                tr("Open File"),
                "C:\\",
                "JSON FILE (*.json)"
                );
    QFile file(fileName);
    if( !file.open(QIODevice::ReadOnly) ) {
        qWarning("Open File Failed.");
        return;
    }
    QByteArray saveData = file.readAll();
    QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));
    m_jsonData = loadDoc.object();
    QJsonObject data = m_jsonData.value("dialog").toObject();

    for(auto it = data.begin(); it != data.end(); ++it) {
        QJsonObject subObj = it.value().toObject();
        if(it.key().contains("label")) {
            QLabel* label = new QLabel(this);
            label->move(subObj.value("hOffset").toInt(), subObj.value("vOffset").toInt());
            label->setText(subObj.value("data").toString());
            label->setObjectName(it.key());
            label->show();
            label->setAttribute(Qt::WA_DeleteOnClose);
        }else if(it.key().contains("textBrowser")) {
            // create text browser window
            m_textBrowser = new QTextBrowser(this);
            m_textBrowser->setGeometry(subObj.value("x").toInt(), subObj.value("y").toInt(),
                                       subObj.value("width").toInt(), subObj.value("height").toInt());
            m_textBrowser->show();
            m_textBrowser->setReadOnly(true);
            m_textBrowser->setAttribute(Qt::WA_DeleteOnClose);

        }
    }
}

void Dialog::saveFile() {
    QJsonDocument doc(m_jsonData);
    QByteArray byte_array = doc.toJson(QJsonDocument::Compact);
    QString json_str(byte_array);
    QString fileName;
    fileName = QFileDialog::getSaveFileName(this,
                                            tr("save file"),
                                            "",
                                            "JSON FILE (*.json)");
    if(fileName.isNull()) {
        return;
    }
    QFile file(fileName);
    if(!file.open(QIODevice::ReadWrite|QIODevice::Append))
       {
          QMessageBox::information(this, tr("Unable to open file"),
                       file.errorString());
          return;
       }
       QTextStream in(&file);
       in << json_str;
        qDebug() << json_str;
       file.flush();
       file.close();
}


void Dialog::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasFormat("application/x-dnditemdata")) {
        event->acceptProposedAction();
        if (event->source() == this) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        } else {
            event->acceptProposedAction();
        }
    } else {
        event->ignore();
    }
}

void Dialog::dragMoveEvent(QDragMoveEvent *event) {
    if (event->mimeData()->hasFormat("application/x-dnditemdata")) {
        if (event->source() == this) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        } else {
            event->acceptProposedAction();
        }
    } else {
        event->ignore();
    }
}

void Dialog::dropEvent(QDropEvent *event) {
    if (event->mimeData()->hasFormat("application/x-dnditemdata")) {
        QByteArray itemData = event->mimeData()->data("application/x-dnditemdata");
        QDataStream dataStream(&itemData, QIODevice::ReadOnly);
        QPoint offset;
        QString data;
        QString objectName;
        dataStream >> data >>  objectName >> offset;
        QRect rect = m_textBrowser->geometry();
        QPoint pt = event->pos();
        if( event->source() == this && rect.contains(pt) == true ) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
            m_textBrowser->append(data);
            //move dragged item to textBrowser
            {
              QJsonObject jsData = m_jsonData.value("dialog").toObject();
              for(auto it = jsData.begin(); it != jsData.end(); ++it) {
                QString keyS = it.key();
                if( keyS == objectName ) {
                    QJsonObject textBrowserJsonObject = jsData.value("textBrowser").toObject();
                    textBrowserJsonObject.insert(objectName, it.value().toObject());
                    jsData["textBrowser"] = textBrowserJsonObject;
                    jsData.erase(it);
                    m_jsonData["dialog"] = jsData;
                }
              }
            }
        } else {
            event->acceptProposedAction();
        }
    } else {
        event->ignore();
    }
}

void Dialog::mousePressEvent(QMouseEvent *event) {
    if( event->button() != Qt::LeftButton ) {
        event->ignore();
        return;
    }
    QWidget* child = childAt(event->pos());
    if( child != nullptr ) {
        QString classname = child->metaObject()->className();
        if( classname == "QLabel") {
            QByteArray itemData;
            QDataStream dataStream(&itemData, QIODevice::WriteOnly);
            dataStream << static_cast<QLabel*>(child)->text() << static_cast<QLabel*>(child)->objectName() << QPoint(event->pos() - child->pos());
            QMimeData *mimeData = new QMimeData;
            mimeData->setData("application/x-dnditemdata", itemData);
            QDrag *drag = new QDrag(this);
            drag->setMimeData(mimeData);
            drag->setHotSpot(event->pos() - child->pos());
            if (drag->exec(Qt::CopyAction | Qt::MoveAction, Qt::CopyAction) == Qt::MoveAction) {
                child->close();
            } else {
                child->show();
            }
        } else {
            event->ignore();
        }
    } else {
        event->ignore();
    }
}
