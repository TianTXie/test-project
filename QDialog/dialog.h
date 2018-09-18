#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QJsonObject>
class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;
class QMouseEvent;
class QTextBrowser;
class QPushButton;

namespace Ui {
class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = nullptr);
    ~Dialog();
protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dropEvent(QDropEvent *event);
    void mousePressEvent(QMouseEvent *event);
private slots:
    void loadFile();
    void saveFile();
private:
    Ui::Dialog *ui;
    QTextBrowser* m_textBrowser;
    QPushButton *m_btnLoad;
    QPushButton *m_btnSave;
    QPushButton *m_btnExit;
    QJsonObject m_jsonData;
};

#endif // DIALOG_H
