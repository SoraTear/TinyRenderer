#pragma once
#include <cmath>
#include "tgaimage.h"
#include "model.h"

void line(int ax,int ay,int bx,int by,TGAImage& framebuffer,TGAColor color);
void wireframe_render(const std::string& file_path, TGAImage& framebuffer, int width, int height,const TGAColor& ecolor,const TGAColor& vcolor);