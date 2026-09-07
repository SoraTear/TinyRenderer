#include "render.h"
#include "model.h"

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
void wireframe_render(const std::string& file_path, TGAImage& framebuffer, int width, int height,const TGAColor& ecolor,const TGAColor& vcolor){
    Model model;
    if(!model.load_obj(file_path)) return;
    std::vector<Eigen::Vector2f> scr_verticies;
    //视口变换，把[-1,1]^2变换到[0,width]*[0,height]
    for(const Eigen::Vector3f& v : model.verticies){
        Eigen::Vector2f scr((v.x() + 1.0f) * width / 2 , (v.y() + 1.0f) * height / 2);
        scr_verticies.push_back(scr);
    }
    for(const Eigen::Vector3i& f : model.face_verticies){
        line(scr_verticies[f[0]].x(),scr_verticies[f[0]].y(),scr_verticies[f[1]].x(),scr_verticies[f[1]].y(),framebuffer,ecolor);
        line(scr_verticies[f[1]].x(),scr_verticies[f[1]].y(),scr_verticies[f[2]].x(),scr_verticies[f[2]].y(),framebuffer,ecolor);
        line(scr_verticies[f[2]].x(),scr_verticies[f[2]].y(),scr_verticies[f[0]].x(),scr_verticies[f[0]].y(),framebuffer,ecolor);
    }
    for(const Eigen::Vector2f& v : scr_verticies)
    {
        framebuffer.set(v.x(),v.y(),vcolor);
    }
}

//有符号三角形面积
float triangle_signed_area(int ax,int ay,int bx,int by,int cx,int cy){
    float ret = 0.5f * static_cast<float>((bx - ax) * (cy - ay) - (by - ay) * (cx - ax));
    return ret;
}

//带背面剔除的三角形光栅化
void triangle(int ax, int ay, int bx, int by, int cx, int cy, TGAImage &framebuffer, TGAColor color) {
    //计算包围盒，减少计算量
    std::vector<Eigen::Vector2i> bouding_box;
    bouding_box.push_back(Eigen::Vector2i(std::min(ax,std::min(bx,cx)),std::min(ay,std::min(by,cy))));
    bouding_box.push_back(Eigen::Vector2i(std::max(ax,std::max(bx,cx)),std::max(ay,std::max(by,cy))));

    float S_abc = triangle_signed_area(ax,ay,bx,by,cx,cy);
    //背面剔除，.obj格式规定顶点逆时针排列
    if(S_abc < 1) return;
    //多线程处理
    #pragma omp parallel for
    for(int x = bouding_box[0][0]; x <= bouding_box[1][0]; x++){
        for(int y = bouding_box[0][1]; y <= bouding_box[1][1]; y++){
            //重心坐标
            float alpha = triangle_signed_area(x,y,bx,by,cx,cy) / S_abc;
            float beta = triangle_signed_area(ax,ay,x,y,cx,cy) / S_abc;
            float gamma = triangle_signed_area(ax,ay,bx,by,x,y) / S_abc;
            if(alpha < 0 || beta < 0 || gamma < 0)continue;
            framebuffer.set(x,y,color);
        }    
    }
}

//三角形渲染
void triangle_render(const std::string& file_path, TGAImage& framebuffer, int width, int height){
    Model model;
    if(!model.load_obj(file_path)) return;
    std::vector<Eigen::Vector2f> scr_verticies;
    //视口变换，把[-1,1]^2变换到[0,width]*[0,height]
    for(const Eigen::Vector3f& v : model.verticies){
        Eigen::Vector2f scr((v.x() + 1.0f) * width / 2 , (v.y() + 1.0f) * height / 2);
        scr_verticies.push_back(scr);
    }
    for(const Eigen::Vector3i& f : model.face_verticies){
        TGAColor rc = {static_cast<std::uint8_t>(std::rand() % 256),static_cast<std::uint8_t>(std::rand() % 256),static_cast<std::uint8_t>(std::rand() % 256),255};
        triangle(scr_verticies[f[0]].x(),scr_verticies[f[0]].y(),scr_verticies[f[1]].x(),scr_verticies[f[1]].y(),scr_verticies[f[2]].x(),scr_verticies[f[2]].y(),framebuffer,rc);
    }
}