#include "mapmarker.h"

mapmarker::mapmarker(double longitude, double latitude, const QString &title, const QString &type)
    : m_index(-1),
      m_longitude(longitude),
      m_latitude(latitude),
      m_title(title),
      m_type(type)
{}

// 获取索引
int mapmarker::index() const
{
    return m_index;
}

// 设置索引
void mapmarker::setIndex(int index)
{
    index = index;
}

// 获取经度
double mapmarker::longitude() const
{
    return m_longitude;
}

// 获取纬度
double mapmarker::latitude() const
{
    return m_latitude;
}

// 设置经度
void mapmarker::setLongitude(double longitude)
{
    m_longitude = longitude;
}

// 设置纬度
void mapmarker::setLatitude(double latitude)
{
    m_latitude = latitude;
}

// 获取标题
QString mapmarker::title() const
{
    return m_title;
}

// 设置标题
void mapmarker::setTitle(const QString &title)
{
    m_title = title;
}

// 获取类型
QString mapmarker::type() const
{
    return m_type;
}

// 设置类型
void mapmarker::setType(const QString &type)
{
    m_type = type;
}

// 获取图标
QPixmap mapmarker::icon() const
{
    return m_icon;
}

// 设置图标
void mapmarker::setIcon(const QPixmap &icon)
{
    m_icon = icon;
}
