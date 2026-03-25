#include "mapmarker.h"

mapmarker::mapmarker(double longitude, double latitude, const QString &title)
    : mIndex(-1),
      mLongitude(longitude),
      mLatitude(latitude),
      mTitle(title)
{}

// 获取索引
int mapmarker::index() const
{
    return mIndex;
}

// 设置索引
void mapmarker::setIndex(int index)
{
    mIndex = index;
}

// 获取经度
double mapmarker::longitude() const
{
    return mLongitude;
}

// 获取纬度
double mapmarker::latitude() const
{
    return mLatitude;
}

// 设置经度
void mapmarker::setLongitude(double longitude)
{
    mLongitude = longitude;
}

// 设置纬度
void mapmarker::setLatitude(double latitude)
{
    mLatitude = latitude;
}

// 获取标题
QString mapmarker::title() const
{
    return mTitle;
}

// 设置标题
void mapmarker::setTitle(const QString &title)
{
    mTitle = title;
}

// 获取图标
QPixmap mapmarker::icon() const
{
    return mIcon;
}

// 设置图标
void mapmarker::setIcon(const QPixmap &icon)
{
    mIcon = icon;
}
