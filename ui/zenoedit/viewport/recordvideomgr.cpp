#include "recordvideomgr.h"
#include <viewport/zenovis.h>
#include <viewport/viewportwidget.h>
#include <zenovis/DrawOptions.h>
#include <zeno/utils/format.h>
#include <zeno/utils/log.h>
#include <util/log.h>
#include <zeno/core/Session.h>
#include <zeno/extra/GlobalState.h>
#include <zeno/extra/GlobalComm.h>


RecordVideoMgr::RecordVideoMgr(QObject* parent)
    : QObject(parent)
    , m_timer(nullptr)
{
}

RecordVideoMgr::~RecordVideoMgr()
{
    cancelRecord();
}

Zenovis* RecordVideoMgr::getZenovis()
{
    DisplayWidget* pWid =  qobject_cast<DisplayWidget *>(parent());
    ZASSERT_EXIT(pWid, nullptr);
    ViewportWidget* viewport = pWid->getViewportWidget();
    ZASSERT_EXIT(viewport, nullptr);
    return viewport->getZenoVis();
}

void RecordVideoMgr::cancelRecord()
{
    if (m_timer)
        m_timer->stop();
    //Zenovis::GetInstance().blockSignals(false);
}

void RecordVideoMgr::setRecordInfo(const VideoRecInfo& recInfo)
{
    m_recordInfo = recInfo;
    m_currFrame = m_recordInfo.frameRange.first;

    //create directory to store screenshot pngs.
    QDir dir(m_recordInfo.record_path);
    ZASSERT_EXIT(dir.exists());
    dir.mkdir("P");
    // remove old image
    {
        QString dir_path = m_recordInfo.record_path + "/P/";
        QDir qDir = QDir(dir_path);
        qDir.setNameFilters(QStringList("*.jpg"));
        QStringList fileList = qDir.entryList(QDir::Files | QDir::NoDotAndDotDot);
        for (auto i = 0; i < fileList.size(); i++) {
            qDir.remove(fileList.at(i));
        }
    }

    if (!m_recordInfo.bRecordAfterRun)
    {
        Zenovis* pVis = getZenovis();
        bool ret = connect(pVis, SIGNAL(frameDrawn(int)), this, SLOT(onFrameDrawn(int)));
    }
    else
    {
        m_timer = new QTimer(this);
        connect(m_timer, SIGNAL(timeout()), this, SLOT(recordFrame()));
        m_timer->start(0);
    }
}

void RecordVideoMgr::finishRecord()
{
    if (m_timer)
        m_timer->stop();
    if (!m_recordInfo.bExportVideo) {
        emit recordFinished();
        return;
    }
    //Zenovis::GetInstance().blockSignals(false);
    {
        QString dir_path = m_recordInfo.record_path + "/P/";
        QDir qDir = QDir(dir_path);
        qDir.setNameFilters(QStringList("*.jpg"));
        QStringList fileList = qDir.entryList(QDir::Files | QDir::NoDotAndDotDot);
        fileList.sort();
        for (auto i = 0; i < fileList.size(); i++) {
            auto new_name = dir_path + zeno::format("{:07d}.jpg", i).c_str();
            auto old_name = dir_path + fileList.at(i);
            QFile::rename(old_name,new_name);
        }
    }
    QString imgPath = m_recordInfo.record_path + "/P/%07d.jpg";
    QString outPath = m_recordInfo.record_path + "/" + m_recordInfo.videoname;

    QString cmd = QString("ffmpeg -y -r %1 -i %2 -b:v %3k -c:v mpeg4 %4")
              .arg(m_recordInfo.fps)
              .arg(imgPath)
              .arg(m_recordInfo.bitrate)
              .arg(outPath);
    int ret = QProcess::execute(cmd);
    if (ret == 0)
    {
        if (!m_recordInfo.audioPath.isEmpty()) {
            cmd = QString("ffmpeg -y -i %1 -i %2 -c:v copy -c:a aac output_av.mp4")
                      .arg(outPath)
                      .arg(m_recordInfo.audioPath);
            ret = QProcess::execute(cmd);
            if (ret == 0)
                emit recordFinished();
            else
                emit recordFailed(QString());
        }
        emit recordFinished();
    }
    else
    {
        //todo get the error string from QProcess.
        emit recordFailed(QString());
    }
}

void RecordVideoMgr::recordFrame()
{
    if (m_currFrame > m_recordInfo.frameRange.second)
    {
        finishRecord();
        return;
    }

    Zenovis* pVis = getZenovis();
    ZASSERT_EXIT(pVis);

    pVis->setCurrentFrameId(m_currFrame);
    pVis->paintGL();

    auto record_file = zeno::format("{}/P/{:07d}.jpg", m_recordInfo.record_path.toStdString(), m_currFrame);

    auto scene = pVis->getSession()->get_scene();
    auto old_num_samples = scene->drawOptions->num_samples;
    scene->drawOptions->num_samples = m_recordInfo.numOptix;
    scene->drawOptions->msaa_samples = m_recordInfo.numMSAA;

    auto [x, y] = pVis->getSession()->get_window_size();

    auto extname = QFileInfo(QString::fromStdString(record_file)).suffix().toStdString();
    pVis->getSession()->set_window_size((int)m_recordInfo.res.x(), (int)m_recordInfo.res.y());
    pVis->getSession()->do_screenshot(record_file, extname);
    pVis->getSession()->set_window_size(x, y);
    scene->drawOptions->num_samples = old_num_samples;

    m_pics.append(QString::fromStdString(record_file));

    emit frameFinished(m_currFrame);

    m_currFrame++;
}

void RecordVideoMgr::onFrameDrawn(int currFrame)
{
    auto& pGlobalComm = zeno::getSession().globalComm;
    ZASSERT_EXIT(pGlobalComm);

    bool bFrameCompleted = pGlobalComm->isFrameCompleted(currFrame);
    bool bFrameRecorded = m_recordInfo.m_bFrameFinished[currFrame];

    Zenovis* pVis = getZenovis();
    ZASSERT_EXIT(pVis);

    if (bFrameCompleted && !bFrameRecorded)
    {
        if (currFrame >= m_recordInfo.frameRange.first && currFrame <= m_recordInfo.frameRange.second)
        {
            auto record_file = zeno::format("{}/P/{:07d}.jpg", m_recordInfo.record_path.toStdString(), currFrame);
            QFileInfo fileInfo(QString::fromStdString(record_file));

            auto scene = pVis->getSession()->get_scene();
            auto old_num_samples = scene->drawOptions->num_samples;
            scene->drawOptions->num_samples = m_recordInfo.numSamples;
            scene->drawOptions->msaa_samples = m_recordInfo.numMSAA;

            auto [x, y] = pVis->getSession()->get_window_size();

            auto extname = QFileInfo(QString::fromStdString(record_file)).suffix().toStdString();
            pVis->getSession()->set_window_size((int)m_recordInfo.res.x(), (int)m_recordInfo.res.y());
            pVis->getSession()->do_screenshot(record_file, extname);
            pVis->getSession()->set_window_size(x, y);
            scene->drawOptions->num_samples = old_num_samples;

            m_recordInfo.m_bFrameFinished[currFrame] = true;
        }

        if (currFrame == m_recordInfo.frameRange.second)
        {
            finishRecord();

            //clear issues:
            m_recordInfo = VideoRecInfo();
        }
    }
}