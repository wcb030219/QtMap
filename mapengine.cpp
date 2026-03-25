#include "mapengine.h"
#include <QNetworkAccessManager>

#include <QNetworkReply>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QEventLoop>


//高德

class AmapEngine : public mapengine
{
public:
    AmapEngine() {
        m_networkManager = new QNetworkAccessManager();
        m_offlineMode = false;
        m_offlineCachePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/amap_cache";
    }

    ~AmapEngine() {
        delete m_networkManager;
    }

    QByteArray loadTileFromCache(int zoom,int x,int y) override{
        QString filePath = QString("%1/%2/%3/%4.png").arg(m_offlineCachePath).arg(zoom).arg(x).arg(y);
        QFile file(filePath);
        if(file.open(QIODevice::ReadOnly)){
            return file.readAll();
        }
        return QByteArray();
    }

    void saveTileToCache(int zoom,int x, int y,const QByteArray &data) override{
        QString zoomDir = QString("%1/%2").arg(m_offlineCachePath).arg(zoom);
        QString xDir = QString("%1/%2").arg(zoomDir).arg(x);
        QDir dir;
        dir.mkpath(xDir);
        QString tilepath = QString("%1/%2.png").arg(xDir).arg(y);
        QFile file(tilepath);
        if(file.open(QIODevice::WriteOnly)){
            file.write(data);
            file.close();
        }
    }


    QString name() const override {
        return "Amap";
    }

    QString type() const override {
        return "standard";
    }

    bool supportsOffline() const override {
        return true;
    }

    void setOfflineMode(bool enabled) override {
        m_offlineMode = enabled;
    }

    void setOfflineCachePath(const QString &path) override {
        m_offlineCachePath = path;
    }

    QString tileUrlTemplate() const override {
        return "https://webst01.is.autonavi.com/appmaptile?x={x}&y={y}&z={z}&lang=zh_cn&size=1&scale=2&style=6";
    }

private:
    QNetworkAccessManager *m_networkManager;
    bool m_offlineMode;
    QString m_offlineCachePath;
};



//Mapbox地图引擎实现

class MapboxEngine : public mapengine
{
public:
    MapboxEngine() {
        m_networkManager = new QNetworkAccessManager();
        m_offlineMode = false;
        m_offlineCachePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/mapbox_cache";
    }

    ~MapboxEngine() {
        delete m_networkManager;
    }

    QByteArray loadTileFromCache(int zoom,int x,int y) override{
        QString filePath = QString("%1/%2/%3/%4.png").arg(m_offlineCachePath).arg(zoom).arg(x).arg(y);
        QFile file(filePath);
        if(file.open(QIODevice::ReadOnly)){
            return file.readAll();
        }
        return QByteArray();
    }

    void saveTileToCache(int zoom,int x, int y,const QByteArray &data) override{
        QString zoomDir = QString("%1/%2").arg(m_offlineCachePath).arg(zoom);
        QString xDir = QString("%1/%2").arg(zoomDir).arg(x);
        QDir().mkpath(xDir);
        QString tilepath = QString("%1/%2.png").arg(xDir).arg(y);
        QFile file(tilepath);
        if(file.open(QIODevice::WriteOnly)){
            file.write(data);
            file.close();
        }
    }

    QString name() const override {
        return "Mapbox";
    }

    QString type() const override {
        return "standard";
    }

    bool supportsOffline() const override {
        return true;
    }

    void setOfflineMode(bool enabled) override {
        m_offlineMode = enabled;
    }

    void setOfflineCachePath(const QString &path) override {
        m_offlineCachePath = path;
    }

    QString tileUrlTemplate() const override {
        return "https://api.mapbox.com/styles/v1/mapbox/streets-v11/tiles/256/{z}/{x}/{y}?access_token=YOUR_MAPBOX_ACCESS_TOKEN";
    }

private:
    QNetworkAccessManager *m_networkManager;
    bool m_offlineMode;
    QString m_offlineCachePath;

};




 //OpenStreetMap地图引擎实现

class OsmEngine : public mapengine
{
public:
    OsmEngine() {
        m_networkManager = new QNetworkAccessManager();
        m_offlineMode = false;
        m_offlineCachePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/osm_cache";
    }

    ~OsmEngine() {
        delete m_networkManager;
    }

    QByteArray loadTileFromCache(int zoom,int x,int y) override{
        QString filePath = QString("%1/%2/%3/%4.png").arg(m_offlineCachePath).arg(zoom).arg(x).arg(y);
        QFile file(filePath);
        if(file.open(QIODevice::ReadOnly)){
            return file.readAll();
        }
        return QByteArray();
    }

    void saveTileToCache(int zoom,int x, int y,const QByteArray &data) override{
        QString zoomDir = QString("%1/%2").arg(m_offlineCachePath).arg(zoom);
        QString xDir = QString("%1/%2").arg(zoomDir).arg(x);
        QDir().mkpath(xDir);
        QString tilepath = QString("%1/%2.png").arg(xDir).arg(y);
        QFile file(tilepath);
        if(file.open(QIODevice::WriteOnly)){
            file.write(data);
            file.close();
        }
    }

    QString name() const override {
        return "OpenStreetMap";
    }

    QString type() const override {
        return "standard";
    }

    bool supportsOffline() const override {
        return true;
    }

    void setOfflineMode(bool enabled) override {
        m_offlineMode = enabled;
    }

    void setOfflineCachePath(const QString &path) override {
        m_offlineCachePath = path;
    }

    QString tileUrlTemplate() const override {
        return "https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png";
    }

private:
    QNetworkAccessManager *m_networkManager;
    bool m_offlineMode;
    QString m_offlineCachePath;

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
