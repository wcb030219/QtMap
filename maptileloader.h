#ifndef MAPTILELOADER_H
#define MAPTILELOADER_H

#include <QObject>
#include <QCache>
#include <QPixmap>
#include <QNetworkAccessManager>
#include <QQueue>
#include "mapengine.h"
#include "imageworkerthread.h"

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

    bool isDragging() const; //获取当前拖拽状态
    void cancelAllRequests(); //取消所有待处理的请求

signals:
    void tileLoaded(int zoom, int x, int y, const QPixmap &pixmap);  //瓦片加载完成信号
    void draggingStateChanged(bool isDragging);      //拖拽状态改变信号

    void cancelAllRequest();        //取消所有待处理的请求

public slots:
    void onReplyFinished();      //网络请求完成处理
    void setDraggingState(bool isDragging);        //拖拽状态改变信号
    void onImageTaskCompleted(const QString &key, const QPixmap &pixmap); // 图片任务完成处理
    void onImageTaskFailed(const QString &key, const QString &error); // 图片任务失败处理

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
    bool m_isDragging; //地图是否正在被拖拽
    QList<QNetworkReply*> m_activeReplies; // 正在进行的网络请求
    ImageWorkerThread *m_workerThread; // 工作线程

    void processNextRequest();     //处理下一个待加载的瓦片请求
    QString tileKey(int zoom, int x, int y) const;  //生成瓦片缓存键
    QPixmap createPlaceholder() const; //创建占位格图像
};

#endif // MAPTILELOADER_H
