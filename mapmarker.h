#ifndef MAPMARKER_H
#define MAPMARKER_H

#include <QPointF>
#include <QPixmap>
#include <QString>

class mapmarker        //管理标记点数据
{
public:
    mapmarker(double longitude, double latitude, const QString &title = "", const QString &type = "");
    
    // 获取索引
    int index() const;
    
    // 设置索引
    void setIndex(int index);
    
    // 获取经纬度
    double longitude() const;
    double latitude() const;
    
    // 设置经纬度
    void setLongitude(double longitude);
    void setLatitude(double latitude);
    
    // 获取标题
    QString title() const;
    
    // 设置标题
    void setTitle(const QString &title);
    
    // 获取类型
    QString type() const;
    
    // 设置类型
    void setType(const QString &type);
    
    // 获取图标
    QPixmap icon() const;
    
    // 设置图标
    void setIcon(const QPixmap &icon);
    
private:
    int m_index;           // 标记索引
    double m_longitude;   // 经度
    double m_latitude;    // 纬度
    QString m_title;      // 标记点标题
    QString m_type;       // 标记点类型
    QPixmap m_icon;       // 标记点图标
};

#endif // MAPMARKER_H
