#include <cmath>
#include <string>
#include <numbers>
#include "tgaimage.h"
#include "model.h"

constexpr TGAColor white   = {255, 255, 255, 255}; // attention, BGRA order
constexpr TGAColor green   = {  0, 255,   0, 255};
constexpr TGAColor red     = {  0,   0, 255, 255};
constexpr TGAColor blue    = {255, 128,  64, 255};
constexpr TGAColor yellow  = {  0, 200, 255, 255};
constexpr TGAColor cyan    = {255, 255,   0, 255};
constexpr TGAColor magenta = {255,   0, 255, 255};

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

//线框渲染
void wireframe_render(const std::string& file_path, TGAImage& framebuffer, int width, int height){
    Model model;
    if(!model.load_obj(file_path)) return;
    std::vector<Eigen::Vector2f> scr_verticies;
    //视口变换，把[-1,1]^2变换到[0,width]*[0,height]
    for(const Eigen::Vector3f& v : model.verticies){
        Eigen::Vector2f scr((v.x() + 1.0f) * width / 2 , (v.y() + 1.0f) * height / 2);
        scr_verticies.push_back(scr);
    }
    for(const Eigen::Vector3i& f : model.face_verticies){
        line(scr_verticies[f[0]].x(),scr_verticies[f[0]].y(),scr_verticies[f[1]].x(),scr_verticies[f[1]].y(),framebuffer,cyan);
        line(scr_verticies[f[1]].x(),scr_verticies[f[1]].y(),scr_verticies[f[2]].x(),scr_verticies[f[2]].y(),framebuffer,cyan);
        line(scr_verticies[f[2]].x(),scr_verticies[f[2]].y(),scr_verticies[f[0]].x(),scr_verticies[f[0]].y(),framebuffer,cyan);
    }
    for(const Eigen::Vector2f& v : scr_verticies)
    {
        framebuffer.set(v.x(),v.y(),magenta);
    }
}

int main(int argc, char** argv) {
    constexpr int width  = 1024;
    constexpr int height = 1024;
    TGAImage framebuffer(width, height, TGAImage::RGB);

    wireframe_render("obj/diablo3_pose/diablo3_pose.obj",framebuffer,width,height);

    framebuffer.write_tga_file("framebuffer.tga");
    return 0;
}

