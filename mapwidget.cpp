#include "mapwidget.h"
#include "mapmarker.h"

#include <QPainter>
#include <QDebug>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QTimer>
#include <cmath>
#include <QPainterPath>
#include <QInputDialog>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QMessageBox>
#include "mapengine.h"
#include <QKeyEvent>
#include <QTime>


mapwidget::mapwidget(QWidget *parent)
    : QWidget{parent},
    mZoomLevel(INITIAL_ZOOM_LEVEL),
    mMapOffset(0, 0),
    mIsDragging(false),
    mSelectedMarker(nullptr),
    mNextMarkerIndex(1),
    mCurrentEngine("amap")
{
    // 创建MapTileLoader实例
    mTileLoader = new MapTileLoader(this);

    // 默认使用高德地图引擎
    mapengine *engine = MapEngineFactory::createEngine("amap");
    if(engine){
        mTileLoader->setMapEngine(engine);
    }
    
    // 连接信号和槽
    connect(mTileLoader, &MapTileLoader::tileLoaded, this, &mapwidget::onTileLoaded);
    
    // 移除初始位置设置，使用默认偏移量
    mMapOffset = QPoint(0, 0);
    
    // 加载初始瓦片
    loadVisibleTiles();
}

void mapwidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    
    // 绘制地图瓦片
    for (int x = 0; x < width() / TILE_SIZE + 2; x++) {
        for (int y = 0; y < height() / TILE_SIZE + 2; y++) {
            // 计算瓦片坐标
            int tileX = (mMapOffset.x() / TILE_SIZE) + x;
            int tileY = (mMapOffset.y() / TILE_SIZE) + y;
            
            // 获取瓦片
            QPixmap tile = mTileLoader->getCachedTile(mZoomLevel, tileX, tileY);
            if (!tile.isNull()) {
                // 绘制瓦片
                int drawX = x * TILE_SIZE - (mMapOffset.x() % TILE_SIZE);
                int drawY = y * TILE_SIZE - (mMapOffset.y() % TILE_SIZE);
                
                // 根据缩放级别调整渲染提示
                // 当缩放级别较低（地图缩小很多）时，使用快速变换保持像素清晰
                if (mZoomLevel < 6) {
                    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
                } else {
                    painter.setRenderHint(QPainter::SmoothPixmapTransform);
                }
                
                painter.drawPixmap(drawX, drawY, tile);
            }
        }
    }
    
    // 绘制标记点
    for (mapmarker *marker : std::as_const(mMarkers)) {
        double longitude = marker->longitude();
        double latitude = marker->latitude();
        QPointF screenPos = geoToScreen(longitude, latitude);

        
        // 绘制标记点图标
        QPixmap icon = marker->icon();
        if (!icon.isNull()) {
            // 调整图标大小
            QPixmap scaledIcon = icon.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            // 绘制图标，使图标底部中心位于标记点位置（像地图针一样）
            int drawX = screenPos.x() - scaledIcon.width() / 2;
            int drawY = screenPos.y() - scaledIcon.height();
            painter.drawPixmap(drawX,drawY,scaledIcon);
        } else {
            // 如果没有图标，绘制默认标记（圆点中心在标记点位置）
            if (marker == mSelectedMarker) {
                // 选中的标记使用不同的颜色
                painter.setPen(QPen(Qt::blue, 3));
                painter.setBrush(QBrush(Qt::yellow));
            } else {
                painter.setPen(QPen(Qt::red, 2));
                painter.setBrush(QBrush(Qt::red));
            }
            // 绘制一个地图针样式的标记：圆点在上方，底部有尖角
            QPainterPath path;
            path.moveTo(screenPos.x(), screenPos.y()); // 底部尖角位置
            path.lineTo(screenPos.x() - 8, screenPos.y() - 16);
            path.arcTo(screenPos.x() - 8, screenPos.y() - 32, 16, 16, 180, 180);
            path.lineTo(screenPos.x() + 8, screenPos.y() - 16);
            path.closeSubpath();
            painter.drawPath(path);
        }
        
        // 绘制标记点标题
        QString title = marker->title();
        if (!title.isEmpty()) {
            if (marker == mSelectedMarker) {
                painter.setPen(QPen(Qt::blue, 2));
            } else {
                painter.setPen(QPen(Qt::black));
            }
            painter.drawText(screenPos.x() + 10, screenPos.y() - 10, title);
        }
        
        // 为选中的标记绘制选中效果
        if (marker == mSelectedMarker) {
            painter.setPen(QPen(Qt::blue, 2, Qt::DashLine));
            painter.setBrush(Qt::NoBrush);
            QPointF midPos = screenPos;
            midPos.ry() -= 10;
            painter.drawEllipse(midPos, 15, 15);
        }
    }

    if(mTrackPoints.size()>1){
        painter.setPen(QPen(Qt::blue,2));
        // 转换轨迹点为屏幕坐标并绘制
        QVector<QPointF> screenPoints;
        for (const auto &point : mTrackPoints) {
            double longitude = point.first;
            double latitude = point.second;
            QPointF screenPos = geoToScreen(longitude, latitude);
            screenPoints.append(screenPos);
        }
        painter.drawPolyline(screenPoints.data(), screenPoints.size());
    }
}

