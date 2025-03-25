#ifndef V2_M_AUDIO_H
#define V2_M_AUDIO_H

#include <QObject>
#include <QThread>
#include <QAudioOutput>
#include <QTimer>
#include <QMutex>
#include <QMutexLocker>
extern "C"{
#include "libavcodec/avcodec.h"
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libswscale/swscale.h>
#include <libavutil/frame.h>
#include <libswresample/swresample.h>
}
#define MAX_AUDIO_FRAME_SIZE_v2 19200
class v2_M_Audio : public QThread
{
    Q_OBJECT
    Q_PROPERTY(QString Url READ ReadUrl WRITE SetUrl);
public:
    v2_M_Audio(QObject *parent = nullptr);

protected:
    void run() override;


private:
    bool stopped;
    QMutex mutex;
    AVFormatContext *m_AVFormatContext;     //容器格式上下文
    AVPacket *m_AVPacket;                   //已编码压缩的视频或音频数据包
    AVPacket m_AVPacket_test;                   //已编码压缩的视频或音频数据包
    AVCodecContext *m_AVCodecContext;       //编码器或解码器上下文
//    AVCodec
    AVFrame *m_AVFrame;                     //编码或解码后的YUV数据
    SwrContext *swrcontext;                 //音频转换
    int AudioIndex;                         //音频的索引
    QString Url;                            //文件或流地址

public slots:
    bool InitFFmpeg(void);
    QString ReadUrl(void);
    void SetUrl(QString str);
    //停止线程
    void stop();
    //关闭
    void close();

signals:
    void AudioDevice_Init(const int &SampleRate, const int &Channles, const int &SampleFmt);
    void receiveAudio(const QByteArray &audiodata);
};

#endif // V2_M_AUDIO_H
