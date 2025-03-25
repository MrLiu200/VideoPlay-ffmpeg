#include "v2_m_video.h"

v2_M_Video::v2_M_Video(v2_DecodeWidget *DecodeWidget, QObject *parent) : QThread(parent)
{
    this->stopped = false;
    Url = "";
    frameCount = 0;
    interval = 3;
    IsPlayout = false;
    decodewidget = DecodeWidget;
}

bool v2_M_Video::init(bool IsNetUrl)
{
    if(IsNetUrl){
        avformat_network_init();
    }

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

    // 查找视频流的起始索引位置（nb_streams表示视音频流的个数）
    videoIndex = -1;

    //找出视频流的索引
    for (int i = 0; i < (int)m_AVFormatContext->nb_streams; i++)
    {
        AVStream *stream = m_AVFormatContext->streams[i];
        // 查找到视频流时退出循环
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) // 判断是否为视频流
        {
            videoIndex = i;

            //计算帧率
            AVRational avgframeRate = stream->avg_frame_rate;
            if (avgframeRate.num != 0 && avgframeRate.den != 0) {
                frameRate = (double)avgframeRate.num / avgframeRate.den; // 计算帧率
                qDebug() << "Video frame rate:" << frameRate;
            }
            m_video_time_base = av_q2d(stream->time_base);
            break;
        }
    }
    if (videoIndex == -1)
    {
        printf("Didn't find a video stream.\n");
        return false;
    }

    //创建帧结构，此函数仅分配基本结构空间，图像数据空间需通过av_malloc分配
    m_AVFrame = av_frame_alloc();

    //查找解码器
    AVCodecParameters *m_AVCodecParameters = avcodec_parameters_alloc();
    // 获取视频流编码结构
    m_AVCodecParameters = m_AVFormatContext->streams[videoIndex]->codecpar;

    frameheight = m_AVCodecParameters->height;
    framewidth = m_AVCodecParameters->width;
    //获取视频流解码器
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

    //初始化m_SwsContext      SWS_BICUBIC     SWS_FAST_BILINEAR
    m_SwsContext = sws_getContext(framewidth,frameheight,m_AVCodecContext->pix_fmt,framewidth,frameheight,AV_PIX_FMT_RGB24,SWS_BICUBIC,0,0,0);

    avcodec_parameters_free(&m_AVCodecParameters);
    return true;
}

void v2_M_Video::run()
{
    QElapsedTimer elapsedtime;
    elapsedtime.start();
    int delay = 1000/frameRate;
    while (!stopped) {
        QMutexLocker locker(&mutex);
        if(av_read_frame(m_AVFormatContext,m_AVPacket) >= 0){//读取一个包的数据
            if(m_AVPacket->stream_index == videoIndex){//判断是否为数据流
                if(avcodec_send_packet(m_AVCodecContext,m_AVPacket) == 0){//发送去解码
                    if(avcodec_receive_frame(m_AVCodecContext,m_AVFrame) == 0){//获取解码数据
                        QImage image = QImage(m_AVCodecContext->width,m_AVCodecContext->height,QImage::Format_RGB888);
                        int linesize[1] = { image.bytesPerLine() };
                        uint8_t *data[1] = { image.bits() };
                        sws_scale(m_SwsContext,m_AVFrame->data,m_AVFrame->linesize,0,frameheight,data,linesize);
                        if(!image.isNull()){
#if 1
                            double mainclock = decodewidget->testclock();//decodewidget->GetClock()
                            double pts = m_AVFrame->pts * m_video_time_base;
                            qDebug()<<"mainclock :" << mainclock << "video pts :" <<pts << CURRENTTIME;
                            double delay2 = pts - mainclock;
                            if(delay2 < 0){
                                delay2 = 0;
                                continue;
                            }
#endif
                            int elapsed = elapsedtime.elapsed(); // 从上一帧到现在的时间
                            double delay3 = delay - elapsed;
                            if (delay3 < 0) {
                                delay3 = 0;
                            }
                            msleep(delay2 + delay3);
                            Q_EMIT receiveImage(image);
                            elapsedtime.restart(); // 重启计时器
                        }
                    }
                }
            }
        }
        av_packet_unref(m_AVPacket);//擦除数据包内容
    }
    stopped = false;
    qDebug() << "Stop FFmpeg Thread";
}

void v2_M_Video::stop()
{
    stopped = true;
    msleep(1);
}

