#include "shader.h"
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



int main(int argc, char** argv) {
    constexpr int width  = 1024;
    constexpr int height = 1024;

    //命令行调用
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " obj/model.obj" << std::endl;
        return 1;
    }
    const std::vector<Eigen::Vector3f> light_pos{Eigen::Vector3f(0.1f,1.0f,1.7f),
                                                 Eigen::Vector3f(0.1f,-1.0f,1.7f),
                                                 };
    const Eigen::Vector3f campos(-1,0,2);
    const Eigen::Vector3f center(0,0,0);
    const Eigen::Vector3f up(0,1,0);
    TGAImage framebuffer(width, height, TGAImage::RGB);

    init_rasterizer(campos,center,up,width/16,height/16,width*7/8,height*7/8,width,height);

    //处理所有命令行中提及的模型
    for(int i = 1; i < argc ; i++){
        Model model;
        model.load_obj(argv[i]);
        TextureMapShader tms(model,light_pos,campos);
        //处理所有面
        for(size_t f = 0; f < model.face_verticies.size() ; f++){
            Triangle clip = {tms.vertex(f,0),tms.vertex(f,1),tms.vertex(f,2)};
            rasterize(clip,tms,framebuffer);
        }
    }

    framebuffer.write_tga_file("framebuffer.tga");
    zbuffer.toTGAImage().write_tga_file("zbuffer.tga");
    return 0;
}
//.\tinyrenderer.exe obj/african_head/african_head.obj obj/african_head/african_head_eye_outer.obj obj/african_head/african_head_eye_inner.obj
//.\tinyrenderer.exe obj/african_head/african_head.obj  obj/african_head/african_head_eye_inner.obj
