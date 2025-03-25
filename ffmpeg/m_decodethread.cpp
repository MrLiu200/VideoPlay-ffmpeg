#include "m_decodethread.h"
#include <QDebug>
#include <QImage>
#include <QTimer>
#include <QPainter>
#include <QElapsedTimer>
M_DecodeThread::M_DecodeThread(QObject *parent):QThread(parent),
    StreamUrl(QString())
{
    stopped = false;
    m_audioIndex = -1;
    m_videoIndex = -1;
    m_FirstAudioFrame = true;
    m_FirstVideoFrame = true;
    m_video_time_base = 0;
    m_audio_time_base = 0;
    m_video_time_base = 0;
    m_video_start_pts = 0;
    FrameTime = 0;


}
uint8_t *audio_buffer = (uint8_t *)av_malloc(FRAME_SIZE*2);
void M_DecodeThread::run()
{
    QElapsedTimer elapsedtime;
    elapsedtime.start();
    while(!stopped){
        if(av_read_frame(m_AVFormatContext,m_AVPacket) == 0){
            if(m_AVPacket->stream_index == m_audioIndex){//音频
                avcodec_send_packet(m_audioCodecContext,m_AVPacket);
                while(avcodec_receive_frame(m_audioCodecContext,m_AVFrame) == 0){
                    //收到了解码的数据
                    double p;
                    if(m_FirstAudioFrame){
                        m_FirstAudioFrame = false;
                        m_audio_start_pts = m_AVFrame->pts;
                        p = m_audio_start_pts * m_audio_time_base;

                    }else{
                        p = (m_AVFrame->pts - m_audio_start_pts) * m_audio_time_base;
                    }
//                    qDebug()<<"audio pts : " << p << DATATIME;

                    int len = swr_convert(m_Swrcontext, &audio_buffer, FRAME_SIZE, (const uint8_t **)m_AVFrame->data, m_AVFrame->nb_samples);
                    if(len < 0 ){
                        continue;
                    }
                    m_audio_pts = p;
                    int data_size = av_samples_get_buffer_size(nullptr, 2,len, AV_SAMPLE_FMT_S16, 1);
                    QByteArray atemp =  QByteArray((const char *)audio_buffer, data_size);
                    Q_EMIT receiveAudio(atemp);
//                    Q_EMIT testAudio(atemp,p);

                }
            }else if(m_AVPacket->stream_index == m_videoIndex){//视频
                avcodec_send_packet(m_videoCodecContext,m_AVPacket);
                while(avcodec_receive_frame(m_videoCodecContext,m_AVFrame) == 0){
                    //收到了解码的数据
                    QImage image  = QImage(m_videoCodecContext->width,m_videoCodecContext->height,QImage::Format_RGB888);
                    int linesiez[1] = {image.bytesPerLine()};
                    uint8_t *data[1] = {image.bits()};
                    sws_scale(m_SwsContext,m_AVFrame->data,m_AVFrame->linesize,0,PixHeight,data,linesiez);
                    if(!image.isNull()){
                        Q_EMIT receiveImage(image);

                        if(m_FirstVideoFrame){
                            m_FirstVideoFrame = false;
                            m_video_start_pts = m_AVFrame->pts;
                            m_video_pts = m_video_start_pts * m_video_time_base;
                            qDebug()<<"m_video_start_pts = " << m_video_start_pts;
                        }else{
                            m_video_pts = (m_AVFrame->pts - m_video_start_pts) * m_video_time_base;
                        }
//                        qDebug()<<"video pts : " << m_video_pts <<"m_audio_pts : "<< m_audio_pts<< DATATIME;
                        int delay2 = m_video_pts - m_audio_pts;
                        int delay3 = 0;
                        if(delay2 < 0){
                            delay2 = 0;
                        }
                        int elapsed = elapsedtime.elapsed(); // 从上一帧到现在的时间
                        if (elapsed < FrameTime) {
                            delay3 = FrameTime - elapsed;
                        }
                        msleep(delay2 + delay3);
                        elapsedtime.restart();
                    }
                }
            }
        }
        av_packet_unref(m_AVPacket);
    }
    stopped = false;
    qDebug()<<"M_DecodeThread is stop";
}

