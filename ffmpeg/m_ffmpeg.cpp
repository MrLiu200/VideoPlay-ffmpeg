#include "m_ffmpeg.h"
//#include <QDat>
M_FFmpeg::M_FFmpeg(QObject *parent) : QThread(parent)
{
    this->stopped = false;
    Url = "";
    frameCount = 0;
    interval = 3;
    IsPlayout = false;

}

bool M_FFmpeg::init(bool IsNetUrl)
{
    if(IsNetUrl){
        avformat_network_init();
    }

    //申请一个AVFormatContext结构的内存
    m_AVFormatContext = avformat_alloc_context();
//    QString filename = "D:/project/Qt_project/Example/MySrc/Myffmpeg/build-MyFFmpeg-Desktop_Qt_5_14_2_MinGW_64_bit-Debug/debug/foreman.mp4";
//    QString filename = "D:/project/Qt_project/Example/MySrc/Myffmpeg/build-MyFFmpeg-Desktop_Qt_5_14_2_MinGW_64_bit-Debug/debug/video.mp4";

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
    //打印出文件流中的所有流类型
    for (unsigned int i = 0; i < m_AVFormatContext->nb_streams; i++) {
        AVStream *stream = m_AVFormatContext->streams[i];
        qDebug() << "Stream index:" << i
                 << " Codec type:" << stream->codecpar->codec_type
                 << " Codec ID:" << stream->codecpar->codec_id;
    }
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
#ifndef USE_AUDIO
            break;
#endif
        }
    }
    if (videoIndex == -1)
    {
        printf("Didn't find a video stream.\n");
        return false;
    }
//    m_AVCodecContext->rc_buffer_size = 1024 * 1024 * 10; // 例如，设置为 10 MB

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

    //打印文件信息
//    qDebug()<<"------------------------------FILE Information-----------------------------------";
//    av_dump_format(m_AVFormatContext, 0, Url.toUtf8().data(), 0);
//    qDebug()<<"---------------------------------------------------------------------------------";

//    设置解码线程和类型
//    m_AVCodecContext->thread_count = 8;
//    m_AVCodecContext->thread_type = FF_THREAD_FRAME;

    avcodec_parameters_free(&m_AVCodecParameters);
    return true;
}

void M_FFmpeg::run()
{
    QElapsedTimer elapsedtime;
    elapsedtime.start();

    while (!stopped) {

        int delay = 1000/frameRate;
//        qDebug()<< "delay = "<<delay;
        QMutexLocker locker(&mutex);
        if(av_read_frame(m_AVFormatContext,m_AVPacket) >= 0){//读取一个包的数据
            if(m_AVPacket->stream_index == videoIndex){//判断是否为数据流
                if(avcodec_send_packet(m_AVCodecContext,m_AVPacket) == 0){//发送去解码
                    if(avcodec_receive_frame(m_AVCodecContext,m_AVFrame) == 0){//获取解码数据
//                        frameCount++;
//                        if(frameCount != interval){
//                            av_packet_unref(m_AVPacket);//擦除数据包内容
//                            msleep(1);
//                            continue;
//                        }else{
//                            frameCount = 0;
//                        }
                        qDebug()<<"pts = " << m_AVFrame->best_effort_timestamp << "dts = " << m_AVPacket->dts << "系统时间:" <<QTime::currentTime().toString("HH:mm:ss.zzz");
                        QImage image = QImage(m_AVCodecContext->width,m_AVCodecContext->height,QImage::Format_RGB888);
                        int linesize[1] = { image.bytesPerLine() };
                        uint8_t *data[1] = { image.bits() };
                        sws_scale(m_SwsContext,m_AVFrame->data,m_AVFrame->linesize,0,frameheight,data,linesize);
                        if(!image.isNull()){
                            Q_EMIT receiveImage(image);
#if 1
                            int elapsed = elapsedtime.elapsed(); // 从上一帧到现在的时间
                            if (elapsed < delay) {
                                msleep(delay - elapsed); // 等待适当的时间
                            }

                            elapsedtime.restart(); // 重启计时器
#else
                            msleep(delay);
#endif
                        }
                    }
//                    qDebug() <<  "use time" << elapsedtime.elapsed();
                }
            }
        }

        av_packet_unref(m_AVPacket);//擦除数据包内容
//        msleep(delay);
    }
    stopped = false;
    qDebug() << "Stop FFmpeg Thread";
}

void M_FFmpeg::stop()
{
    stopped = true;
    msleep(1);
}

void M_FFmpeg::close()
{
    qDebug()<<"isRunning == "<<isRunning();
    av_packet_free(&m_AVPacket);
    av_frame_free(&m_AVFrame);
    sws_freeContext(m_SwsContext);
    qDebug()<<"opkok";

    avcodec_free_context(&m_AVCodecContext);

    qDebug() << "avcodec_free_context";
    avformat_close_input(&m_AVFormatContext);
    qDebug()<<"avformat_close_input";
    avformat_free_context(m_AVFormatContext);
    qDebug()<<"avformat_free_context";


}

void M_FFmpeg::setUrl(QString NewUrl)
{
    this->Url = NewUrl;
}

QString M_FFmpeg::readUrl()
{
    return this->Url;
}

M_FFmpegWidget::M_FFmpegWidget(QWidget *parent):QWidget(parent)
{
    m_image = QImage();
    m_ffmpeg = new M_FFmpeg(this);
    connect(m_ffmpeg,&M_FFmpeg::receiveImage,this,&M_FFmpegWidget::receiveImage);
    connect(m_ffmpeg,&M_FFmpeg::PlayOut,this,&M_FFmpegWidget::PlayOut);
    count = 0;
}

M_FFmpegWidget::~M_FFmpegWidget()
{
    m_ffmpeg->stop();
    m_ffmpeg->close();
}
#if 1
void M_FFmpegWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    QImage img = m_image;
    if(!img.isNull()){
        painter.drawImage(rect(), img);
//        count++;
//        qDebug() <<"刷新第 " << count << " 张"<<  "use time" << elapsedtime.elapsed();
    }

}
#endif
void M_FFmpegWidget::open()
{
    m_ffmpeg->setUrl(FrameUrl);
    bool ok = m_ffmpeg->init();
    if(ok){
        elapsedtime.start();
        m_ffmpeg->start();
    }
}

void M_FFmpegWidget::SetFrameUrl(QString url)
{
    this->FrameUrl = url;
}

void M_FFmpegWidget::setImage(const QImage &image)
{
    this->m_image = image;
}

void M_FFmpegWidget::PlayOut()
{
    if(m_ffmpeg->isRunning()){
        m_ffmpeg->stop();
        if(m_ffmpeg->isRunning()){
            m_ffmpeg->wait();
            qDebug() << "wait ok";

        }

        m_ffmpeg->close();

        qDebug() << "视频播放完毕";
    }

}

void M_FFmpegWidget::receiveImage(const QImage &image)
{
    if(!image.isNull()){
        this->m_image= image;
        update();
    }
#if USE_DEBUG
    count++;
    qDebug()<<"第 " << count <<"次触发了receiveImage信号";
#endif
}
