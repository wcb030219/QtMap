#include "imageworkerthread.h"
#include <QImage>
#include <QFile>


ImageWorkerThread::ImageWorkerThread(QObject *parent)
    : QThread(parent), m_stop(false)
{
}


ImageWorkerThread::~ImageWorkerThread()
{
    stop();
    wait();
}


void ImageWorkerThread::addTask(const Task &task) //添加任务到队列
{
    QMutexLocker locker(&m_mutex);
    m_taskQueue.enqueue(task);
    m_condition.wakeOne();
}


void ImageWorkerThread::stop()
{
    QMutexLocker locker(&m_mutex);
    m_stop = true;
    m_condition.wakeOne();
}

void ImageWorkerThread::run()
{
    while (!m_stop) {
        Task task;
        {
            QMutexLocker locker(&m_mutex);  // 加锁：操作共享的任务队列和停止标志
            if (m_taskQueue.isEmpty()) {
                m_condition.wait(&m_mutex);
            }
            if (m_stop || m_taskQueue.isEmpty()) {
                continue;
            }
            task = m_taskQueue.dequeue();
        }
        processTask(task);
    }
}

void ImageWorkerThread::processTask(const Task &task)
{
    switch (task.type) {
    case LoadImage: {
        QPixmap pixmap = loadImageFromData(task.data);
        if (!pixmap.isNull()) {
            emit taskCompleted(task.path, pixmap);
        } else {
            emit taskFailed(task.path, "Failed to load image");
        }
        break;
    }
    case SaveImage: {
        QPixmap pixmap;
        if (pixmap.loadFromData(task.data)) {
            if (saveImageToFile(pixmap, task.path)) {
                emit taskCompleted(task.path, pixmap);
            } else {
                emit taskFailed(task.path, "Failed to save image");
            }
        } else {
            emit taskFailed(task.path, "Failed to load image data");
        }
        break;
    }
    case ProcessImage:
        // 实现图片处理逻辑
        break;
    }
}


QPixmap ImageWorkerThread::loadImageFromData(const QByteArray &data)//从数据加载图片
{
    QPixmap pixmap;
    pixmap.loadFromData(data);
    return pixmap;
}


bool ImageWorkerThread::saveImageToFile(const QPixmap &pixmap, const QString &path)//保存图片到文件
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    bool result = pixmap.save(&file, "PNG");
    file.close();
    return result;
}
