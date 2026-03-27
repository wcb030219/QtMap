#ifndef MAPENGINE_H
#define MAPENGINE_H


#include <QNetworkAccessManager>
#include <QPixmap>
#include <QString>

class mapengine    //支持多地图服务
{
public:
    mapengine(){}
    virtual ~mapengine(){}

    virtual void saveTileToCache(int zoom,int x,int y,const QByteArray &data);//将瓦片数据保存到本地缓存

    virtual QByteArray loadTileFromCache(int zoom,int x,int y);//从本地缓存加载瓦片数据

    virtual QString name() const = 0;   //获取地图引擎名

    virtual QString type() const = 0;   //获取地图类型

    virtual bool supportsOffline() const = 0;  //检查是否是离线状态

    virtual void setOfflineMode(bool enabled) = 0;  //设置离线状态

    virtual void setOfflineCachePath(const QString &path) = 0;   //设置离线缓存路径

    virtual QString tileUrlTemplate() const = 0;  //获取瓦片URL模板
   // QString m_offlineCachePath;
};

class BaseMapEngine : public mapengine{
public:
    BaseMapEngine(const QString &cachePathSuffix);
    virtual ~BaseMapEngine();

    void saveTileToCache(int zoom, int x, int y, const QByteArray &data) override;
    QByteArray loadTileFromCache(int zoom, int x, int y) override;
    bool supportsOffline() const override;
    void setOfflineMode(bool enabled) override;
    void setOfflineCachePath(const QString &path) override;

private:
    QNetworkAccessManager *m_networkManager; //网络管理器，用于发起HTTP请求获取地
    bool m_offlineMode;        // 离线模式标志，为true时优先使用缓
    QString m_offlineCachePath;    // 离线缓存文件存储路径
};

class MapEngineFactory
{
public:
    static mapengine *createEngine(const QString &name);     //创建地图引擎实例
};

#endif // MAPENGINE_H
