#include "frmmain.h"
#include "ui_frmmain.h"

#include "m_decodethread.h"
#include "v2_m_video.h"
frmmain::frmmain(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::frmmain)
{
    ui->setupUi(this);
    this->resize(1920,1080);
//    ui->widget_2->setVisible(false);
//    ui->widget_3->setVisible(false);
//    ui->widget_4->setVisible(false);
//    ui->widget->setVisible(false);
}

frmmain::~frmmain()
{
    delete ui;
}


void frmmain::on_pushButton_clicked()
{
    QString filename = "D:/project/Qt_project/Example/MySrc/Video resource/童话镇.mp4";//foreman.mp4
    ui->widget->SetFrameUrl(filename);
    ui->widget->open();

}

void frmmain::on_pushButton_2_clicked()
{
    QString filename = "D:/project/Qt_project/Example/MySrc/Video resource/名侦探柯南.mp4";
    ui->widget_2->SetFrameUrl(filename);
    ui->widget_2->open();

    closeaudio = new M_Audio(this);
    closeaudio->SetUrl(filename);
    if(closeaudio->InitFFmpeg()){
        closeaudio->start();
    }
}

void frmmain::on_pushButton_3_clicked()
{
    QString filename = "D:/project/Qt_project/Example/MySrc/Video resource/history.mp4";
    ui->widget_3->SetFrameUrl(filename);
    ui->widget_3->open();
}

void frmmain::on_pushButton_4_clicked()
{
    QString filename = "D:/project/Qt_project/Example/MySrc/Video resource/小尖尖.mp4";
    ui->widget_4->SetFrameUrl(filename);
    ui->widget_4->open();


}

void frmmain::on_pushButton_5_clicked()
{
        QString filename = "D:/project/Qt_project/Example/MySrc/Video resource/童话镇.mp4";
        M_Audio *audio = new M_Audio(this);
        audio->SetUrl(filename);
        if(audio->InitFFmpeg()){
            audio->start();
        }
}

void frmmain::on_pushButton_6_clicked()
{
    QString filename = "D:/project/Qt_project/Example/MySrc/Video resource/名侦探柯南.mp4";
    M_Audio *audio = new M_Audio(this);
    audio->SetUrl(filename);
    if(audio->InitFFmpeg()){
        audio->start();
    }
}

void frmmain::on_pushButton_7_clicked()
{
    QString filename = "D:/project/Qt_project/Example/MySrc/Video resource/history.mp4";
    M_Audio *audio = new M_Audio(this);
    audio->SetUrl(filename);
    if(audio->InitFFmpeg()){
        audio->start();
    }
}

void frmmain::on_pushButton_8_clicked()
{
    QString filename = "D:/project/Qt_project/Example/MySrc/Video resource/若月亮还没来.mp4";
    M_Audio *audio = new M_Audio(this);
    audio->SetUrl(filename);
    if(audio->InitFFmpeg()){
        audio->start();
    }
}

void frmmain::on_pushButton_9_clicked()
{
    QString filename = "D:/project/Qt_project/Example/MySrc/Video resource/童话镇.mp4";
    ui->widget_7->Init(filename);
    ui->widget_7->StartDecode();
}

void frmmain::on_pushButton_10_clicked()
{
//    M_DecodeThread *m_decodethread = new M_DecodeThread(this);
    QString filename = "D:/project/Qt_project/Example/MySrc/Video resource/若月亮还没来.mp4";
//    m_decodethread->SetUrl(filename);
//    bool ret = m_decodethread->FFmpeg_Init();
//    qDebug()<<"ret = " << ret;
    ui->widget_6->SetUrl(filename);
    ui->widget_6->Start();
}
