#include "rasterizer.h"

//z-buffer实现
ZBuffer::ZBuffer(int width,int height) : width(width),height(height),data(width*height,-std::numeric_limits<float>::infinity()){}

void ZBuffer::set(const int x,const int y,const float value){
    if (!data.size() || x<0 || y<0 || x>=width || y>=height) return;
    data[x + y*width] = value;
}

float ZBuffer::get(const int x,const int y) const {
    if (!data.size() || x<0 || y<0 || x>=width || y>=height) return -std::numeric_limits<float>::infinity();
    return data[x + y*width];
}

TGAImage ZBuffer::toTGAImage(){
    TGAImage img(width,height,TGAImage::GRAYSCALE);
    float zmin = std::numeric_limits<float>::infinity();
    float zmax = -std::numeric_limits<float>::infinity();
    for(float z : data){
        if (!std::isfinite(z)) continue;
        zmin = std::min(zmin,z);
        zmax = std::max(zmax,z);
    }
    for(int y = 0; y < height; y++){
        for (int x = 0; x < width; x++){
            float z = data[x+y*width];
            if (!std::isfinite(z)) continue;
            TGAColor color = {static_cast<uint8_t>((zmin<zmax ? (z-zmin)/(zmax-zmin) : 1.0f) * 255.0f)};
            img.set(x,y,color);
        }
    }
    return img;
}
//三角形有符号面积
float triangle_signed_area(float ax,float ay,float bx,float by,float cx,float cy){
    float ret = 0.5f * ((bx - ax) * (cy - ay) - (by - ay) * (cx - ax));
    return ret;
}
//计算变换矩阵
Eigen::Matrix4f get_view_matrix(const Eigen::Vector3f& campos,const Eigen::Vector3f& center,const Eigen::Vector3f& up){
    Eigen::Matrix4f view;
    Eigen::Vector3f n = (campos - center).normalized();
    Eigen::Vector3f l = up.cross(n).normalized();
    Eigen::Vector3f m = n.cross(l).normalized();
    view << l.x(),l.y(),l.z(),-l.dot(center),
            m.x(),m.y(),m.z(),-m.dot(center),
            n.x(),n.y(),n.z(),-n.dot(center),
            0,0,0,1;
    return view;
}
Eigen::Matrix4f get_projection_matrix(const float distance){
    Eigen::Matrix4f projection;
    projection << 1,0,0,0,
                  0,1,0,0,
                  0,0,1,0,
                  0,0,-1.0f/distance,1;
    return projection;
}
Eigen::Matrix4f get_viewport_matrix(int x,int y,int width,int height){
    Eigen::Matrix4f viewport;
    viewport << width/2.0f,0,0,x+width/2.0f,
                0,height/2.0f,0,y+height/2.0f,
                0,0,1,0,
                0,0,0,1;
    return viewport;
}
//全局变换矩阵变量、z-buffer
Eigen::Matrix4f view,projection,viewport;
ZBuffer zbuffer;
//初始化光栅化器
void init_rasterizer(const Eigen::Vector3f campos,const Eigen::Vector3f center, const Eigen::Vector3f up,
                     const int vp_x, const int vp_y, const int vp_width, const int vp_height,
                     const int width, const int height)
{
    view = get_view_matrix(campos,center,up);
    projection = get_projection_matrix((center - campos).norm());
    viewport = get_viewport_matrix(vp_x,vp_y,vp_width,vp_height);
    zbuffer = ZBuffer(width,height);
}
//光栅化
void rasterize(const Triangle& clip, const IShader& shader,TGAImage& framebuffer){
    //透视除法变换到NDC空间
    Eigen::Vector4f ndc[3] = {clip[0]/clip[0].w(),clip[1]/clip[1].w(),clip[2]/clip[2].w()};
    //视口变换变换到屏幕空间
    Eigen::Vector3f scr[3] = {(viewport * ndc[0]).head<3>(),(viewport * ndc[1]).head<3>(),(viewport * ndc[2]).head<3>()};
    std::vector<Eigen::Vector2f> bouding_box;
    bouding_box.push_back(Eigen::Vector2f(std::min(scr[0].x(),std::min(scr[1].x(),scr[2].x())),std::min(scr[0].y(),std::min(scr[1].y(),scr[2].y()))));
    bouding_box.push_back(Eigen::Vector2f(std::max(scr[0].x(),std::max(scr[1].x(),scr[2].x())),std::max(scr[0].y(),std::max(scr[1].y(),scr[2].y()))));
    //矩阵形式重心坐标
    Eigen::Matrix3f ABC;
    ABC << scr[0].x(),scr[1].x(),scr[2].x(),
           scr[0].y(),scr[1].y(),scr[2].y(),
           1.0f,1.0f,1.0f;
    Eigen::Matrix3f ABC_inv = ABC.inverse();
    //忽略过小的面
    if(std::abs(ABC.determinant()) < 1e-6f) return;
    #pragma omp parallel for
    for(int y = std::max<int>(bouding_box[0][1],0); y <= std::min<int>(bouding_box[1][1],framebuffer.height()-1); y++){
        for(int x = std::max<int>(bouding_box[0][0],0); x <= std::min<int>(bouding_box[1][0],framebuffer.width()-1); x++){
            Eigen::Vector3f bar = ABC_inv * Eigen::Vector3f(static_cast<float>(x),static_cast<float>(y),1.0f);
            if(bar.x() < 0 || bar.y() < 0 || bar.z() < 0)continue;
            float z = bar.dot(Eigen::Vector3f(ndc[0].z(),ndc[1].z(),ndc[2].z()));
            if(z <= zbuffer.get(x,y)) continue;
            //利用重心坐标计算片段着色情况
            auto [discard, color] = shader.fragment(bar);
            if(discard) continue;
            zbuffer.set(x,y,z);
            framebuffer.set(x,y,color);
        }    
    }
}