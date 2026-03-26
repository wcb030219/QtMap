#ifndef IMAGEWORKERTHREAD_H
#define IMAGEWORKERTHREAD_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QMap>
#include <QPair>
#include <QPixmap>
#include <QVariant>
#include <QQueue>

class ImageWorkerThread : public QThread
{
    Q_OBJECT

public:
    enum TaskType {
        LoadImage,  // 从二进制数据加载图片
        SaveImage,   // 把图片数据保存到文件
        ProcessImage  // 图片处理（缩放/旋转/滤镜等）
    };

    struct Task {
        TaskType type;   // 任务类型（必须）
        QString path;  // 文件路径：加载/保存/处理的目标路径
        QByteArray data;   // 图片二进制数据（核心数据）
        QMap<QString, QVariant> parameters;   // 扩展参数（缩放比例、角度等）
    };

    explicit ImageWorkerThread(QObject *parent = nullptr);
    ~ImageWorkerThread();

    void addTask(const Task &task);//添加任务到队列
    void stop();

signals:
    void taskCompleted(const QString &path, const QPixmap &result);
    void taskFailed(const QString &path, const QString &error);

protected:
    void run() override;

private:
    QQueue<Task> m_taskQueue;    // 任务队列：存储待处理的图片任务
    QMutex m_mutex;      // 互斥锁：保护共享数据（任务队列、停止标志）
    QWaitCondition m_condition;    // 条件变量：线程休眠/唤醒
    bool m_stop;   // 停止标志：控制线程退出

    void processTask(const Task &task);  //处理单个任务
    QPixmap loadImageFromData(const QByteArray &data);//从数据加载图片
    bool saveImageToFile(const QPixmap &pixmap, const QString &path);//保存图片到文件
};

#endif // IMAGEWORKERTHREAD_H
