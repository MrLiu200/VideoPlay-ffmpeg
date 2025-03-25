#ifndef M_FFMPEG_H
#define M_FFMPEG_H

#include <QObject>
#include <QThread>
#include <QDebug>
#include <QImage>
#include <QTime>
#include <QWidget>
#include <QPainter>
#include <QElapsedTimer>
#include <QAudioOutput>
#include <QMutex>
#include <QMutexLocker>
//引入ffmpeg头文件
extern "C"{
#include "libavcodec/avcodec.h"
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libswscale/swscale.h>
#include <libavutil/frame.h>
}

//#define USE_AUDIO
class M_FFmpeg : public QThread
{
    Q_OBJECT
    Q_PROPERTY(QString Url READ readUrl WRITE setUrl);
public:
    M_FFmpeg(QObject *parent);
    bool init(bool IsNetUrl = true);

protected:
    void run() override;


private:
    volatile bool stopped;                  //线程结束标志
    QMutex mutex;
    int frameCount;                         //帧数统计
    double frameRate;                       //帧率
    int videoIndex;                         //视频流索引

    AVFormatContext *m_AVFormatContext;     //容器格式上下文
    AVPacket *m_AVPacket;                   //已编码压缩的视频或音频数据包
    AVPacket m_AVPacket_test;                   //已编码压缩的视频或音频数据包
    AVCodecContext *m_AVCodecContext;       //编码器或解码器上下文
    AVFrame *m_AVFrame;                     //编码或解码后的YUV数据
    SwsContext *m_SwsContext;               //处理图片数据对象

    QAudioOutput *m_audioOutput;                    //一个音频输出对象
    QIODevice *m_audioIODevice;

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
    QString readUrl();

signals:
    //收到图片信号
    void receiveImage(const QImage &image);
    void PlayOut(void);
};


//显示窗口
class M_FFmpegWidget : public QWidget
{
    Q_OBJECT
public:
    explicit M_FFmpegWidget(QWidget *parent = 0);
    ~M_FFmpegWidget();


protected:
    void paintEvent(QPaintEvent *event) override;
private:
    M_FFmpeg * m_ffmpeg;
    QImage m_image;
    QString FrameUrl;
    int count;
    QElapsedTimer elapsedtime;

public slots:
    void open();
    void SetFrameUrl(QString url);
    void setImage(const QImage &image);
    void PlayOut(void);

private slots:
    void receiveImage(const QImage &image);

};

#endif // M_FFMPEG_H
