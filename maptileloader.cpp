#include "maptileloader.h"
#include <QDebug>
#include <QMessageBox>
#include <QNetworkReply>
#include "mapengine.h"

MapTileLoader::MapTileLoader(QObject *parent) : QObject(parent)
{
    m_tileCache.setMaxCost(1000); // 增加缓存容量到500
    m_offlineMode = false;
    m_networkManager = new QNetworkAccessManager(this); // 初始化网络管理器
    m_mapEngine = MapEngineFactory::createEngine("amap");
    m_maxConcurrentRequest = 6;
    m_currentConcurrentRequest = 0;

    
}

MapTileLoader::~MapTileLoader()
{
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

    if(reply->error() == QNetworkReply::NoError){
        QByteArray data = reply->readAll();

        if(m_mapEngine){
            m_mapEngine->saveTileToCache(zoom,x,y,data);

        }
        QPixmap pixmap;
        pixmap.loadFromData(data);
        if(!pixmap.isNull()){
            m_tileCache.insert(key,new QPixmap(pixmap));
            emit tileLoaded(zoom,x,y,pixmap);
        }
    }
    reply->deleteLater();

    processNextRequest();
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
        m_currentConcurrentRequest++;
    }
}

QString MapTileLoader::tileKey(int zoom, int x, int y) const
{
    return QString("%1_%2_%3").arg(zoom).arg(x).arg(y);
}
