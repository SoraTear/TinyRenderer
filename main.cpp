#include "rasterizer.h"
#include <iostream>
#include <cstring>
#include <cmath>

constexpr TGAColor white   = {255, 255, 255, 255}; // attention, BGRA order
constexpr TGAColor green   = {  0, 255,   0, 255};
constexpr TGAColor red     = {  0,   0, 255, 255};
constexpr TGAColor blue    = {255, 128,  64, 255};
constexpr TGAColor yellow  = {  0, 200, 255, 255};
constexpr TGAColor cyan    = {255, 255,   0, 255};
constexpr TGAColor magenta = {255,   0, 255, 255};
extern Eigen::Matrix4f view,projection,viewport;
extern ZBuffer zbuffer;

struct BlinnPhongShader : IShader{
    const Model& model;
    Eigen::Vector3f light_pos;
    Eigen::Vector3f campos; 
    Eigen::Vector3f k_ads[3] = {Eigen::Vector3f(0.1f,0.1f,0.1f),Eigen::Vector3f(0.7f,0.7f,0.7f),Eigen::Vector3f(0.8f,0.8f,0.8f)};
    Eigen::Vector3f I{1.5f,1.5f,1.5f};
    Eigen::Vector3f Ia{1.0f,1.0f,1.0f};
    //相机空间三角形
    Eigen::Vector3f view_triangle[3];
    Eigen::Vector3f clip_vertex_w;
    BlinnPhongShader(const Model& model,const Eigen::Vector3f& light_pos,const Eigen::Vector3f& campos) : model(model),light_pos(light_pos),campos(campos){
        this->light_pos = (view * Eigen::Vector4f(light_pos.x(),light_pos.y(),light_pos.z(),1.0f)).head<3>();
        this->campos = (view * Eigen::Vector4f(campos.x(),campos.y(),campos.z(),1.0f)).head<3>();
    };
    //输入模型中的面索引和面上点索引，将找到的点变换到相机空间并记入相机空间三角形，返回投影变换后的该点（裁剪空间）
    virtual Eigen::Vector4f vertex(const int f, const int f_index) {
        Eigen::Vector3f model_vertex = model.verticies[model.face_verticies[f][f_index]];
        Eigen::Vector4f view_vertex = view * Eigen::Vector4f(model_vertex.x(),model_vertex.y(),model_vertex.z(),1.0f);
        view_triangle[f_index] = view_vertex.head<3>();
        Eigen::Vector4f clip_vertex = projection * view_vertex;
        clip_vertex_w[f_index] = clip_vertex.w();
        return clip_vertex;
    }
    virtual std::pair<bool,TGAColor> fragment(const Eigen::Vector3f bar) const {
        Eigen::Vector3f weight{bar.x()/clip_vertex_w.x(),bar.y()/clip_vertex_w.y(),bar.z()/clip_vertex_w.z()};
        weight = weight / weight.sum();
        Eigen::Vector3f fragment_pos = view_triangle[0] * weight.x() + view_triangle[1] * weight.y() + view_triangle[2] * weight.z();
        Eigen::Vector3f n = (view_triangle[1] - view_triangle[0]).cross(view_triangle[2] - view_triangle[0]).normalized();
        Eigen::Vector3f v = (campos - fragment_pos).normalized();
        Eigen::Vector3f l = (light_pos - fragment_pos).normalized();
        Eigen::Vector3f h = (v + l).normalized();
        float r_square = std::max((light_pos - fragment_pos).squaredNorm(),1e-6f);
        Eigen::Vector3f ambient = k_ads[0].cwiseProduct(Ia);
        Eigen::Vector3f diffuse = k_ads[1].cwiseProduct(I) * std::max(0.0f,n.dot(l)) / r_square;
        Eigen::Vector3f specular = Eigen::Vector3f::Zero();
        if(n.dot(l) >0){
            specular = (k_ads[2].cwiseProduct(I) * std::pow(std::max(0.0f,n.dot(h)),16) / r_square);        
        }
        Eigen::Vector3f color = ambient + diffuse + specular;
        color = color.cwiseMax(0.0f).cwiseMin(1.0f);
        TGAColor ret = {static_cast<uint8_t>(color.z() * 255.0f + 0.5f),static_cast<uint8_t>(color.y() * 255.0f + 0.5f),static_cast<uint8_t>(color.x() * 255.0f + 0.5f),255};
        return {false,ret};
    }
};

int main(int argc, char** argv) {
    //命令行调用
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " obj/model.obj" << std::endl;
        return 1;
    }
    constexpr int width  = 1024;
    constexpr int height = 1024;
    const Eigen::Vector3f light_pos(0.0f,0.0f,2.0f);
    const Eigen::Vector3f campos(0,0,2);
    const Eigen::Vector3f center(0,0,0);
    const Eigen::Vector3f up(0,1,0);
    TGAImage framebuffer(width, height, TGAImage::RGB);

    init_rasterizer(campos,center,up,width/16,height/16,width*7/8,height*7/8,width,height);

    //处理所有命令行中提及的模型
    for(int i = 1; i < argc ; i++){
        Model model;
        model.load_obj(argv[i]);
        BlinnPhongShader bps(model,light_pos,campos);
        //处理所有面
        for(size_t f = 0; f < model.face_verticies.size() ; f++){
            Triangle clip = {bps.vertex(f,0),bps.vertex(f,1),bps.vertex(f,2)};
            rasterize(clip,bps,framebuffer);
        }
    }

    framebuffer.write_tga_file("framebuffer.tga");
    zbuffer.toTGAImage().write_tga_file("zbuffer.tga");
    return 0;
}