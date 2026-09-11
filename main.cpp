#include "rasterizer.h"
#include <iostream>
#include <cstring>

constexpr TGAColor white   = {255, 255, 255, 255}; // attention, BGRA order
constexpr TGAColor green   = {  0, 255,   0, 255};
constexpr TGAColor red     = {  0,   0, 255, 255};
constexpr TGAColor blue    = {255, 128,  64, 255};
constexpr TGAColor yellow  = {  0, 200, 255, 255};
constexpr TGAColor cyan    = {255, 255,   0, 255};
constexpr TGAColor magenta = {255,   0, 255, 255};
extern Eigen::Matrix4f view,projection,viewport;
extern ZBuffer zbuffer;

//RandomShader实现IShader接口
struct RandomShader : IShader{
    const Model& model;
    TGAColor color = {};
    //相机空间三角形
    Eigen::Vector3f view_triangle[3];
    RandomShader(const Model& model) : model(model){};
    //输入模型中的面索引和面上点索引，将找到的点变换到相机空间并记入相机空间三角形，返回投影变换后的该点（裁剪空间）
    virtual Eigen::Vector4f vertex(const int f, const int f_index) {
        Eigen::Vector3f model_vertex = model.verticies[model.face_verticies[f][f_index]];
        Eigen::Vector4f view_vertex = view * Eigen::Vector4f(model_vertex.x(),model_vertex.y(),model_vertex.z(),1.0f);
        view_triangle[f_index] = view_vertex.head<3>();
        return projection * view_vertex;
    }
    //RandomShader对片段的着色，目前不忽略片段且直接返回color
    virtual std::pair<bool,TGAColor> fragment(const Eigen::Vector3f bar) const {
        return {false, color};
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
    const Eigen::Vector3f campos(-1,0,2);
    const Eigen::Vector3f center(0,0,0);
    const Eigen::Vector3f up(0,1,0);
    TGAImage framebuffer(width, height, TGAImage::RGB);

    init_rasterizer(campos,center,up,width/16,height/16,width*7/8,height*7/8,width,height);

    //处理所有命令行中提及的模型
    for(int i = 1; i < argc ; i++){
        Model model;
        model.load_obj(argv[i]);
        //RandomShader获取当前处理模型的引用
        RandomShader rs(model);
        //处理所有面
        for(size_t f = 0; f < model.face_verticies.size() ; f++){
            //每个面随机一个固定颜色
            rs.color = {static_cast<uint8_t>(std::rand()%256),static_cast<uint8_t>(std::rand()%256),static_cast<uint8_t>(std::rand()%256), 255 };
            //构造裁剪空间三角形用于光栅化
            Triangle clip = {rs.vertex(f,0),rs.vertex(f,1),rs.vertex(f,2)};
            rasterize(clip,rs,framebuffer);
        }
    }

    framebuffer.write_tga_file("framebuffer.tga");
    zbuffer.toTGAImage().write_tga_file("zbuffer.tga");
    return 0;
}