void mapwidget::wheelEvent(QWheelEvent *event)
{
    // 处理鼠标滚轮事件，实现缩放
    int oldZoom = mZoomLevel;
    
    if (event->angleDelta().y() > 0) {
        // 放大
        if (mZoomLevel < MAX_ZOOM_LEVEL) {
            mZoomLevel++;
        }
    } else {
        // 缩小
        if (mZoomLevel > MIN_ZOOM_LEVEL) {
            mZoomLevel--;
        }
    }
    
    // 只有缩放级别实际改变时才更新偏移量
    if (mZoomLevel != oldZoom) {
        // 计算缩放比例
        double scale = pow(2, mZoomLevel - oldZoom);
        
        // 获取鼠标位置
        QPoint mousePos = event->position().toPoint();
        
        // 计算新的偏移量，使鼠标位置对应的地理点保持在屏幕同一位置
        // 地图坐标 = 屏幕坐标 + 偏移量
        // 缩放后：新地图坐标 = 旧地图坐标 * scale
        // 要求：新屏幕坐标 = 旧屏幕坐标（鼠标位置不变）
        // 所以：新偏移量 = 新地图坐标 - 鼠标位置
        double mapX = mousePos.x() + mMapOffset.x();
        double mapY = mousePos.y() + mMapOffset.y();
        double newMapX = mapX * scale;
        double newMapY = mapY * scale;
        mMapOffset = QPoint(newMapX - mousePos.x(), newMapY - mousePos.y());
        
        // 重新加载瓦片
        loadVisibleTiles();
        update();
    }
}

void mapwidget::mousePressEvent(QMouseEvent *event)
{
    // 处理鼠标按下事件
    if (event->button() == Qt::LeftButton) {
        QPoint clickPos = event->position().toPoint();
        
        // 检测是否点击在标记上
        mapmarker *clickedMarker = findMarkerAtPosition(clickPos);
        if (clickedMarker) {
            // 选中标记
            setSelectedMarker(clickedMarker);
        } else {
            // 点击空白处，取消选中
            setSelectedMarker(nullptr);
        }
        
        mLastMousePos = clickPos;
        mIsDragging = true;
        setCursor(Qt::ClosedHandCursor);
    }
}

void mapwidget::mouseMoveEvent(QMouseEvent *event)
{
    // 处理鼠标移动事件，实现平移
    if (mIsDragging) {
        QPoint delta = event->position().toPoint() - mLastMousePos;
        QPoint newOffset = mMapOffset - delta;
        
        // 计算地图边界限制
        double zoomFactor = (1 << mZoomLevel);
        double totalPixels = zoomFactor * TILE_SIZE;
        
        // X方向边界：经度范围 -180 到 180
        int minX = -width() ;    // / 2;
        int maxX = totalPixels;    // - width() / 2;
        
        // Y方向边界：纬度范围约 -85 到 85
        int minY = -height();// / 2;
        int maxY = totalPixels;// - height() / 2;
        
        // 限制偏移量在边界范围内
        newOffset.setX(qMax(minX, qMin(maxX, newOffset.x())));
        newOffset.setY(qMax(minY, qMin(maxY, newOffset.y())));
        
        // 更新偏移量
        mMapOffset = newOffset;
        mLastMousePos = event->position().toPoint();
        
        // 重新加载瓦片
        loadVisibleTiles();
        update();
    }
}

