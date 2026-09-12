#pragma once
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

#pragma pack(push,1)
struct TGAHeader {
    std::uint8_t  idlength = 0;
    std::uint8_t  colormaptype = 0;
    std::uint8_t  datatypecode = 0;
    std::uint16_t colormaporigin = 0;
    std::uint16_t colormaplength = 0;
    std::uint8_t  colormapdepth = 0;
    std::uint16_t x_origin = 0;
    std::uint16_t y_origin = 0;
    std::uint16_t width = 0;
    std::uint16_t height = 0;
    std::uint8_t  bitsperpixel = 0;
    std::uint8_t  imagedescriptor = 0;
};
#pragma pack(pop)

struct TGAColor {
    std::uint8_t bgra[4] = {0,0,0,0};
    std::uint8_t bytespp = 4;
     //索引器实现，&返回引用（TGA颜色顺序：BGRA）
    std::uint8_t& operator[](const int i) { return bgra[i]; }
};

struct TGAImage {
    //使用：Format f = RGB;
    enum Format { GRAYSCALE=1, RGB=3, RGBA=4 };
    //生成默认构造器
    TGAImage() = default; 
     //构造器，由.cpp实现
    TGAImage(const int w, const int h, const int bpp);
    //输入指定路径，读TGA文件，写入bpp、w、h和data
    bool  read_tga_file(const std::string filename);
    //指定输出路径、是否启用垂直翻转和是否启用压缩，将data、w、h和bpp信息输出为TGA文件
    //保存为TGA：文件名，垂直翻转，是否启用RLE压缩；不修改对象的成员
    bool write_tga_file(const std::string filename, const bool vflip=true, const bool rle=true) const;
    //水平翻转图像
    void flip_horizontally();
    //竖直翻转图像
    void flip_vertically();
    //读取图像中(x,y)坐标处的颜色，返回一个TGAColor对象
    TGAColor get(const int x, const int y) const;
    //设定c为data中一个像素的值
    void set(const int x, const int y, const TGAColor &c);
    //获取图像宽
    int width()  const;
    //获取图像高
    int height() const;
    int get_bpp() const;
private:
    //接收输入流引用，辅助read_tga_file读取压缩的文件到data
    bool   load_rle_data(std::ifstream &in);
    //接收输出流引用，辅助write_tga_file写压缩后内容到文件
    bool unload_rle_data(std::ofstream &out) const;
    int w = 0, h = 0;
    //每像素字节数
    std::uint8_t bpp = 0;
    //data存储整个图像的所有像素的所有通道（BGRA）
    std::vector<std::uint8_t> data = {};
};

