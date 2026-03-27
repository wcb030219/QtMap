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
#include <QLineEdit>
#include <QLabel>
#include <QGroupBox>
#include <QTableWidget>
#include <QHeaderView>


mapwidget::mapwidget(QWidget *parent)
    : QWidget{parent},
    m_zoomLevel(INITIAL_ZOOM_LEVEL),
    m_mapOffset(0, 0),
    m_isDragging(false),
    m_isDraggingMarker(false),
    m_selectedMarker(nullptr),
    m_nextMarkerIndex(1),
    m_currentEngine("amap"),
    m_hasDronePosition(false),
    m_droneLongitude(0.0),
    m_droneLatitude(0.0),
    m_droneHeading(0.0),
    m_droneType("drone")
{
    // 创建MapTileLoader实例
    m_tileLoader = new MapTileLoader(this);

    // 默认使用高德地图引擎
    mapengine *engine = MapEngineFactory::createEngine("amap");
    if(engine){
        m_tileLoader->setMapEngine(engine);
    }


    
    // 连接信号和槽
    connect(m_tileLoader, &MapTileLoader::tileLoaded, this, &mapwidget::onTileLoaded);
    
    // 移除初始位置设置，使用默认偏移量
    m_mapOffset = QPoint(0, 0);

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
            int tileX = (m_mapOffset.x() / TILE_SIZE) + x;
            int tileY = (m_mapOffset.y() / TILE_SIZE) + y;
            
            // 获取瓦片
            QPixmap tile = m_tileLoader->getCachedTile(m_zoomLevel, tileX, tileY);
            if (!tile.isNull()) {
                // 绘制瓦片
                int drawX = x * TILE_SIZE - (m_mapOffset.x() % TILE_SIZE);
                int drawY = y * TILE_SIZE - (m_mapOffset.y() % TILE_SIZE);
                
                // 根据缩放级别调整渲染提示
                // 当缩放级别较低（地图缩小很多）时，使用快速变换保持像素清晰
                if (m_zoomLevel < 6) {
                    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
                } else {
                    painter.setRenderHint(QPainter::SmoothPixmapTransform);
                }
                
                painter.drawPixmap(drawX, drawY, tile);
            }
        }
    }
    
    // 绘制标记点
    for (mapmarker *marker : std::as_const(m_markers)) {
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
            if (marker == m_selectedMarker) {
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
            if (marker == m_selectedMarker) {
                painter.setPen(QPen(Qt::blue, 2));
            } else {
                painter.setPen(QPen(Qt::black));
            }
            painter.drawText(screenPos.x() + 10, screenPos.y() - 10, title);
        }
        
        // 为选中的标记绘制选中效果
        if (marker == m_selectedMarker) {
            painter.setPen(QPen(Qt::blue, 2, Qt::DashLine));
            painter.setBrush(Qt::NoBrush);
            QPointF midPos = screenPos;
            midPos.ry() -= 10;
            painter.drawEllipse(midPos, 15, 15);
            
            // 绘制经纬度信息在标记旁边
            drawCoordinateTooltip(&painter, screenPos, marker->longitude(), marker->latitude());
        }
    }

    if(m_trackPoints.size()>1){
        painter.setPen(QPen(Qt::blue,2));
        // 转换轨迹点为屏幕坐标并绘制
        QVector<QPointF> screenPoints;
        for (const auto &point : m_trackPoints) {
            double longitude = point.first;
            double latitude = point.second;
            QPointF screenPos = geoToScreen(longitude, latitude);
            screenPoints.append(screenPos);
        }
        painter.drawPolyline(screenPoints.data(), screenPoints.size());
    }
    





    // 绘制无人机位置
    if (m_hasDronePosition) {
        QPointF screenPos = geoToScreen(m_droneLongitude, m_droneLatitude);
        
        // 保存当前绘制状态
        painter.save();
        
        // 移动到无人机位置
        painter.translate(screenPos);
        
        // 旋转到无人机方向
        painter.rotate(m_droneHeading);
        
        // 绘制无人机图标（使用三角形表示）
        painter.setPen(QPen(Qt::red, 2));
        painter.setBrush(QBrush(Qt::red));
        
        // 绘制无人机形状（三角形）
        QPainterPath dronePath;
        dronePath.moveTo(0, -15); // 机头
        dronePath.lineTo(-10, 10); // 左翼
        dronePath.lineTo(10, 10); // 右翼
        dronePath.closeSubpath();
        painter.drawPath(dronePath);
        
        // 绘制方向指示器（细线）
        painter.setPen(QPen(Qt::yellow, 2));
        painter.drawLine(0, -15, 0, -25);
        
        // 恢复绘制状态
        painter.restore();
        
        // 绘制无人机信息
        painter.setPen(QPen(Qt::black));
        QString droneInfo = QString("无人机: %1\n方向: %2°").arg(m_droneType).arg(m_droneHeading, 0, 'f', 1);
        QFontMetrics metrics(painter.font());
        QRect textRect = metrics.boundingRect(0, 0, 200, 50, Qt::TextWordWrap, droneInfo);
        textRect.adjust(-5, -5, 5, 5);
        textRect.moveTo(screenPos.x() + 20, screenPos.y() - textRect.height() - 10);
        
        painter.setBrush(QBrush(QColor(255, 255, 220, 200)));
        painter.drawRect(textRect);
        painter.drawText(textRect, Qt::AlignCenter, droneInfo);
    }
}