void mapwidget::mouseReleaseEvent(QMouseEvent *event)
{
    // 处理鼠标释放事件
    if (event->button() == Qt::LeftButton) {
        mIsDragging = false;
        setCursor(Qt::ArrowCursor);
    }
}

mapmarker* mapwidget::findMarkerAtPosition(const QPoint &pos) const  //检测点击位置是否在某个标记上
{
    for (mapmarker *marker : mMarkers) {
        QPointF screenPos = geoToScreen(marker->longitude(), marker->latitude());
        // 检测点击位置是否在标记附近（半径15像素范围内）
        double distance = std::sqrt(std::pow(pos.x() - screenPos.x(), 2) + 
                                    std::pow(pos.y() - screenPos.y(), 2));
        if (distance <= 15) {
            return marker;
        }
    }
    return nullptr;
}


void mapwidget::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    QPoint clickPos = event->pos();
    
    // 检测是否点击在某个标记上
    mapmarker *clickedMarker = findMarkerAtPosition(clickPos);
    
    // 没有点击在标记上，显示默认菜单
    QAction *addMarkAction = menu.addAction("添加标记");
    QAction *editAllMarkersAction = menu.addAction("编辑标记");
    QAction *clearMarkersAction = menu.addAction("清除所有标记");
    QAction *deleteMarkerAction = menu.addAction("删除此标记");
    menu.addSeparator();

    QAction *addTrackPointAction = menu.addAction("添加轨迹");
    QAction *clearTrackPointAction = menu.addAction("清除所有轨迹");
    menu.addSeparator();

    // 添加地图引擎切换菜单
    QMenu *engineMenu = menu.addMenu("切换地图引擎");
    QAction *amapAction = engineMenu->addAction("高德地图");
    QAction *mapboxAction = engineMenu->addAction("Mapbox");
    QAction *osmAction = engineMenu->addAction("OpenStreetMap");

    connect(addMarkAction, &QAction::triggered, this, [=](){
        QPair<double,double> geoPos = screenToGeo(clickPos.x(), clickPos.y());
        // 创建标记时不指定标题，让addMarker自动添加索引
        mapmarker *marker = new mapmarker(geoPos.first, geoPos.second, "");
        addMarker(marker);
    });

    connect(editAllMarkersAction, &QAction::triggered, this, &mapwidget::showEditAllMarkersDialog);

    connect(addTrackPointAction,&QAction::triggered,this,[=](){
        QPair<double,double> geoPos = screenToGeo(clickPos.x(),clickPos.y());
        addTrackPoint(geoPos.first,geoPos.second);
    });

    connect(clearMarkersAction, &QAction::triggered, this, &mapwidget::clearMarkers);

    connect(clearTrackPointAction,&QAction::triggered,this,&mapwidget::clearTrack);

    // 地图引擎切换
    connect(amapAction, &QAction::triggered, this, [=](){
        setMapEngine("amap");
    });
    connect(mapboxAction, &QAction::triggered, this, [=](){
        setMapEngine("mapbox");
    });
    connect(osmAction, &QAction::triggered, this, [=](){
        setMapEngine("osm");
    });

    if(clickedMarker){
        if(mSelectedMarker != nullptr){
        connect(deleteMarkerAction, &QAction::triggered, this, [=](){
            removeMarker(mSelectedMarker);
            });}
    }

    menu.exec(event->globalPos());
}






void mapwidget::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Delete && mSelectedMarker != nullptr){
        deleteSelectedMarker();
    }else{
        QWidget::keyPressEvent(event);
    }
}



void mapwidget::onTileLoaded(int zoom, int x, int y, const QPixmap &pixmap)
{
    // 瓦片加载完成，触发重绘
    if (zoom == mZoomLevel) {
        update();
    }
}

void mapwidget::deleteSelectedMarker()
{
    if(!mSelectedMarker){
        return;
    }

    mMarkers.removeOne(mSelectedMarker);
    delete mSelectedMarker;
    mSelectedMarker = nullptr;
    update();
}




