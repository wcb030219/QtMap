#include "mapengine.h"
#include <QNetworkAccessManager>

#include <QNetworkReply>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QEventLoop>


BaseMapEngine::BaseMapEngine(const QString &cachePathSuffix){
    m_networkManager = new QNetworkAccessManager;
    m_offlineMode = false;
    m_offlineCachePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)+ "/" + cachePathSuffix;

}

BaseMapEngine::~BaseMapEngine(){
    delete m_networkManager;
}

void BaseMapEngine::saveTileToCache(int zoom, int x, int y, const QByteArray &data)
{
    QString zoomDir = QString("%1/%2").arg(m_offlineCachePath).arg(zoom);
    QString xDir = QString("%1/%2").arg(zoomDir).arg(x);
    QDir dir;
    dir.mkpath(xDir);
    QString tilePath = QString("%1/%2.png").arg(xDir).arg(y);
    QFile file(tilePath);
    if(file.open(QIODevice::WriteOnly)){
        file.write(data);
        file.close();
    }
}

QByteArray BaseMapEngine::loadTileFromCache(int zoom, int x, int y)
{
    QString filePath = QString("%1/%2/%3/%4.png").arg(m_offlineCachePath).arg(zoom).arg(x).arg(y);
    QFile file(filePath);
    if(file.open(QIODevice::ReadOnly)){
        return file.readAll();
    }
    return QByteArray();
}

bool BaseMapEngine::supportsOffline() const
{
    return true;
}

void BaseMapEngine::setOfflineMode(bool enabled)
{
    m_offlineMode = enabled;
}

void BaseMapEngine::setOfflineCachePath(const QString &path)
{
    m_offlineCachePath = path;
}

class AmapEngine : public BaseMapEngine
{
public:
    AmapEngine() : BaseMapEngine("amap_cache") {}

    QString name() const override {
        return "Amap";
    }

    QString type() const override {
        return "standard";
    }

    QString tileUrlTemplate() const override {
        return "https://webst01.is.autonavi.com/appmaptile?x={x}&y={y}&z={z}&lang=zh_cn&size=1&scale=2&style=6";
    }
};

class MapboxEngine : public BaseMapEngine
{
public:
    MapboxEngine() : BaseMapEngine("mapbox_cache") {}

    QString name() const override {
        return "Mapbox";
    }

    QString type() const override {
        return "standard";
    }
    QString tileUrlTemplate() const override {
        return "https://api.mapbox.com/styles/v1/mapbox/streets-v11/tiles/256/{z}/{x}/{y}?access_token=YOUR_MAPBOX_ACCESS_TOKEN";
    }
};

class OsmEngine : public BaseMapEngine
{
public:
    OsmEngine() : BaseMapEngine("osm_cache") {}

    QString name() const override {
        return "OpenStreetMap";
    }

    QString type() const override {
        return "standard";
    }

    QString tileUrlTemplate() const override {
        return "https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png";
    }
};




mapengine *MapEngineFactory::createEngine(const QString &name)
{
    if (name.toLower() == "amap") {
        return new AmapEngine();
    } else if (name.toLower() == "mapbox") {
        return new MapboxEngine();
    } else if (name.toLower() == "osm") {
        return new OsmEngine();
    }
    return nullptr;
}

void mapengine::saveTileToCache(int zoom, int x, int y, const QByteArray &data)
{

}

QByteArray mapengine::loadTileFromCache(int zoom, int x, int y)
{
    return QByteArray();
}