bool M_DecodeThread::FFmpeg_Init()
{
    int result = 0;
    //初始化网络流设备
    avformat_network_init();

    //开辟空间，分配内存
    m_AVFormatContext = avformat_alloc_context();

    //打开流媒体并填充m_AVFormatContext
    result = avformat_open_input(&m_AVFormatContext,StreamUrl.toUtf8().data(),nullptr,nullptr);
    if(result < 0){
        qDebug()<<" open Stream fail";
        return false;
    }

    //获取流信息
    result = avformat_find_stream_info(m_AVFormatContext,nullptr);
    if(result < 0){
        qDebug()<<" find stream info fail";
        return false;
    }

    //查找音频流索引和视频流索引
    int streamNum = m_AVFormatContext->nb_streams;         //获取流数量
    for(int i=0;i<streamNum;i++){
        AVStream *t_AVStream = m_AVFormatContext->streams[i];
        if(t_AVStream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
            m_video_time_base = av_q2d(t_AVStream->time_base);
            StreamRate = (double)t_AVStream->avg_frame_rate.num / t_AVStream->avg_frame_rate.den;
            m_videoIndex = i;
            FrameTime = 1000/StreamRate;
            qDebug()<<"m_video_time_base = " << m_video_time_base << "StreamRate = " << StreamRate;
        }else if(t_AVStream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO){
            m_audio_time_base = av_q2d(t_AVStream->time_base);
            m_audioIndex = i;
            qDebug()<<"m_audio_time_base = " << m_audio_time_base;
        }
    }

    if(m_videoIndex == -1){
        qDebug()<<"Video streams are not included ";
    }else{
        //查找并打开视频解码器
        AVCodecParameters *t_VideoParameters = avcodec_parameters_alloc();
        t_VideoParameters = m_AVFormatContext->streams[m_videoIndex]->codecpar;
        PixHeight = t_VideoParameters->height;
        PixWidth = t_VideoParameters->width;
        const AVCodec *t_videoCodec = avcodec_find_decoder(t_VideoParameters->codec_id);   //获取流解码器
        m_videoCodecContext = avcodec_alloc_context3(t_videoCodec);
        avcodec_parameters_to_context(m_videoCodecContext,t_VideoParameters);
        if(avcodec_open2(m_videoCodecContext,t_videoCodec,NULL) < 0 ){
            qDebug()<<"find video decodec fail";
            return false;
        }

        m_SwsContext = sws_getContext(PixWidth,PixHeight,m_videoCodecContext->pix_fmt,PixWidth,PixHeight,AV_PIX_FMT_RGB24,SWS_BICUBIC,0,0,0);

        avcodec_parameters_free(&t_VideoParameters);
    }

    if(m_audioIndex == -1){
        qDebug()<<"Audio streams are not included";
    }else{
        //查找并打开音频解码器
        AVCodecParameters *t_audioParameters = avcodec_parameters_alloc();
        t_audioParameters = m_AVFormatContext->streams[m_audioIndex]->codecpar;
        const AVCodec *t_audioCodec = avcodec_find_decoder(t_audioParameters->codec_id);
        m_audioCodecContext = avcodec_alloc_context3(t_audioCodec);
        avcodec_parameters_to_context(m_audioCodecContext,t_audioParameters);
        if(avcodec_open2(m_audioCodecContext,t_audioCodec,NULL) < 0 ){
            qDebug()<<"find audio decodec fail";
            return false;
        }


        // 获取音频参数
        AVChannelLayout out_channel_layout = m_audioCodecContext->ch_layout;//通道布局
        AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
        const int out_sample_rate = m_audioCodecContext->sample_rate;//采样率
        const int out_channels = out_channel_layout.nb_channels;//通道数量

        //开辟swrcontext内存并初始化
        m_Swrcontext = nullptr;
        result = swr_alloc_set_opts2(&m_Swrcontext, &out_channel_layout, out_sample_fmt,out_sample_rate,
                                                  &m_audioCodecContext->ch_layout, m_audioCodecContext->sample_fmt, m_audioCodecContext->sample_rate,0,nullptr);
        if(result != 0){
            qDebug()<< "swr_alloc_set_opts2 fail";
            return false;
        }
        swr_init(m_Swrcontext);

        avcodec_parameters_free(&t_audioParameters);
        //初始化音频播放设备
        Q_EMIT AudioDevice_Init(out_sample_rate,out_channels,16,StreamRate);
    }

    m_AVPacket = av_packet_alloc();
    m_audioPacket = av_packet_alloc();
    m_AVFrame = av_frame_alloc();
    return true;
}

void M_DecodeThread::SetAudioPts(double pts)
{
    m_audio_pts = pts;
//    qDebug()<<"m_audio_pts = " << m_audio_pts;
}