void mapwidget::loadVisibleTiles()
{
    // 计算可见区域的瓦片范围
    int startX = mMapOffset.x() / TILE_SIZE - 1;
    int startY = mMapOffset.y() / TILE_SIZE - 1;
    int endX = (mMapOffset.x() + width()) / TILE_SIZE + 1;
    int endY = (mMapOffset.y() + height()) / TILE_SIZE + 1;
    
    // 限制瓦片加载数量，避免拖拽时频繁加载
    static QTime LastLoadTime =QTime::currentTime();
    if (LastLoadTime.msecsTo(QTime::currentTime()) < 100) {
        return; // 100ms内不重复加载
    }
    LastLoadTime = QTime::currentTime();
    
    int centerScreenX = width()/2;
    int centerScreenY = height()/2;
    int centerTileX = (mMapOffset.x() + centerScreenX) / TILE_SIZE;
    int centerTileY = (mMapOffset.y() + centerScreenY) / TILE_SIZE;

    QVector<QPair<int,QPair<int,int>>> tiles;
    for(int x = startX; x < endX; x++){
        for(int y = startY; y<endY; y++){
            int distance = abs(x-centerTileX) + abs(y-centerTileY);

            tiles.append(QPair<int,QPair<int,int>>(distance,QPair<int,int>(x,y)));
        }
    }

    std::sort(tiles.begin(),tiles.end(),[](const QPair<int,QPair<int,int>> &a,const QPair<int,QPair<int,int>> &b){
        return a.first<b.first;
    });

    QTimer::singleShot(0, [=]() {
        int tileCount = 0;
        for(const auto &tile : tiles){
            if(tileCount<20){
                int x = tile.second.first;
                int y = tile.second.second;
                mTileLoader->loadTile(mZoomLevel,x,y);
                tileCount++;
            }
        }
    });
}





void mapwidget::addMarker(mapmarker *marker)
{
    if (marker) {
        // 自动添加索引
        QString title = marker->title();
        if (title.isEmpty()) {
            title = QString("标记%1").arg(mNextMarkerIndex);
            marker->setTitle(title);
            mNextMarkerIndex++;
        }
        mMarkers.append(marker);
        update();
    }
}



void mapwidget::removeMarker(mapmarker *marker)
{
    if (marker) {
        mMarkers.removeOne(marker);
        delete marker;
        update(); // 触发重绘
    }
}

void mapwidget::clearMarkers()
{
    mMarkers.clear();
    mNextMarkerIndex = 1; // 重置索引
    mSelectedMarker = nullptr; // 清除选中状态
    update(); // 触发重绘
}




QPointF mapwidget::geoToScreen(double longitude, double latitude) const
{

    latitude = clampLatitude(latitude);
    double zoomFactor = (1 << mZoomLevel);
    double totalPixels = zoomFactor * TILE_SIZE;

    // 经度转换
    double x = (longitude + 180.0) / 360.0 * totalPixels;

    // 纬度转换（Web墨卡托）
    double latRad = latitude * M_PI / 180.0;
    double y = (1.0 - std::log(std::tan(M_PI / 4.0 + latRad / 2.0)) / M_PI) / 2.0 * totalPixels;

    // 应用地图偏移
    return QPointF(x - mMapOffset.x(), y - mMapOffset.y());
}

QPair<double, double> mapwidget::screenToGeo(int x, int y) const
{
    double zoomFactor = (1 << mZoomLevel);
    double totalPixels = zoomFactor * TILE_SIZE;

    double screenX = x + mMapOffset.x();
    double screenY = y + mMapOffset.y();

    double longitude = (screenX / totalPixels - 0.5) * 360.0;

    double yFraction = screenY / totalPixels;
    // 反向计算纬度（与geoToScreen对应）
    double expInput = M_PI * (1.0 - 2.0 * yFraction);
    // 限制expInput的范围，避免exp函数溢出
    expInput = std::max(-709.0, std::min(709.0, expInput));
    double latRad = 2.0 * std::atan(std::exp(expInput)) - M_PI / 2.0;
    double latitude = latRad * 180.0 / M_PI;
    latitude = clampLatitude(latitude);

    return QPair<double, double>(longitude, latitude);
}

void mapwidget::addTrackPoint(double longitude, double latitude)
{
    // 存储经纬度坐标
    mTrackPoints.append(QPair<double, double>(longitude, latitude));
    update();
}

void mapwidget::clearTrack()
{
    mTrackPoints.clear();
    update();
}

