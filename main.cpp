#include <cmath>
#include "tgaimage.h"

constexpr TGAColor white   = {255, 255, 255, 255}; // attention, BGRA order
constexpr TGAColor green   = {  0, 255,   0, 255};
constexpr TGAColor red     = {  0,   0, 255, 255};
constexpr TGAColor blue    = {255, 128,  64, 255};
constexpr TGAColor yellow  = {  0, 200, 255, 255};

//Bresenham直线绘制算法，输入点对、image对象和直线颜色，画出直线
void line(int ax,int ay,int bx,int by,TGAImage& framebuffer,TGAColor color){
    //判断直线是否“陡峭”
    bool steep = std::abs(ax - bx) < std::abs(ay - by);
    //对“陡峭”的直线，转置处理以保证采样率
    if(steep){
        std::swap(ax,ay);
        std::swap(bx,by);
    }
    //默认b点在右边，a点在左边
    if(ax > bx){
        std::swap(ax,bx);
        std::swap(ay,by);
    }
    //利用error实现的整数y值
    int y = ay;
    /*
     *整数误差，浮点误差累加量为std::abs(by - ay)/static_cast<float>(bx - ax)，比较量为0.5f，减少量为1.0f
     *整数误差 = 浮点误差 * 2 * (bx - ax)
     */
    int error = 0;
    //x逐像素采样，采样最精准且采样次数最少
    for(int x = ax;x <= bx;x++){
        //“陡峭”直线还原转置
        if(steep){
            framebuffer.set(y,x,color);
        }
        else{
            framebuffer.set(x,y,color);
        }
        //后更新y的位置，因为第一轮y=ay；error相关量的更新按比例放大
        error += 2 * std::abs(by - ay);
        if(error > bx - ax){
            y += (ay < by) ? 1 : -1;
            error -= 2 * (bx - ax);
        }
    }
}

int main(int argc, char** argv) {
    constexpr int width  = 64;
    constexpr int height = 64;
    TGAImage framebuffer(width, height, TGAImage::RGB);

    int ax =  7, ay =  3;
    int bx = 12, by = 37;
    int cx = 62, cy = 53;

    line(ax, ay, bx, by, framebuffer, blue);
    line(cx, cy, bx, by, framebuffer, green);
    line(cx, cy, ax, ay, framebuffer, yellow);
    line(ax, ay, cx, cy, framebuffer, red);

    framebuffer.set(ax, ay, white);
    framebuffer.set(bx, by, white);
    framebuffer.set(cx, cy, white);
    framebuffer.write_tga_file("framebuffer.tga");
    return 0;
}

