#pragma once
#include <cmath>
#include "tgaimage.h"
#include "model.h"

struct ZBuffer{
    
    ZBuffer() = default;
    ZBuffer(const int width,const int height);
    float get(const int x,const int y) const;
    void set(const int x,const int y,const float value);
    TGAImage toTGAImage();
    
    private:
    int width = 0;
    int height = 0;
    std::vector<float> data = {};
};

void line(int ax,int ay,int bx,int by,TGAImage& framebuffer,TGAColor color);
void wireframe_render(const std::string& file_path, TGAImage& framebuffer, int width, int height,const TGAColor& ecolor,const TGAColor& vcolor);
void triangle(int ax, int ay,int az, int bx, int by, int bz, int cx, int cy, int cz, TGAImage& framebuffer, TGAImage& zbuffer, TGAColor color);
void triangle_render(const std::string& file_path, TGAImage& framebuffer, ZBuffer& zbuffer, int width, int height);