#ifndef FRMMAIN_H
#define FRMMAIN_H

#include <QWidget>
#include "m_audio.h"
QT_BEGIN_NAMESPACE
namespace Ui { class frmmain; }
QT_END_NAMESPACE

class frmmain : public QWidget
{
    Q_OBJECT

public:
    frmmain(QWidget *parent = nullptr);
    ~frmmain();

private slots:
    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();

    void on_pushButton_4_clicked();

    void on_pushButton_5_clicked();

    void on_pushButton_6_clicked();

    void on_pushButton_7_clicked();

    void on_pushButton_8_clicked();

    void on_pushButton_9_clicked();

    void on_pushButton_10_clicked();

private:
    Ui::frmmain *ui;
    M_Audio *closeaudio;
};
#endif // FRMMAIN_H
