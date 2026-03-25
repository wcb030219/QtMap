#ifndef MAPENGINE_H
#define MAPENGINE_H


#include <QPixmap>
#include <QString>

class mapengine    //支持多地图服务
{
public:
    mapengine(){}
    virtual ~mapengine(){}


    //virtual QPixmap getTile(int zoom,int x,int y) = 0;   //获取瓦片

    virtual void saveTileToCache(int zoom,int x,int y,const QByteArray &data);

    virtual QByteArray loadTileFromCache(int zoom,int x,int y);

    virtual QString name() const = 0;   //获取地图引擎名

    virtual QString type() const = 0;   //获取地图类型

    virtual bool supportsOffline() const = 0;  //检查是否是离线状态

    virtual void setOfflineMode(bool enabled) = 0;  //设置离线状态

    virtual void setOfflineCachePath(const QString &path) = 0;   //设置离线缓存路径

    virtual QString tileUrlTemplate() const = 0;  //获取瓦片URL模板

};

class MapEngineFactory
{
public:
    static mapengine *createEngine(const QString &name);
};

#endif // MAPENGINE_H
