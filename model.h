#pragma once
#include <vector>
#include <string>
#include <Eigen/Eigen>

struct Model
{
    std::string file_path;
    //obj模型顶点v
    std::vector<Eigen::Vector3f> verticies;
    //obj模型顶点法线vn
    std::vector<Eigen::Vector3f> verticies_normal;
    //obj模型纹理坐标vt
    std::vector<Eigen::Vector3f> verticies_texture;
    //obj模型面f：记录使用顶点索引
    std::vector<Eigen::Vector3i> face_verticies;
    //obj模型面f：记录使用顶点法线索引
    std::vector<Eigen::Vector3i> face_verticies_normal;
    //obj模型面f：记录使用顶点纹理坐标索引
    std::vector<Eigen::Vector3i> face_verticies_texture;

    bool load_obj(const std::string& file_path);
};
