#pragma once
#include <cmath>
#include "tgaimage.h"
#include "model.h"

struct ZBuffer{
    
    ZBuffer() = default;
    ZBuffer(const int width,const int height);
    float get(const int x,const int y) const;
    void set(const int x,const int y,const float value);
    TGAImage toTGAImage();
    
    private:
    int width = 0;
    int height = 0;
    std::vector<float> data = {};
};
//初始化光栅化器，设置全局变换矩阵和深度缓冲
void init_rasterizer(const Eigen::Vector3f campos,const Eigen::Vector3f center, const Eigen::Vector3f up,
                     const int vp_x, const int vp_y, const int vp_width, const int vp_height,
                     const int width, const int height);

//shader接口，(= 0)纯虚函数fragment，要求派生类提供实现：接受一组重心坐标bar，返回一对(bool,TGAColor)的值(discard,color)
struct IShader{
    virtual std::pair<bool,TGAColor> fragment(const Eigen::Vector3f bar) const = 0;
};

//定义数据类型Triangle为三个Eigen::Vector4f的数组
typedef Eigen::Vector4f Triangle[3];

//光栅化，接受一个裁剪空间三角形(的三个顶点)，一个shader，在framebuffer上光栅化
void rasterize(const Triangle& clip, const IShader& shader,TGAImage& framebuffer);