void mapwidget::wheelEvent(QWheelEvent *event)
{
    // 处理鼠标滚轮事件，实现缩放
    int oldZoom = m_zoomLevel;
    
    if (event->angleDelta().y() > 0) {
        // 放大
        if (m_zoomLevel < MAX_ZOOM_LEVEL) {
            m_zoomLevel++;
        }
    } else {
        // 缩小
        if (m_zoomLevel > MIN_ZOOM_LEVEL) {
            m_zoomLevel--;
        }
    }
    
    // 只有缩放级别实际改变时才更新偏移量
    if (m_zoomLevel != oldZoom) {
        // 计算缩放比例
        double scale = pow(2, m_zoomLevel - oldZoom);
        
        // 获取鼠标位置
        QPoint mousePos = event->position().toPoint();
        
        // 计算新的偏移量，使鼠标位置对应的地理点保持在屏幕同一位置
        // 地图坐标 = 屏幕坐标 + 偏移量
        // 缩放后：新地图坐标 = 旧地图坐标 * scale
        // 要求：新屏幕坐标 = 旧屏幕坐标（鼠标位置不变）
        // 所以：新偏移量 = 新地图坐标 - 鼠标位置
        double mapX = mousePos.x() + m_mapOffset.x();
        double mapY = mousePos.y() + m_mapOffset.y();
        double newMapX = mapX * scale;
        double newMapY = mapY * scale;
        m_mapOffset = QPoint(newMapX - mousePos.x(), newMapY - mousePos.y());
        
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
            m_isDraggingMarker = true;
            m_markerDragStartPos = clickPos;
            m_isDragging = false;
        } else {
            // 点击空白处，取消选中
            setSelectedMarker(nullptr);
            m_isDragging = true;
            m_isDraggingMarker = false;
        }
        
        m_lastMousePos = clickPos;
        m_tileLoader->cancelAllRequests();
        m_tileLoader->setDraggingState(true);
        setCursor(Qt::ClosedHandCursor);
    }
}

void mapwidget::mouseMoveEvent(QMouseEvent *event)
{

    if(m_isDraggingMarker){
        QPoint currentPos = event->position().toPoint();
        updateMarkerPosition(currentPos);
        update();
    }
    // 处理鼠标移动事件，实现平移
    else if (m_isDragging) {
        QPoint delta = event->position().toPoint() - m_lastMousePos;
        QPoint newOffset = m_mapOffset - delta;
        
        // 计算地图边界限制
        double zoomFactor = (1 << m_zoomLevel);
        double totalPixels = zoomFactor * TILE_SIZE;
        
        // X方向边界：确保地图充满整个控件宽度
        int minX = 0;
        int maxX = totalPixels - width();
        
        // Y方向边界：确保地图充满整个控件高度
        int minY = 0;
        int maxY = totalPixels - height();
        
        // 确保边界值合理
        maxX = qMax(0, maxX);
        maxY = qMax(0, maxY);
        
        // 限制偏移量在边界范围内
        newOffset.setX(qMax(minX, qMin(maxX, newOffset.x())));
        newOffset.setY(qMax(minY, qMin(maxY, newOffset.y())));
        
        // 更新偏移量
        m_mapOffset = newOffset;
        m_lastMousePos = event->position().toPoint();
        
        // 优化：减少重绘频率，提高拖拽流畅度
        static QTime lastUpdateTime = QTime::currentTime();
        if (lastUpdateTime.msecsTo(QTime::currentTime()) > 20) { // 每20ms重绘一次
            update();
            lastUpdateTime = QTime::currentTime();
        }
        
        // 重新加载瓦片
        loadVisibleTiles();
    }
}

