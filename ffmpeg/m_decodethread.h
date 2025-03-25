#ifndef M_DECODETHREAD_H
#define M_DECODETHREAD_H

#include <QObject>
#include <QThread>
#include <QAudioOutput>
#include <QWidget>
#include <QMutex>
#include <QMutexLocker>
#include <QDateTime>
#include <QMap>
#include <QPair>
extern "C"{
#include "libavcodec/avcodec.h"
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libswscale/swscale.h>
#include <libavutil/frame.h>
#include <libswresample/swresample.h>
}
#define FRAME_SIZE 19200
#define DATATIME QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz")
class M_DecodeThread : public QThread
{
    Q_OBJECT
    Q_PROPERTY(QString StreamUrl READ ReadUrl WRITE SetUrl)
public:
    M_DecodeThread(QObject *parent = nullptr);
protected:
    void run() override;

private:
    bool stopped;                               //线程运行标志

    AVFormatContext *m_AVFormatContext;         //容器格式上下文
    AVCodecContext *m_audioCodecContext;        //音频编码器或解码器上下文
    AVCodecContext *m_videoCodecContext;        //视频编码器或解码器上下文
    AVPacket *m_AVPacket;                       //已编码压缩的视频或音频数据包
    AVPacket *m_audioPacket;                    //音频解码
    AVFrame *m_AVFrame;                         //编码或解码后的YUV数据
    SwrContext *m_Swrcontext;                   //音频转换
    SwsContext *m_SwsContext;                   //视频转换

    QString StreamUrl;                          //流媒体url
    int m_audioIndex;                           //音频索引
    int m_videoIndex;                           //视频索引
    int PixHeight;                              //视频高度
    int PixWidth;                               //视频宽度
    double audiopts;                            //音频时间戳
    int StreamRate;                             //视频帧率
    int FrameTime;                              //每帧所耗费的时间
    double m_audio_time_base;                   //音频时间基
    double m_video_time_base;                   //视频时间基
    double m_audio_start_pts;                   //音频开始pts
    double m_video_start_pts;                   //视频开始pts
    double m_audio_pts;                         //音频当前pts
    double m_video_pts;                         //视频当前pts
    bool m_FirstAudioFrame;
    bool m_FirstVideoFrame;


    QAudioOutput *m_audiooutput;                //音频输出
    QIODevice *m_audioIODevice;                 //IO设备



public:
    //ffmpeg初始化
    bool FFmpeg_Init(void);
    void SetAudioPts(double pts);

//    void AudioDevice_Init(const int SampleRate, const int Channles, const int SampleFmt);

    //设置流媒体地址
    void SetUrl(QString Url){
        if(!Url.isEmpty()){
            StreamUrl = Url;
        }
    }
    //读取流媒体地址
    QString ReadUrl(void){
        return StreamUrl;
    }

    void close();

signals:
    void receiveImage(const QImage &image);
    void receiveAudio(const QByteArray &audiodata);
    void AudioDevice_Init(const int &SampleRate, const int &Channles, const int &SampleFmt, const int &Rate);
    void testAudio(const QByteArray &audiodata,double pts);

};


class M_DecodeWidegt : public QWidget
{
    Q_OBJECT
public:
    explicit M_DecodeWidegt(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
private:
    M_DecodeThread *m_decodethread;
    QImage m_image;
    QAudioOutput *m_audiooutput;
    QIODevice *m_audioIODevice;
    QByteArray m_AudioData;
    QTimer *m_audiotimer;
    QMutex m_mutex;
    int m_StreamRate;
    QString m_Url;
    int clock;
    QMap<double,QByteArray> m_audioList;
    QList<QPair<QByteArray,double>> testlist;

private slots:
    void AudioDevice_Init(const int &SampleRate, const int &Channles, const int &SampleFmt, const int &Rate);
    void receiveImage(const QImage &image);
    void receiveAudio(const QByteArray &audiodata);
    void PlayAudio(void);
    void audiotest(const QByteArray &audiodata, double pts);


public slots:
    void Start(void);
    void SetUrl(QString file);
    void handleStateChanged(QAudio::State state);

};

#endif // M_DECODETHREAD_H
