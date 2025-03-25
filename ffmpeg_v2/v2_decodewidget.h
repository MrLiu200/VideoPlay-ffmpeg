#ifndef V2_DECODEWIDGET_H
#define V2_DECODEWIDGET_H

#include <QWidget>
#include <QAudioOutput>
#include <QMutex>
#include <QMutexLocker>
#include <QTimer>
#include "v2_m_audio.h"
#include "v2_m_video.h"
class v2_DecodeWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(double clock READ GetClock WRITE SetClock)
public:
    explicit v2_DecodeWidget(QWidget *parent = nullptr);

public:
    void Init(QString Url);
    void StartDecode(void);
    double GetClock(void) const;
    void SetClock(double value);
private:
    QMutex mutex;                               //锁，在处理音频时使用
    QString m_Url;                              //流地址或文件名
    double clock;                               //主时钟
    //音频
    v2_M_Audio *m_Audio;                        //音频解码线程
    QAudioOutput *m_audiooutput;                //音频播放
    QIODevice *m_audioIODevice;                 //播放设备
    QByteArray m_AudioData;                     //音频数据
    QTimer *m_audiotimer;                       //音频数据添加至播放器

    //视频
    v2_M_Video *m_Video;                        //视频解码线程


private slots:
    void AudioDevice_Init(const int &SampleRate, const int &Channles, const int &SampleFmt);    //播放器初始化
    void receiveAudio(QByteArray &data);        //音频返回
    void handleStateChanged(QAudio::State state);                   //播放器状态返回

    void receiveImage(const QImage &image);                         //图片返回

private:
    void AddPlayBuffer(void);                   //播放容器添加内容
signals:

};

#endif // V2_DECODEWIDGET_H