void mapwidget::setMapEngine(const QString &engineName)
{
    mCurrentEngine = engineName;

    mapengine *oleEngine  = mTileLoader->mapEngine();
    if(oleEngine){
        delete oleEngine;
    }

    mapengine *engine = MapEngineFactory::createEngine(engineName);
    if (engine) {
        mTileLoader->setMapEngine(engine);
        // 重新加载瓦片
        loadVisibleTiles();
        update();
    }
}

QString mapwidget::currentMapEngine() const
{
    return mCurrentEngine;
}

int mapwidget::getNextMarkerIndex() const
{
    return mNextMarkerIndex;
}

void mapwidget::resetMarkerIndices()
{
    // 重新为所有标记分配索引
    int index = 1;
    for (mapmarker *marker : mMarkers) {
        QString title = QString("标记%1").arg(index);
        marker->setTitle(title);
        index++;
    }
    mNextMarkerIndex = index;
    update();
}

void mapwidget::setSelectedMarker(mapmarker *marker)
{
    mSelectedMarker = marker;
    update();
}

mapmarker* mapwidget::getSelectedMarker() const
{
    return mSelectedMarker;
}

void mapwidget::showEditAllMarkersDialog()
{
    // 弹出一个对话框，显示所有标记的信息
    QDialog dialog(this);
    dialog.setWindowTitle("编辑所有标记");
    dialog.resize(400, 300);
    
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    
    QListWidget *markerList = new QListWidget(&dialog);
    for (int i = 0; i < mMarkers.size(); i++) {
        mapmarker *marker = mMarkers[i];
        QString info = QString("%1: %2, %3").arg(marker->title())
                           .arg(marker->longitude())
                           .arg(marker->latitude());
        QListWidgetItem *item = new QListWidgetItem(info);
        item->setData(Qt::UserRole, i);
        markerList->addItem(item);
    }
    layout->addWidget(markerList);
    
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *editButton = new QPushButton("编辑选中", &dialog);
    QPushButton *deleteButton = new QPushButton("删除选中", &dialog);
    QPushButton *closeButton = new QPushButton("关闭", &dialog);
    
    buttonLayout->addWidget(editButton);
    buttonLayout->addWidget(deleteButton);
    buttonLayout->addWidget(closeButton);
    layout->addLayout(buttonLayout);
    
    connect(editButton, &QPushButton::clicked, [=, &dialog]() {
        QListWidgetItem *item = markerList->currentItem();
        if (item) {
            int index = item->data(Qt::UserRole).toInt();
            mapmarker *marker = mMarkers[index];
            
            bool ok;
            QString newTitle = QInputDialog::getText(&dialog, "编辑标记", "请输入新标题:", 
                                                     QLineEdit::Normal, marker->title(), &ok);
            if (ok && !newTitle.isEmpty()) {
                marker->setTitle(newTitle);
                // 更新列表项
                QString info = QString("%1: %2, %3").arg(marker->title())
                                   .arg(marker->longitude())
                                   .arg(marker->latitude());
                item->setText(info);
                update();
            }
        }
    });
    
    connect(deleteButton, &QPushButton::clicked, [=, &dialog]() {
        QListWidgetItem *item = markerList->currentItem();
        if (item) {
            int index = item->data(Qt::UserRole).toInt();
            mapmarker *marker = mMarkers[index];
            
            if (QMessageBox::question(&dialog, "删除标记", 
                                      QString("确定要删除标记 %1 吗？").arg(marker->title())) == QMessageBox::Yes) {
                if(marker == mSelectedMarker){
                    mSelectedMarker = nullptr;
                }
                    mMarkers.removeAt(index);
                    delete marker;
                    delete item;
                
                // 重新更新列表
                markerList->clear();
                for (int i = 0; i < mMarkers.size(); i++) {
                    mapmarker *m = mMarkers[i];
                    QString info = QString("%1: %2, %3").arg(m->title())
                                       .arg(m->longitude())
                                       .arg(m->latitude());
                    QListWidgetItem *newItem = new QListWidgetItem(info);
                    newItem->setData(Qt::UserRole, i);
                    markerList->addItem(newItem);
                }
                
                update();
            }
        }
    });
    
    connect(closeButton, &QPushButton::clicked, &dialog, &QDialog::close);
    
    dialog.exec();
}
