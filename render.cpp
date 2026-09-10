#include "render.h"

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
float triangle_signed_area(float ax,float ay,float bx,float by,float cx,float cy){
    float ret = 0.5f * ((bx - ax) * (cy - ay) - (by - ay) * (cx - ax));
    return ret;
}

//带背面剔除的三角形光栅化
void triangle(float ax, float ay,float az, float bx, float by, float bz, float cx, float cy, float cz, TGAImage& framebuffer, ZBuffer& zbuffer, TGAColor color) {
    //计算包围盒，减少计算量
    std::vector<Eigen::Vector2f> bouding_box;
    bouding_box.push_back(Eigen::Vector2f(std::min(ax,std::min(bx,cx)),std::min(ay,std::min(by,cy))));
    bouding_box.push_back(Eigen::Vector2f(std::max(ax,std::max(bx,cx)),std::max(ay,std::max(by,cy))));
    float S_abc = triangle_signed_area(ax,ay,bx,by,cx,cy);
    //背面剔除，不忽略小三角形，.obj格式规定顶点逆时针排列（优化）
    if(S_abc < 0.0f) return;
    //多线程处理
    #pragma omp parallel for
    //限制包围盒最大范围
    for(int y = std::max<int>(bouding_box[0][1],0); y <= std::min<int>(bouding_box[1][1],framebuffer.height()-1); y++){
        for(int x = std::max<int>(bouding_box[0][0],0); x <= std::min<int>(bouding_box[1][0],framebuffer.width()-1); x++){
            //重心坐标
            float alpha = triangle_signed_area(x,y,bx,by,cx,cy) / S_abc;
            float beta = triangle_signed_area(ax,ay,x,y,cx,cy) / S_abc;
            float gamma = triangle_signed_area(ax,ay,bx,by,x,y) / S_abc;
            if(alpha < 0 || beta < 0 || gamma < 0)continue;
            //深度，当前设定下值越大深度越小
            float z = az * alpha + bz * beta + cz * gamma;
            if(z <= zbuffer.get(x,y)) continue;
            zbuffer.set(x,y,z);
            framebuffer.set(x,y,color);
        }    
    }
}

//单轴旋转矩阵，0=x,1=y,2=z
Eigen::Matrix4f get_rotation_matrix(int axis,float degree){
    Eigen::Matrix4f rotation;
    float rad_deg = degree * std::numbers::pi_v<float>/ 180.0f;
    switch (axis){
    case 0:
        rotation << 1,0,0,0,
                    0,std::cos(rad_deg),-std::sin(rad_deg),0,
                    0,std::sin(rad_deg),std::cos(rad_deg),0,
                    0,0,0,1;
        return rotation;
    case 1:
        rotation << std::cos(rad_deg),0,std::sin(rad_deg),0,
                    0,1,0,0,
                    -std::sin(rad_deg),0,std::cos(rad_deg),0,
                    0,0,0,1;
        return rotation;
    case 2:
        rotation << std::cos(rad_deg),-std::sin(rad_deg),0,0,
                    std::sin(rad_deg),std::cos(rad_deg),0,0,
                    0,0,1,0,
                    0,0,0,1;
        return rotation;
    default:
        return Eigen::Matrix4f::Identity();
    }
}

//视图变换矩阵，将世界坐标系转换到相机坐标系
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

//透视除法矩阵，约定投影平面为z=0，distance为视口变换后相机到投影平面的距离
Eigen::Matrix4f get_projection_matrix(const float distance){
    Eigen::Matrix4f projection;
    projection << 1,0,0,0,
                  0,1,0,0,
                  0,0,1,0,
                  0,0,-1.0f/distance,1;
    return projection;
}

//视口变换矩阵，(x,y)为视口左下角坐标，将[-1,1]^2的NDC x,y坐标映射到指定视口空间
Eigen::Matrix4f get_viewport_matrix(int x,int y,int width,int height){
    Eigen::Matrix4f viewport;
    viewport << width/2.0f,0,0,x+width/2.0f,
                0,height/2.0f,0,y+height/2.0f,
                0,0,1,0,
                0,0,0,1;
    return viewport;
}

//三角形渲染
void triangle_render(const std::string& file_path, TGAImage& framebuffer, ZBuffer& zbuffer, int width, int height){
    Model model;
    if(!model.load_obj(file_path)) return;
    std::vector<Eigen::Vector3f> scr_verticies;
    Eigen::Vector3f campos(-1,0,2);
    Eigen::Vector3f center(0,0,0);
    Eigen::Vector3f up(0,1,0);
    Eigen::Matrix4f v = get_view_matrix(campos,center,up);
    Eigen::Matrix4f p = get_projection_matrix((center - campos).norm());
    Eigen::Matrix4f vp = get_viewport_matrix(width/16,height/16,width * 7/8,height * 7/8);
    for(const Eigen::Vector3f& vertex : model.verticies){
        //裁剪空间坐标，未进行透视除法
        Eigen::Vector4f clip = p * v * Eigen::Vector4f(vertex.x(),vertex.y(),vertex.z(),1.0f);
        //NDC坐标，进行了透视除法
        Eigen::Vector4f ndc = clip/clip.w();
        //平面空间坐标，进行了视口变换
        Eigen::Vector3f scr = (vp * ndc).head<3>();
        scr_verticies.push_back(scr);
    }
    for(const Eigen::Vector3i& f : model.face_verticies){
        TGAColor rc = {static_cast<std::uint8_t>(std::rand() % 256),static_cast<std::uint8_t>(std::rand() % 256),static_cast<std::uint8_t>(std::rand() % 256),255};
        triangle(scr_verticies[f[0]].x(),scr_verticies[f[0]].y(),scr_verticies[f[0]].z(),
                 scr_verticies[f[1]].x(),scr_verticies[f[1]].y(),scr_verticies[f[1]].z(),
                 scr_verticies[f[2]].x(),scr_verticies[f[2]].y(),scr_verticies[f[2]].z(),
                 framebuffer, zbuffer ,rc);
    }
}