void mapwidget::mouseReleaseEvent(QMouseEvent *event)
{
    // 处理鼠标释放事件
    if (event->button() == Qt::LeftButton) {
        m_isDragging = false;
        m_isDraggingMarker = false;
        m_tileLoader->setDraggingState(false);
        setCursor(Qt::ArrowCursor);
    }
}

mapmarker* mapwidget::findMarkerAtPosition(const QPoint &pos) const  //检测点击位置是否在某个标记上
{
    for (mapmarker *marker : m_markers) {
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


    //标记
    connect(addMarkAction, &QAction::triggered, this, [=](){
        QPair<double,double> geoPos = screenToGeo(clickPos.x(), clickPos.y());
        // 创建标记时不指定标题，让addMarker自动添加索引
        mapmarker *marker = new mapmarker(geoPos.first, geoPos.second, "");
        addMarker(marker);
    });

    connect(editAllMarkersAction, &QAction::triggered, this, &mapwidget::showEditAllMarkersDialog);
    connect(clearMarkersAction, &QAction::triggered, this, &mapwidget::clearMarkers);


    //轨迹
    connect(addTrackPointAction,&QAction::triggered,this,[=](){
        QPair<double,double> geoPos = screenToGeo(clickPos.x(),clickPos.y());
        addTrackPoint(geoPos.first,geoPos.second);
    });
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
        if(m_selectedMarker != nullptr){
        connect(deleteMarkerAction, &QAction::triggered, this, [=](){
            removeMarker(m_selectedMarker);
            });}
    }

    menu.exec(event->globalPos());
}



void mapwidget::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Delete && m_selectedMarker != nullptr){
        deleteSelectedMarker();
    }else{
        QWidget::keyPressEvent(event);
    }
}



void mapwidget::onTileLoaded(int zoom, int x, int y, const QPixmap &pixmap)
{
    // 瓦片加载完成，触发重绘
    if (zoom == m_zoomLevel) {
        update();
    }
}



void mapwidget::deleteSelectedMarker()
{
    if(!m_selectedMarker){
        return;
    }

    m_markers.removeOne(m_selectedMarker);
    delete m_selectedMarker;
    m_selectedMarker = nullptr;
    update();
}



void mapwidget::updateMarkerPosition(const QPoint &pos)
{
    if(m_selectedMarker){
        QPair<double,double> geoPos = screenToGeo(pos.x(),pos.y());
        m_selectedMarker->setLongitude(geoPos.first);
        m_selectedMarker->setLatitude(geoPos.second);
    }
}





void mapwidget::drawCoordinateTooltip(QPainter *painter, const QPointF &pos, double longitude, double latitude)
{
    // 格式化坐标文本
    QString coordText = QString("纬度: %1\n经度: %2").arg(latitude, 0, 'f', 6).arg(longitude, 0, 'f', 6);

    // 绘制背景
    QFontMetrics metrics(painter->font());
    QRect textRect = metrics.boundingRect(0, 0, 200, 50, Qt::TextWordWrap, coordText);
    textRect.adjust(-5, -5, 5, 5);
    // 调整位置，显示在标记的右上方
    textRect.moveTo(pos.x() + 20, pos.y() - textRect.height() - 10);

    painter->setBrush(QBrush(QColor(255, 255, 220, 200)));
    painter->setPen(QPen(Qt::black, 1));
    painter->drawRect(textRect);

    // 绘制文本
    painter->setPen(QPen(Qt::black));
    painter->drawText(textRect, Qt::AlignCenter, coordText);
}




