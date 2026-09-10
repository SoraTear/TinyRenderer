#include "render.h"

constexpr TGAColor white   = {255, 255, 255, 255}; // attention, BGRA order
constexpr TGAColor green   = {  0, 255,   0, 255};
constexpr TGAColor red     = {  0,   0, 255, 255};
constexpr TGAColor blue    = {255, 128,  64, 255};
constexpr TGAColor yellow  = {  0, 200, 255, 255};
constexpr TGAColor cyan    = {255, 255,   0, 255};
constexpr TGAColor magenta = {255,   0, 255, 255};



int main(int argc, char** argv) {
    constexpr int width  = 1024;
    constexpr int height = 1024;
    TGAImage framebuffer(width, height, TGAImage::RGB);
    ZBuffer zbuffer(width,height);

    triangle_render("obj/diablo3_pose/diablo3_pose.obj",framebuffer,zbuffer,width,height);

    framebuffer.write_tga_file("framebuffer.tga");
    zbuffer.toTGAImage().write_tga_file("zbuffer.tga");
    return 0;
}