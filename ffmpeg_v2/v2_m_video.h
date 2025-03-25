#ifndef V2_M_VIDEO_H
#define V2_M_VIDEO_H

#include <QObject>
#include <QThread>
#include <QDebug>
#include <QImage>
#include <QTime>
#include <QTimer>
#include <QElapsedTimer>
#include <QMutex>
#include <QMutexLocker>
#include <QWidget>
#include <QPainter>
//#include "v2_decodewidget.h"
#include "v2_m_audio.h"
//引入ffmpeg头文件
extern "C"{
#include "libavcodec/avcodec.h"
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libswscale/swscale.h>
#include <libavutil/frame.h>
}
#define CURRENTTIME QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz")
class v2_DecodeWidget;
class v2_M_Video : public QThread
{
    Q_OBJECT
    Q_PROPERTY(QString Url READ readUrl WRITE setUrl);
public:
    v2_M_Video(v2_DecodeWidget *DecodeWidget, QObject *parent = nullptr);
    bool init(bool IsNetUrl = true);

protected:
    void run() override;


private:
    volatile bool stopped;                  //线程结束标志
    v2_DecodeWidget *decodewidget;
    QMutex mutex;
    int frameCount;                         //帧数统计
    double frameRate;                       //帧率
    int videoIndex;                         //视频流索引
    double m_video_time_base;                   //视频时间基

    AVFormatContext *m_AVFormatContext;     //容器格式上下文
    AVPacket *m_AVPacket;                   //已编码压缩的视频或音频数据包
    AVCodecContext *m_AVCodecContext;       //编码器或解码器上下文
    AVFrame *m_AVFrame;                     //编码或解码后的YUV数据
    SwsContext *m_SwsContext;               //处理图片数据对象

    QString Url;                            //要打开的url或者是本地文件名称

    int frameheight;                        //视频流高度
    int framewidth;                         //视频流宽度
    int interval;                           //显示间隔

    bool IsPlayout;
public slots:
    //停止线程
    void stop();
    //关闭
    void close();

public slots:
    void setUrl(QString NewUrl);
    QString readUrl() const;

signals:
    //收到图片信号
    void receiveImage(const QImage &image);
    void PlayOut(void);
};


class v2_DecodeWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(double clock READ GetClock WRITE SetClock)
public:
    explicit v2_DecodeWidget(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
public:
    void Init(QString Url);
    void StartDecode(void);
    double GetClock(void) const;
    void SetClock(double value);
    double testclock(void);
private:
    QMutex mutex;                               //锁，在处理音频时使用
    QString m_Url;                              //流地址或文件名
    double clock;                               //主时钟
    double pauseclock;                          //暂停时间
    double startpauseclock;                     //开始暂停
    double endpauseclock;                       //结束暂停
    bool IsPause;                               //是否暂停
    //音频
    v2_M_Audio *m_Audio;                        //音频解码线程
    QAudioOutput *m_audiooutput;                //音频播放
    QIODevice *m_audioIODevice;                 //播放设备
    QByteArray m_AudioData;                     //音频数据
    QTimer *m_audiotimer;                       //音频数据添加至播放器
    bool First;


    //视频
    v2_M_Video *m_Video;                        //视频解码线程
    QImage m_image;


private slots:
    void AudioDevice_Init(const int &SampleRate, const int &Channles, const int &SampleFmt);    //播放器初始化
    void receiveAudio(const QByteArray &data);        //音频返回
    void handleStateChanged(QAudio::State state);                   //播放器状态返回

    void receiveImage(const QImage &image);                         //图片返回

private:
    void AddPlayBuffer(void);                   //播放容器添加内容
signals:

};

#endif // V2_M_VIDEO_H