void mapwidget::loadVisibleTiles()
{
    // 计算可见区域的瓦片范围
    int startX = m_mapOffset.x() / TILE_SIZE - 1;
    int startY = m_mapOffset.y() / TILE_SIZE - 1;
    int endX = (m_mapOffset.x() + width()) / TILE_SIZE + 1;
    int endY = (m_mapOffset.y() + height()) / TILE_SIZE + 1;
    
    // 限制瓦片加载数量，避免拖拽时频繁加载
    static QTime LastLoadTime = QTime::currentTime();
    // 优化：拖拽时减少瓦片加载频率
    int minInterval = m_isDragging ? 200 : 100; // 拖拽时200ms，静止时100ms
    if (LastLoadTime.msecsTo(QTime::currentTime()) < minInterval) {
        return; // 时间间隔内不重复加载
    }
    LastLoadTime = QTime::currentTime();
    
    int centerScreenX = width()/2;
    int centerScreenY = height()/2;
    int centerTileX = (m_mapOffset.x() + centerScreenX) / TILE_SIZE;
    int centerTileY = (m_mapOffset.y() + centerScreenY) / TILE_SIZE;

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
                m_tileLoader->loadTile(m_zoomLevel,x,y);
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
            title = QString("标记%1").arg(m_nextMarkerIndex);
            marker->setTitle(title);
            m_nextMarkerIndex++;
        }
        m_markers.append(marker);
        update();
    }
}



void mapwidget::removeMarker(mapmarker *marker)
{
    if (marker) {
        m_markers.removeOne(marker);
        delete marker;
        update(); // 触发重绘
    }
}

void mapwidget::clearMarkers()
{
    m_markers.clear();
    m_nextMarkerIndex = 1; // 重置索引
    m_selectedMarker = nullptr; // 清除选中状态
    update(); // 触发重绘
}




QPointF mapwidget::geoToScreen(double longitude, double latitude) const
{

    latitude = clampLatitude(latitude);
    double zoomFactor = (1 << m_zoomLevel);
    double totalPixels = zoomFactor * TILE_SIZE;

    // 经度转换
    double x = (longitude + 180.0) / 360.0 * totalPixels;

    // 纬度转换（Web墨卡托）
    double latRad = latitude * M_PI / 180.0;
    double y = (1.0 - std::log(std::tan(M_PI / 4.0 + latRad / 2.0)) / M_PI) / 2.0 * totalPixels;

    // 应用地图偏移
    return QPointF(x - m_mapOffset.x(), y - m_mapOffset.y());
}

QPair<double, double> mapwidget::screenToGeo(int x, int y) const
{
    double zoomFactor = (1 << m_zoomLevel);
    double totalPixels = zoomFactor * TILE_SIZE;

    double screenX = x + m_mapOffset.x();
    double screenY = y + m_mapOffset.y();

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
    m_trackPoints.append(QPair<double, double>(longitude, latitude));
    update();
}


void mapwidget::clearTrack()
{
    m_trackPoints.clear();
    update();
}

void mapwidget::setMapEngine(const QString &engineName)
{
    m_currentEngine = engineName;

    mapengine *engine = MapEngineFactory::createEngine(engineName);
    if (engine) {
        m_tileLoader->setMapEngine(engine);
        // 重新加载瓦片
        loadVisibleTiles();
        update();
    }
}

QString mapwidget::currentMapEngine() const
{
    return m_currentEngine;
}

/**
 * 设置无人机位置
 * @param longitude 经度
 * @param latitude 纬度
 * @param heading 方向（角度）
 * @param type 无人机类型
 */
void mapwidget::setDronePosition(double longitude, double latitude, double heading, const QString &type)
{
    m_droneLongitude = longitude;
    m_droneLatitude = latitude;
    m_droneHeading = heading;
    m_droneType = type;
    m_hasDronePosition = true;
    update(); // 触发重绘
}

/**
 * 清除无人机位置
 */
void mapwidget::clearDronePosition()
{
    m_hasDronePosition = false;
    update(); // 触发重绘
}

/**
 * 检查是否有无人机位置
 * @return 是否有无人机位置
 */
bool mapwidget::hasDronePosition() const
{
    return m_hasDronePosition;
}

int mapwidget::getNextMarkerIndex() const
{
    return m_nextMarkerIndex;
}

