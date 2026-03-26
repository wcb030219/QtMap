#include "maptileloader.h"
#include <QDebug>
#include <QMessageBox>
#include <QNetworkReply>
#include <QPainter>
#include <QFont>
#include <QColor>
#include "mapengine.h"

MapTileLoader::MapTileLoader(QObject *parent) : QObject(parent)
{
    m_tileCache.setMaxCost(1000); // 增加缓存容量到500
    m_offlineMode = false;
    m_networkManager = new QNetworkAccessManager(this); // 初始化网络管理器
    m_mapEngine = MapEngineFactory::createEngine("amap");
    m_maxConcurrentRequest = 6;
    m_currentConcurrentRequest = 0;
    m_isDragging = false;

    // 初始化工作线程
    m_workerThread = new ImageWorkerThread(this);
    connect(m_workerThread, &ImageWorkerThread::taskCompleted, this, &MapTileLoader::onImageTaskCompleted);
    connect(m_workerThread, &ImageWorkerThread::taskFailed, this, &MapTileLoader::onImageTaskFailed);
    m_workerThread->start();
}

MapTileLoader::~MapTileLoader()
{
    if (m_workerThread) {
        m_workerThread->stop();
        m_workerThread->wait();
        delete m_workerThread;
    }
    if (m_mapEngine) {
        delete m_mapEngine;
    }
    if (m_networkManager) {
        delete m_networkManager;
    }
}

void MapTileLoader::loadTile(int zoom, int x, int y)
{
    // 检查缓存
    QString key = tileKey(zoom, x, y);
    if (m_tileCache.contains(key)) {
        emit tileLoaded(zoom, x, y, *m_tileCache.object(key));
        return;
    }
    
    // 使用地图引擎同步加载瓦片
    if (m_mapEngine) {
        QByteArray cacheData = m_mapEngine->loadTileFromCache(zoom,x,y); //
        if(!cacheData.isEmpty()){
            QPixmap pixmap;
            pixmap.loadFromData(cacheData);
            m_tileCache.insert(key,new QPixmap(pixmap));
            emit tileLoaded(zoom,x,y,pixmap);
            return;
        }
    }

    // 发送占位格
    QPixmap placeholder = createPlaceholder();
    m_tileCache.insert(key, new QPixmap(placeholder));
    emit tileLoaded(zoom, x, y, placeholder);

    if(m_pendingRequest.contains(key)){
        return;
    }

    m_requestQueue.enqueue(QPair<int,QPair<int,int>>(zoom,QPair<int,int>(x,y)));
    m_pendingRequest.insert(key);

    processNextRequest();

}

QPixmap MapTileLoader::getCachedTile(int zoom, int x, int y) const
{
    QString key = tileKey(zoom, x, y);
    if (m_tileCache.contains(key)) {
        return *m_tileCache.object(key);
    }
    return QPixmap();
}

void MapTileLoader::setMapEngine(mapengine *engine)
{
    if (m_mapEngine) {
        delete m_mapEngine;
    }
    m_mapEngine = engine;
    // 清空缓存，因为不同地图引擎的瓦片不同
    m_tileCache.clear();
}

mapengine *MapTileLoader::mapEngine() const
{
    return m_mapEngine;
}

void MapTileLoader::setOfflineMode(bool enabled)
{
    m_offlineMode = enabled;
    if (m_mapEngine) {
        m_mapEngine->setOfflineMode(enabled);
    }
}

bool MapTileLoader::offlineMode() const
{
    return m_offlineMode;
}

bool MapTileLoader::isDragging() const
{
    return m_isDragging;
}

void MapTileLoader::cancelAllRequests()
{
    // 清空请求队列
    m_requestQueue.clear();

    // 取消所有正在进行的网络请求
    for (QNetworkReply *reply : m_activeReplies) {
        if (reply && reply->isRunning()) {
            reply->abort();
        }
    }

    // 清空待处理请求集合
    m_pendingRequest.clear();

    // 重置当前并发请求数
    m_currentConcurrentRequest = 0;

    // 清空活跃请求列表
    m_activeReplies.clear();
}


void MapTileLoader::onReplyFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if(!reply) return;
    int zoom = reply->property("zoom").toInt();
    int x = reply->property("x").toInt();
    int y = reply->property("y").toInt();
    QString key = reply->property("key").toString();

    m_pendingRequest.remove(key);
    m_currentConcurrentRequest--;
    m_activeReplies.removeOne(reply);

    if(reply->error() == QNetworkReply::NoError){
        QByteArray data = reply->readAll();

        if(m_mapEngine){
            m_mapEngine->saveTileToCache(zoom,x,y,data);
        }
        
        // 将图片加载任务交给工作线程处理
        ImageWorkerThread::Task task;
        task.type = ImageWorkerThread::LoadImage;
        task.path = key;
        task.data = data;
        m_workerThread->addTask(task);
    }
    reply->deleteLater();

    processNextRequest();
}


void MapTileLoader::setDraggingState(bool isDragging)
{
    if(m_isDragging != isDragging){
        m_isDragging = isDragging;
        emit draggingStateChanged(isDragging);
    }
}


void MapTileLoader::onImageTaskCompleted(const QString &key, const QPixmap &pixmap)   //图片任务完成处理函数
{
    if (!pixmap.isNull()) {
        // 从key中解析出zoom, x, y
        QStringList parts = key.split('_');
        if (parts.size() == 3) {
            int zoom = parts[0].toInt();
            int x = parts[1].toInt();
            int y = parts[2].toInt();
            m_tileCache.insert(key, new QPixmap(pixmap));
            emit tileLoaded(zoom, x, y, pixmap);
        }
    }
}


void MapTileLoader::onImageTaskFailed(const QString &key, const QString &error)
{
    qDebug() << "Image task failed:" << key << error;
}


void MapTileLoader::processNextRequest()
{
    if(m_currentConcurrentRequest >= m_maxConcurrentRequest || m_requestQueue.isEmpty()) return;

    QPair<int,QPair<int,int>> request = m_requestQueue.dequeue();
    int zoom = request.first;
    int x = request.second.first;
    int y = request.second.second;
    QString key = tileKey(zoom,x,y);

    if(m_mapEngine){
        QString url = m_mapEngine->tileUrlTemplate();
        url.replace("{z}",QString::number(zoom));
        url.replace("{x}",QString::number(x));
        url.replace("{y}",QString::number(y));

        QNetworkRequest request(url);
        request.setTransferTimeout(5000);

        QNetworkReply *reply = m_networkManager->get(request);

        reply->setProperty("zoom",zoom);
        reply->setProperty("x",x);
        reply->setProperty("y",y);
        reply->setProperty("key",key);

        connect(reply,&QNetworkReply::finished,this,&MapTileLoader::onReplyFinished);
        m_activeReplies.append(reply);
        m_currentConcurrentRequest++;
    }
}

QString MapTileLoader::tileKey(int zoom, int x, int y) const
{
    return QString("%1_%2_%3").arg(zoom).arg(x).arg(y);
}

QPixmap MapTileLoader::createPlaceholder() const
{
    // 创建一个256x256的占位格图像
    QPixmap placeholder(256, 256);
    
    // 填充灰色背景
    placeholder.fill(QColor(230, 230, 230));
    
    // 绘制加载中文字
    QPainter painter(&placeholder);
    painter.setPen(QColor(100, 100, 100));
    painter.setFont(QFont("Arial", 12));
    painter.drawText(placeholder.rect(), Qt::AlignCenter, "加载中...");
    
    return placeholder;
}
