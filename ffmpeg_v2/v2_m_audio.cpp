#include "v2_m_audio.h"
#include <QDebug>
#include <QElapsedTimer>
#include <QTime>
uint8_t *audio_out_buffer_v2 = (uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE_v2*2);
v2_M_Audio::v2_M_Audio(QObject *parent) : QThread(parent)
{
    this->stopped = false;
    this->Url = "";
    AudioIndex = 0;
}

void v2_M_Audio::run()
{
    QElapsedTimer elapsedtime;
    elapsedtime.start();
    QMutexLocker lock(&mutex);
    while (!stopped) {
        if(av_read_frame(m_AVFormatContext,m_AVPacket) >= 0){//读取一个包的数据
            if(m_AVPacket->stream_index == AudioIndex){//判断是否为数据流
                if(avcodec_send_packet(m_AVCodecContext,m_AVPacket) == 0){//发送去解码
                    if(avcodec_receive_frame(m_AVCodecContext,m_AVFrame) == 0){//获取解码数据
                        int len = swr_convert(swrcontext, &audio_out_buffer_v2, MAX_AUDIO_FRAME_SIZE_v2, (const uint8_t **)m_AVFrame->data, m_AVFrame->nb_samples);
                        if(len < 0 ){
                            continue;
                        }
                        int data_size = av_samples_get_buffer_size(nullptr, 2,len, AV_SAMPLE_FMT_S16, 1);
                        QByteArray atemp =  QByteArray((const char *)audio_out_buffer_v2, data_size);
                        Q_EMIT receiveAudio(atemp);
                    }
                }
            }
        }
        av_packet_unref(m_AVPacket);//擦除数据包内容
        msleep(1);
    }
    stopped = false;
    qDebug() << "Stop FFmpeg Thread";
}

bool v2_M_Audio::InitFFmpeg()
{
    //申请一个AVFormatContext结构的内存
    m_AVFormatContext = avformat_alloc_context();

    //打开一个视频文件并且初始化m_AVFormatContext结构体
    if(avformat_open_input(&m_AVFormatContext,Url.toUtf8().data(),NULL,NULL) != 0){
        //正常返回0，不正常返回负数
        qDebug()<<"avformat_open_input error";
        return false;
    }

    //获取流信息
    if(avformat_find_stream_info(m_AVFormatContext,NULL) != 0){
        qDebug()<<"avformat_find_stream_info error";
        return false;
    }

    // 打印媒体文件的格式信息
    av_dump_format(m_AVFormatContext, 1, Url.toUtf8().data(), 0);

    // 查找视频流的起始索引位置（nb_streams表示视音频流的个数）
    AudioIndex = -1;

    //找出音频流的索引
    for (int i = 0; i < (int)m_AVFormatContext->nb_streams; i++)
    {
        AVStream *stream = m_AVFormatContext->streams[i];
        // 查找到视频流时退出循环
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) // 判断是否为视频流
        {
            AudioIndex = i;
            qDebug()<<"---------------------------------------------------------";
            qDebug()<< " Codec Rate:"<< stream->codecpar->sample_rate
                    << " frame_size:" << stream->codecpar->frame_size
                    <<" nb_channels:" << stream->codecpar->ch_layout.nb_channels;
            qDebug()<<"---------------------------------------------------------";
            //            break;
        }
    }

    if (AudioIndex == -1)
    {
        printf("Didn't find a audio stream.\n");
        return false;
    }

    m_AVFrame = av_frame_alloc();

    double durltion = m_AVFormatContext->duration / (double)AV_TIME_BASE;
    qDebug()<<"音频时长" << durltion;
    //查找解码器
    AVCodecParameters *m_AVCodecParameters = avcodec_parameters_alloc();
    // 获取流编码结构
    m_AVCodecParameters = m_AVFormatContext->streams[AudioIndex]->codecpar;

    //获取流解码器
    const AVCodec *m_AVCodec = avcodec_find_decoder(m_AVCodecParameters->codec_id);

    //分配一个解码器为 m_AVCodec 的m_AVCodecContext内存
    m_AVCodecContext = avcodec_alloc_context3(m_AVCodec);
    //填充m_AVCodecContext
    avcodec_parameters_to_context(m_AVCodecContext,m_AVCodecParameters);
    //打开对应解码器
    if(avcodec_open2(m_AVCodecContext,m_AVCodec,NULL) < 0 ){
        return false;
    }

    //开辟m_AVPacket空间
    m_AVPacket = av_packet_alloc();

    // 获取音频参数

    AVChannelLayout out_channel_layout = m_AVCodecContext->ch_layout;//通道布局
    AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
    //    AVSampleFormat out_sample_fmt = m_AVCodecContext->sample_fmt;//采样格式
    const int out_sample_rate = m_AVCodecContext->sample_rate;//采样率
    const int out_channels = out_channel_layout.nb_channels;//通道数量

    //    qDebug()<<"      采样率: "<< out_sample_rate;
    //开辟swrcontext 空间
    swrcontext = nullptr;
    int result = swr_alloc_set_opts2(&swrcontext, &out_channel_layout, out_sample_fmt,out_sample_rate,
                                     &m_AVCodecContext->ch_layout, m_AVCodecContext->sample_fmt, m_AVCodecContext->sample_rate,0,nullptr);

    if(result != 0){
        qDebug()<< "swr_alloc_set_opts2 fail";
        return false;
    }
    //初始化 swrcontext
    swr_init(swrcontext);

    Q_EMIT AudioDevice_Init(out_sample_rate,out_channels,16);
    return true;
}

QString v2_M_Audio::ReadUrl()
{
    return Url;
}

void v2_M_Audio::SetUrl(QString str)
{
    this->Url = str;
}

void v2_M_Audio::stop()
{
    stopped = true;

}

void v2_M_Audio::close()
{
    AudioIndex = 0;
    av_packet_free(&m_AVPacket);
    av_frame_free(&m_AVFrame);
    swr_free(&swrcontext);
    avcodec_free_context(&m_AVCodecContext);
    avformat_close_input(&m_AVFormatContext);
    avformat_free_context(m_AVFormatContext);
    qDebug()<<"close audio ok";
}