void mapwidget::resetMarkerIndices()
{
    // 重新为所有标记分配索引
    int index = 1;
    for (mapmarker *marker : m_markers) {
        QString title = QString("标记%1").arg(index);
        marker->setTitle(title);
        index++;
    }
    m_nextMarkerIndex = index;
    update();
}

void mapwidget::setSelectedMarker(mapmarker *marker)
{
    m_selectedMarker = marker;
    update();
}

mapmarker* mapwidget::getSelectedMarker() const
{
    return m_selectedMarker;
}





void mapwidget::showEditAllMarkersDialog()
{
    // 弹出一个对话框，双击直接编辑每个字段
    QDialog dialog(this);
    dialog.setWindowTitle("编辑标记");
    dialog.resize(800, 450);

    QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);

    // 说明标签
    QLabel *hintLabel = new QLabel("双击单元格可直接编辑", &dialog);
    hintLabel->setStyleSheet("color: gray;");
    mainLayout->addWidget(hintLabel);

    // 使用表格显示标记信息，每列都可编辑
    QTableWidget *markerTable = new QTableWidget(&dialog);
    markerTable->setColumnCount(4);
    markerTable->setHorizontalHeaderLabels(QStringList() << "标题" << "纬度" << "经度" << "类型");
    markerTable->setRowCount(m_markers.size());
    markerTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    markerTable->setSelectionMode(QAbstractItemView::SingleSelection);
    markerTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    markerTable->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);

    for (int i = 0; i < m_markers.size(); i++) {
        mapmarker *marker = m_markers[i];

        QTableWidgetItem *titleItem = new QTableWidgetItem(marker->title());
        QTableWidgetItem *latItem = new QTableWidgetItem(QString::number(marker->latitude(), 'f', 6));
        QTableWidgetItem *lonItem = new QTableWidgetItem(QString::number(marker->longitude(), 'f', 6));
        QTableWidgetItem *typeItem = new QTableWidgetItem(marker->type());

        // 设置对齐方式
        latItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        lonItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        // 存储标记索引
        titleItem->setData(Qt::UserRole, i);
        latItem->setData(Qt::UserRole, i);
        lonItem->setData(Qt::UserRole, i);
        typeItem->setData(Qt::UserRole, i);

        markerTable->setItem(i, 0, titleItem);
        markerTable->setItem(i, 1, latItem);
        markerTable->setItem(i, 2, lonItem);
        markerTable->setItem(i, 3, typeItem);
    }

    markerTable->resizeColumnsToContents();
    mainLayout->addWidget(markerTable);

    // 按钮区域
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *confirmButton = new QPushButton("确认", &dialog);
    QPushButton *closeButton = new QPushButton("关闭", &dialog);

    buttonLayout->addStretch();
    buttonLayout->addWidget(confirmButton);
    buttonLayout->addWidget(closeButton);
    mainLayout->addLayout(buttonLayout);

    // 确认按钮：应用所有更改
    connect(confirmButton, &QPushButton::clicked, this, [this, markerTable, &dialog]() {
        bool hasError = false;

        for (int i = 0; i < markerTable->rowCount(); i++) {
            mapmarker *marker = m_markers[i];

            // 获取编辑后的值
            QString newTitle = markerTable->item(i, 0)->text();
            QString latStr = markerTable->item(i, 1)->text();
            QString lonStr = markerTable->item(i, 2)->text();
            QString newType = markerTable->item(i, 3)->text();

            // 解析经纬度
            bool latOk = false, lonOk = false;
            double newLat = latStr.toDouble(&latOk);
            double newLon = lonStr.toDouble(&lonOk);

            if (!latOk || !lonOk) {
                hasError = true;
                continue;
            }

            // 限制纬度范围
            newLat = clampLatitude(newLat);

            // 更新标记属性
            marker->setTitle(newTitle);
            marker->setLatitude(newLat);
            marker->setLongitude(newLon);
            marker->setType(newType);
        }

        if (hasError) {
            QMessageBox::warning(&dialog, "部分错误", "部分经纬度输入无效，已跳过这些行。");
        }

        // 刷新显示
        update();
        dialog.accept();
    });

    connect(closeButton, &QPushButton::clicked, &dialog, &QDialog::close);

    dialog.exec();
}