void v2_M_Video::close()
{
    av_packet_free(&m_AVPacket);
    av_frame_free(&m_AVFrame);
    sws_freeContext(m_SwsContext);
    avcodec_free_context(&m_AVCodecContext);
    avformat_close_input(&m_AVFormatContext);
    avformat_free_context(m_AVFormatContext);
}

void v2_M_Video::setUrl(QString NewUrl)
{
    this->Url = NewUrl;
}

QString v2_M_Video::readUrl() const
{
    return this->Url;
}


/*--------------------------------------------------------------------------------------------------------------------*/
v2_DecodeWidget::v2_DecodeWidget(QWidget *parent) : QWidget(parent)
{

}

void v2_DecodeWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    QImage img = m_image;
    if(!img.isNull()){
        painter.drawImage(rect(), img);
    }
}

void v2_DecodeWidget::Init(QString Url)
{
    m_Audio = new v2_M_Audio(this);
    connect(m_Audio,&v2_M_Audio::receiveAudio,this,&v2_DecodeWidget::receiveAudio);
    connect(m_Audio,&v2_M_Audio::AudioDevice_Init,this,&v2_DecodeWidget::AudioDevice_Init);

    m_Video = new v2_M_Video(this,this);
    connect(m_Video,&v2_M_Video::receiveImage,this,&v2_DecodeWidget::receiveImage);

    m_AudioData.clear();

    m_audiotimer = new QTimer;
    m_audiotimer->setTimerType(Qt::PreciseTimer);
    connect(m_audiotimer,&QTimer::timeout,this,&v2_DecodeWidget::AddPlayBuffer);

    this->m_Url = Url;
    IsPause = false;
    pauseclock = 0;
    endpauseclock = 0;
    startpauseclock = 0;
    clock = 0;
    First = true;
}

void v2_DecodeWidget::StartDecode()
{
    m_Audio->SetUrl(m_Url);
    if(m_Audio->InitFFmpeg()){
        m_Audio->start();
        qDebug()<<"m_Audio init ok";
    }
    m_Video->setUrl(m_Url);
    if(m_Video->init()){
        m_Video->start();
        qDebug()<<"m_Video init ok";
    }

}

double v2_DecodeWidget::GetClock() const
{
    return this->clock;
}

void v2_DecodeWidget::SetClock(double value)
{
    this->clock = value;
}

double v2_DecodeWidget::testclock()
{
    double a =  (double)m_audiooutput->elapsedUSecs()/1000000;// - pauseclock
    return a;
}

void v2_DecodeWidget::AudioDevice_Init(const int &SampleRate, const int &Channles, const int &SampleFmt)
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
        connect(m_audiooutput,&QAudioOutput::stateChanged,this,&v2_DecodeWidget::handleStateChanged);
        m_audiotimer->start(15);
}

void v2_DecodeWidget::receiveAudio(const QByteArray &data)
{
    QMutexLocker lock(&mutex);
    if(!data.isEmpty()){
        m_AudioData.append(data);
    }
}

void v2_DecodeWidget::handleStateChanged(QAudio::State state)
{
    if(IsPause && state!= QAudio::SuspendedState){
        IsPause = false;
        endpauseclock = m_audiooutput->elapsedUSecs()/1000;
        pauseclock += endpauseclock - startpauseclock;
    }
    switch (state) {
        case QAudio::ActiveState:
            if(First){
                First = false;
                pauseclock = m_audiooutput->elapsedUSecs()/1000;
            }
            qDebug() << "Audio Output: Active";
            break;
        case QAudio::SuspendedState:
            qDebug() << "Audio Output: Suspended";
            startpauseclock  = m_audiooutput->elapsedUSecs()/1000;
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

void v2_DecodeWidget::receiveImage(const QImage &image)
{
    if(!image.isNull()){
        m_image = image;
        update();
    }
}

void v2_DecodeWidget::AddPlayBuffer()
{
    QMutexLocker locker(&mutex);
    if((m_audiooutput) && (m_audiooutput->state() != QAudio::StoppedState) && (m_audiooutput->state() != QAudio::SuspendedState)){
        int writeByte = qMin(m_AudioData.size(),m_audiooutput->bytesFree());
        m_audioIODevice->write(m_AudioData.data(),writeByte);
        m_AudioData = m_AudioData.right(m_AudioData.size() - writeByte);

    }
}
