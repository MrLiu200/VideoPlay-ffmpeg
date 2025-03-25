#include "m_audio.h"
#include <QDebug>
#include <QElapsedTimer>
//#include <QAtomicInt>
#include <QTime>
M_Audio::M_Audio(QObject *parent) : QThread(parent)
{
    this->stopped = false;
    playIndex = 0;
    m_audioOutput = new QAudioOutput;
    this->Url = "";
    AudioIndex = 0;
    decodeIndex = 0;
    byteBuf.clear();
    timer = new QTimer;
//    timer->setInterval(10);
    timer->setTimerType(Qt::PreciseTimer);
    connect(timer,&QTimer::timeout,this,&M_Audio::playaudio);
}
uint8_t *audio_out_buffer = (uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE*2);
void M_Audio::run()
{
    QElapsedTimer elapsedtime;
        elapsedtime.start();
//        QMutexLocker lock(&mutex);
    while (!stopped) {
        if(av_read_frame(m_AVFormatContext,m_AVPacket) >= 0){//读取一个包的数据
            if(m_AVPacket->stream_index == AudioIndex){//判断是否为数据流
                if(avcodec_send_packet(m_AVCodecContext,m_AVPacket) == 0){//发送去解码
                    if(avcodec_receive_frame(m_AVCodecContext,m_AVFrame) == 0){//获取解码数据
                        qDebug()<<"pts = " << m_AVFrame->best_effort_timestamp << "dts = " << m_AVPacket->dts << "系统时间:" <<QTime::currentTime().toString("HH:mm:ss.zzz");
                        int len = swr_convert(swrcontext, &audio_out_buffer, MAX_AUDIO_FRAME_SIZE, (const uint8_t **)m_AVFrame->data, m_AVFrame->nb_samples);
                        if(len < 0 ){
                            continue;
                        }
                        int data_size = av_samples_get_buffer_size(nullptr, 2,len, AV_SAMPLE_FMT_S16, 1);
                        QByteArray atemp =  QByteArray((const char *)audio_out_buffer, data_size);
//                        m_audioIODevice->write(atemp);
                         QMutexLocker locker(&mutex);  // 加锁
                            byteBuf.append(atemp);
                            decodeIndex += atemp.size();
//                        QMutexLocker unlocker(&mutex);  // 解锁

#if 0
#if 0
                        int data_size = av_samples_get_buffer_size(nullptr, m_AVCodecContext->ch_layout.nb_channels,
                                                                   m_AVFrame->nb_samples, m_AVCodecContext->sample_fmt, 1);
#else
                        // 假设每个样本的字节大小是 2（16 位）或 4（32 位）
//                        int bytes_per_sample = (out_sample_fmt == AV_SAMPLE_FMT_S16) ? 2 : 4;
//                        int data_size = bytes_per_sample * out_channels * m_AVFrame->nb_samples;
                        int data_size = 2*2*m_AVFrame->nb_samples;
#endif
                        uint8_t *out_buffer = (uint8_t *)av_malloc(data_size);
                        int len = swr_convert(swrcontext, &out_buffer, data_size, (const uint8_t **)m_AVFrame->data, m_AVFrame->nb_samples);
                        if(len < 0 ){
                            av_free(out_buffer);
                            continue;
                        }
                        m_audioIODevice->write((const char*)out_buffer, data_size);
                        av_free(out_buffer);
#endif
//                        int elapsed = elapsedtime.elapsed(); // 从上一帧到现在的时间
//                        //                            qDebug()<< "elapsed = "<< elapsed;
//                        if (elapsed < 20) {
//                            msleep(20 - elapsed); // 等待适当的时间
//                        }
//                        elapsedtime.restart(); // 重启计时器
//                        usleep(1);
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

bool M_Audio::InitFFmpeg()
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
    //打印出文件流中的所有流类型
    for (unsigned int i = 0; i < m_AVFormatContext->nb_streams; i++) {
        AVStream *stream = m_AVFormatContext->streams[i];
        qDebug() << "Stream index:" << i
                 << " Codec type:" << stream->codecpar->codec_type
                 << " Codec ID:" << stream->codecpar->codec_id;
    }
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
        }else if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) // 判断是否为视频流
        {
            //计算帧率
            AVRational avgframeRate = stream->avg_frame_rate;
            if (avgframeRate.num != 0 && avgframeRate.den != 0) {
                    frameRate = (double)avgframeRate.num / avgframeRate.den; // 计算帧率
                    qDebug() << "Video frame rate:" << frameRate;
                }
        }
    }
    if (AudioIndex == -1)
    {
        printf("Didn't find a audio stream.\n");
        return false;
    }

    m_AVFrame = av_frame_alloc();

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
//    qDebug()<<"result = " << result;
    if(result != 0){
        qDebug()<< "swr_alloc_set_opts2 fail";
        return false;
    }
    //初始化 swrcontext
    swr_init(swrcontext);

    //初始化播放媒体设备
    QAudioFormat format;
    // Set up the format, eg.
    format.setSampleRate(out_sample_rate);
    format.setChannelCount(out_channels);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);
    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
          if (!info.isFormatSupported(format)) {
              qWarning() << "Raw audio format not supported by backend, cannot play audio.";
          }
    m_audioOutput = new QAudioOutput(format,this);
    m_audioIODevice = m_audioOutput->start();

    timer->start(1000/frameRate);
    return true;
}

QString M_Audio::ReadUrl()
{
    return Url;
}

void M_Audio::SetUrl(QString str)
{
    this->Url = str;
}

void M_Audio::stop()
{
    stopped = true;
    timer->stop();
}

void M_Audio::close()
{
    playIndex = 0;
    decodeIndex = 0;
    AudioIndex = 0;
    av_packet_free(&m_AVPacket);
    av_frame_free(&m_AVFrame);
    swr_free(&swrcontext);
    avcodec_free_context(&m_AVCodecContext);
    avformat_close_input(&m_AVFormatContext);
    avformat_free_context(m_AVFormatContext);
    qDebug()<<"close audio ok";
}

void M_Audio::playaudio()
{
    qDebug()<<"clock = "<<m_audioOutput->elapsedUSecs()/1000;
    // 音频缓存播放
    QMutexLocker locker(&mutex);  // 加锁
    if(m_audioOutput && m_audioOutput->state() != QAudio::StoppedState && m_audioOutput->state() != QAudio::SuspendedState)
    {
//        if(playIndex > byteBuf.size()) return;
        if(playIndex > decodeIndex){
            //播放完成，释放资源
            this->stop();
            this->close();
            qDebug() << "音频播放完毕";
            return;
        }
        int writeBytes = qMin(byteBuf.length(), m_audioOutput->bytesFree());
        playIndex += writeBytes;
#if 0
        QByteArray array = byteBuf.mid(playIndex,writeBytes);
        m_audioIODevice->write(array);
//        if(playIndex > byteBuf.size()){
//            timer->stop();
//        }
#else
        m_audioIODevice->write(byteBuf.data(), writeBytes);

        byteBuf = byteBuf.right(byteBuf.length() - writeBytes);
//
#endif
//        qDebug()<<"byteBuf.size = "<<byteBuf.size()<< ", writeBytes = " << writeBytes << "playIndex = " << playIndex;
    }
//    QMutexLocker unlocker(&mutex);  // 解锁
}
