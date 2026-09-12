#pragma once
#include "rasterizer.h"

struct BlinnPhongShader : IShader{
    const Model& model;
    std::vector<Eigen::Vector3f> light_pos;
    Eigen::Vector3f campos; 
    Eigen::Vector3f k_ads[3] = {Eigen::Vector3f(0.1f,0.1f,0.1f),Eigen::Vector3f(0.7f,0.7f,0.7f),Eigen::Vector3f(0.8f,0.8f,0.8f)};
    Eigen::Vector3f I{1.5f,1.5f,1.5f};
    Eigen::Vector3f Ia{1.0f,1.0f,1.0f};
    Eigen::Vector3f view_triangle[3];
    Eigen::Vector3f view_triangle_normal[3];
    Eigen::Vector3f triangle_texture[3];
    Eigen::Vector3f clip_vertex_w;

    BlinnPhongShader(const Model& model,const std::vector<Eigen::Vector3f>& light_pos,const Eigen::Vector3f& campos);

    virtual Eigen::Vector4f vertex(const int f, const int f_index);
    virtual std::pair<bool,TGAColor> fragment(const Eigen::Vector3f bar) const override;
};

struct TextureMapShader : BlinnPhongShader{
    TGAImage texture_nm;
    TGAImage texture_diff;
    TGAImage texture_spec;

    TextureMapShader(const Model& model,const std::vector<Eigen::Vector3f>& light_pos,const Eigen::Vector3f& campos);
    virtual std::pair<bool,TGAColor> fragment(const Eigen::Vector3f bar) const override;
};