#if 0
void M_DecodeThread::AudioDevice_Init(const int SampleRate, const int Channles, const int SampleFmt)
{
    QAudioFormat format;
    // Set up the format
    format.setSampleRate(SampleRate);
    format.setChannelCount(Channles);
    format.setSampleSize(SampleFmt);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);
    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    if (!info.isFormatSupported(format)) {
        qWarning() << "Raw audio format not supported by backend, cannot play audio.";
    }
    m_audiooutput = new QAudioOutput(format,this);
    m_audioIODevice = m_audiooutput->start();
}
#endif
M_DecodeWidegt::M_DecodeWidegt(QWidget *parent):QWidget(parent)
{
    clock = 0;
    m_audiotimer = new QTimer(this);
    m_audiotimer->setTimerType(Qt::PreciseTimer);
    connect(m_audiotimer,&QTimer::timeout,this,&M_DecodeWidegt::PlayAudio);
    m_decodethread = new M_DecodeThread(this);
    connect(m_decodethread,&M_DecodeThread::receiveImage,this,&M_DecodeWidegt::receiveImage);
    connect(m_decodethread,&M_DecodeThread::receiveAudio,this,&M_DecodeWidegt::receiveAudio);
    connect(m_decodethread,&M_DecodeThread::AudioDevice_Init,this,&M_DecodeWidegt::AudioDevice_Init);
    connect(m_decodethread,&M_DecodeThread::testAudio,this,&M_DecodeWidegt::audiotest);

}

void M_DecodeWidegt::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    QImage img = m_image;
    if(!img.isNull()){
        painter.drawImage(rect(),img);
    }
}

void M_DecodeWidegt::AudioDevice_Init(const int &SampleRate, const int &Channles, const int &SampleFmt, const int &Rate)
{
    this->m_StreamRate = Rate;
    QAudioFormat format;
    // Set up the format
    format.setSampleRate(SampleRate);
    format.setChannelCount(Channles);
    format.setSampleSize(SampleFmt);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);
    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    if (!info.isFormatSupported(format)) {
        qWarning() << "Raw audio format not supported by backend, cannot play audio.";
    }
    m_audiooutput = new QAudioOutput(format,this);
//    m_audiooutput->setBufferSize(20480);
    connect(m_audiooutput,&QAudioOutput::stateChanged,this,&M_DecodeWidegt::handleStateChanged);
//    m_audioIODevice = m_audiooutput->start();
    qDebug()<<m_audiooutput->state();
}

void M_DecodeWidegt::receiveImage(const QImage &image)
{
    if(!image.isNull()){
        this->m_image = image;
        update();
    }
}

void M_DecodeWidegt::receiveAudio(const QByteArray &audiodata)
{
    QMutexLocker locker(&m_mutex);
    if(!audiodata.isEmpty()){
        this->m_AudioData.append(audiodata);
    }
}

void M_DecodeWidegt::PlayAudio()
{
    QMutexLocker locker(&m_mutex);

//    if(testlist.isEmpty()) {
//        return;
//    }
    if((m_audiooutput) && (m_audiooutput->state() != QAudio::StoppedState) && (m_audiooutput->state() != QAudio::SuspendedState)){
#if 1
        int writeByte = qMin(m_AudioData.size(),m_audiooutput->bytesFree());
        m_audioIODevice->write(m_AudioData.data(),writeByte);
        m_AudioData = m_AudioData.right(m_AudioData.size() - writeByte);
#else
p1:
        QByteArray arr = testlist.at(0).first;
        double pts = testlist.at(0).second;
        if(m_audiooutput->bytesFree() > arr.size()){
//            qDebug()<< "m_audiooutput->bytesFree() = " << m_audiooutput->bufferSize() << "arr.size() = " << arr.size();
            m_audioIODevice->write(arr,arr.size());
            testlist.removeFirst();
            m_decodethread->SetAudioPts(pts);
            if(testlist.size() > 0){
                goto p1;
            }
        }
#endif
    }
}

void M_DecodeWidegt::audiotest(const QByteArray &audiodata, double pts)
{

    QMutexLocker locker(&m_mutex);
    if(!audiodata.isEmpty()){
        testlist.append(qMakePair(audiodata,pts));
    }
}

void M_DecodeWidegt::Start()
{
    m_decodethread->SetUrl(m_Url);
    bool ok = m_decodethread->FFmpeg_Init();
    if(ok){
        qDebug()<<"FFmpeg init success!" << DATATIME;
        m_decodethread->start();
//        m_audiotimer->start(1000/m_StreamRate);//这个好像不用按这个时间吧？可以快一点检测
        m_audiotimer->start(15);
        m_audioIODevice = m_audiooutput->start();
    }
}

void M_DecodeWidegt::SetUrl(QString file)
{
    this->m_Url = file;
}

void M_DecodeWidegt::handleStateChanged(QAudio::State state) {
    switch (state) {
    case QAudio::ActiveState:
        qDebug() << "Audio Output: Active";
        break;
    case QAudio::SuspendedState:
        qDebug() << "Audio Output: Suspended";
        break;
    case QAudio::StoppedState:
        qDebug() << "Audio Output: Stopped";
        break;
    case QAudio::IdleState:
        qDebug() << "Audio Output: Idle";
        break;
    default:
        qDebug() << "Audio Output: Unknown State";
        break;
    }
}
