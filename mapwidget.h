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



    void addTrackPoint(double longitude,double latitude); //添加轨迹

    void clearTrack();       //清除所有轨迹



    void setMapEngine(const QString &engineName);  //设置地图引擎

    QString currentMapEngine() const; //获取当前地图引擎名称

    
    // 无人机位置相关方法
    void setDronePosition(double longitude, double latitude, double heading, const QString &type = "drone"); // 设置无人机位置
    void clearDronePosition(); // 清除无人机位置
    bool hasDronePosition() const; // 检查是否有无人机位置



    double clampLatitude(double lat) const {          //限制纬度在Web墨卡托投影有效范围内
        const double MAX_LAT = 85.05112878;
        return std::max(-MAX_LAT, std::min(MAX_LAT, lat));
    }



protected:
    void paintEvent(QPaintEvent *event) override;    //绘制事件处理
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;    //右键菜单事件处理
    void keyPressEvent(QKeyEvent *event) override;



private slots:

    void onTileLoaded(int zoom, int x, int y, const QPixmap &pixmap); //处理瓦片加载完成事件

    void deleteSelectedMarker();      //删除选中的标记点
    

private:
    // 地图常量
    static const int TILE_SIZE = 256; ///< 单张瓦片尺寸（高德/谷歌都是256x256）
    static const int MIN_ZOOM_LEVEL = 4; ///< 最小缩放级别
    static const int MAX_ZOOM_LEVEL = 18; ///< 最大缩放级别
    static const int INITIAL_ZOOM_LEVEL = 12; ///< 初始缩放级别


    
    MapTileLoader *m_tileLoader; ///< 地图瓦片加载器
    
    // 地图状态
    int m_zoomLevel; ///< 当前缩放级别
    QPoint m_mapOffset; ///< 地图偏移量
    QPoint m_lastMousePos; ///< 上次鼠标位置
    bool m_isDragging; ///< 是否正在拖动
    bool m_isDraggingMarker; ///<是否正在拖动标记
    QPoint m_markerDragStartPos; ///<标记拖动开始的位置


    
    QList<mapmarker*> m_markers; ///< 标记点列表
    QList<QPair<double, double>> m_trackPoints; ///< 轨迹点列表（经纬度）
    mapmarker* m_selectedMarker; ///< 当前选中的标记
    int m_nextMarkerIndex; ///< 下一个标记的索引
    QString m_currentEngine; ///< 当前地图引擎名称

    
    // 无人机位置信息
    bool m_hasDronePosition; ///< 是否有无人机位置
    double m_droneLongitude; ///< 无人机经度
    double m_droneLatitude;  ///< 无人机纬度
    double m_droneHeading;   ///< 无人机方向（角度）
    QString m_droneType;     ///< 无人机类型
    
    // 高度/海拔显示模式
    bool m_showHeight; ///< 是否显示高度（true）或海拔（false）



    void updateMarkerPosition(const QPoint &pos);  //更改标记点位置

    void drawCoordinateTooltip(QPainter *painter, const QPointF &pos, mapmarker *marker);  //绘制坐标提示

    void loadVisibleTiles();  //加载可见区域的瓦片
    
    mapmarker* findMarkerAtPosition(const QPoint &pos) const;  //检测点击位置是否在某个标记上
    
    int getNextMarkerIndex() const;  //获取下一个可用的标记索引
    
    void resetMarkerIndices();  //重置所有标记的索引
    
    void setSelectedMarker(mapmarker *marker);  //设置选中的标记

    mapmarker* getSelectedMarker() const;  //获取当前选中的标记
    
    void showEditAllMarkersDialog();  //显示编辑所有标记的对话框
};

#endif // MAPWIDGET_H
