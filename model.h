#pragma once
#include <vector>
#include <string>
#include <Eigen/Eigen>

struct Model
{
    //obj模型顶点v
    std::vector<Eigen::Vector3f> verticies;
    //obj模型面f
    std::vector<Eigen::Vector3i> face_verticies;
    bool load_obj(const std::string& file_path);
};
