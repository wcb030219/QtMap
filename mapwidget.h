#ifndef MAPWIDGET_H
#define MAPWIDGET_H

#include <QWidget>
#include <QPixmap>
#include <QPoint>
#include <QMenu>
#include <QContextMenuEvent>

#include "mapmarker.h"
#include "maptileloader.h"



class mapwidget : public QWidget     //	地图主视图，负责绘制与事件处理
{
    Q_OBJECT
public:
    explicit mapwidget(QWidget *parent = nullptr);
    

    void addMarker(mapmarker *marker);  //添加标记点

    void removeMarker(mapmarker *marker); //移除标记点
    

    void clearMarkers();//清除所有标记点
    

    QPointF geoToScreen(double longitude, double latitude) const; //将经纬度坐标转换为屏幕坐标
    

    QPair<double, double> screenToGeo(int x, int y) const;  //将屏幕坐标转换为经纬度坐标

    void addTrackPoint(double longitude,double latitude);

    void clearTrack();



    void setMapEngine(const QString &engineName);  //设置地图引擎


    QString currentMapEngine() const; //获取当前地图引擎名称


    double clampLatitude(double lat) const {
        const double MAX_LAT = 85.05112878;
        return std::max(-MAX_LAT, std::min(MAX_LAT, lat));
    }





protected:
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;






private slots:

    void onTileLoaded(int zoom, int x, int y, const QPixmap &pixmap); //处理瓦片加载完成事件

    void deleteSelectedMarker();
    

private:
    // 地图常量
    static const int TILE_SIZE = 256; ///< 单张瓦片尺寸（高德/谷歌都是256x256）
    static const int MIN_ZOOM_LEVEL = 3; ///< 最小缩放级别
    static const int MAX_ZOOM_LEVEL = 20; ///< 最大缩放级别
    static const int INITIAL_ZOOM_LEVEL = 10; ///< 初始缩放级别


    
    MapTileLoader *mTileLoader; ///< 地图瓦片加载器
    
    // 地图状态
    int mZoomLevel; ///< 当前缩放级别
    QPoint mMapOffset; ///< 地图偏移量
    QPoint mLastMousePos; ///< 上次鼠标位置
    bool mIsDragging; ///< 是否正在拖动
    
    QList<mapmarker*> mMarkers; ///< 标记点列表
    QList<QPair<double, double>> mTrackPoints; ///< 轨迹点列表（经纬度）
    mapmarker* mSelectedMarker; ///< 当前选中的标记
    int mNextMarkerIndex; ///< 下一个标记的索引
    QString mCurrentEngine; ///< 当前地图引擎名称

    void loadVisibleTiles();  //加载可见区域的瓦片
    

    mapmarker* findMarkerAtPosition(const QPoint &pos) const;  //检测点击位置是否在某个标记上
    

    int getNextMarkerIndex() const;  //获取下一个可用的标记索引
    

    void resetMarkerIndices();  //重置所有标记的索引
    

    void setSelectedMarker(mapmarker *marker);  //设置选中的标记

    mapmarker* getSelectedMarker() const;  //获取当前选中的标记
    

    void showEditAllMarkersDialog();  //显示编辑所有标记的对话框
};

#endif // MAPWIDGET_H
