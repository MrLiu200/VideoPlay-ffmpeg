#include "v2_decodewidget.h"
#include <QDebug>
v2_DecodeWidget::v2_DecodeWidget(QWidget *parent) : QWidget(parent)
{

}

void v2_DecodeWidget::Init(QString Url)
{
    m_Audio = new v2_M_Audio(this);
    connect(m_Audio,&v2_M_Audio::receiveAudio,this,&v2_DecodeWidget::receiveAudio);

    m_Video = new v2_M_Video(this);
    m_AudioData.clear();

    m_audiotimer = new QTimer;
    connect(m_audiotimer,&QTimer::timeout,this,&v2_DecodeWidget::AddPlayBuffer);

    this->m_Url = Url;
}

void v2_DecodeWidget::StartDecode()
{
    m_Audio->SetUrl(m_Url);
    if(m_Audio->InitFFmpeg()){
        m_Audio->start();
    }
}

double v2_DecodeWidget::GetClock() const
{
    return m_audiooutput->elapsedUSecs()/1000;
//    return this->clock;
}

void v2_DecodeWidget::SetClock(double value)
{
    this->clock = value;
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

        connect(m_audiooutput,&QAudioOutput::stateChanged,this,&v2_DecodeWidget::handleStateChanged);
}

void v2_DecodeWidget::receiveAudio(QByteArray &data)
{
    QMutexLocker lock(&mutex);
    if(!data.isEmpty()){
        m_AudioData.append(data);
    }
}

void v2_DecodeWidget::handleStateChanged(QAudio::State state)
{
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

void v2_DecodeWidget::AddPlayBuffer()
{
    QMutexLocker locker(&mutex);
    if((m_audiooutput) && (m_audiooutput->state() != QAudio::StoppedState) && (m_audiooutput->state() != QAudio::SuspendedState)){
        int writeByte = qMin(m_AudioData.size(),m_audiooutput->bytesFree());
        m_audioIODevice->write(m_AudioData.data(),writeByte);
        m_AudioData = m_AudioData.right(m_AudioData.size() - writeByte);
    }
}
