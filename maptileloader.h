#ifndef MAPTILELOADER_H
#define MAPTILELOADER_H

#include <QObject>
#include <QCache>
#include <QPixmap>
#include <QNetworkAccessManager>
#include <QQueue>
#include "mapengine.h"

class MapTileLoader : public QObject      //负责地图瓦片的加载和缓存
{
    Q_OBJECT

public:
    explicit MapTileLoader(QObject *parent = nullptr);
    ~MapTileLoader();

    void loadTile(int zoom, int x, int y); //加载瓦片


    QPixmap getCachedTile(int zoom, int x, int y) const;  //获取缓存的瓦片


    void setMapEngine(mapengine *engine); //设置地图引擎


    mapengine *mapEngine() const; //获取当前地图引擎


    void setOfflineMode(bool enabled); //设置离线模式


    bool offlineMode() const; //获取是否启用离线模式

signals:

    void tileLoaded(int zoom, int x, int y, const QPixmap &pixmap);  //瓦片加载完成信号

public slots:
    void onReplyFinished();

private:
    QCache<QString, QPixmap> m_tileCache; ///< 瓦片缓存
    mapengine *m_mapEngine; ///< 地图引擎
    bool m_offlineMode; ///< 是否启用离线模式
    bool m_errorShown; ///< 错误是否已经显示过
    QNetworkAccessManager *m_networkManager; ///< 网络管理器
    QSet<QString> m_pendingRequest; ///< 正在进行的请求
    QQueue<QPair<int,QPair<int,int>>> m_requestQueue; //请求队列
    int m_maxConcurrentRequest; //最大并发请求数
    int m_currentConcurrentRequest; //当前并发请求数

    void processNextRequest();

    QString tileKey(int zoom, int x, int y) const;  //生成瓦片缓存键
};

#endif // MAPTILELOADER_H
