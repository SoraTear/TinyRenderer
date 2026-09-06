#include "model.h"
#include <iostream>
#include <fstream>
#include <sstream>

bool Model::load_obj(const std::string& file_path)
{
    std::ifstream obj_file;
    //读取行
    obj_file.open(file_path);
    if (!obj_file.is_open()) {
        std::cerr << "can't open .obj file " << file_path << "\n";
        return false;
    }
    //重复调用时清空上次模型信息
    verticies.clear();
    face_verticies.clear();
    //读取的一行
    std::string line;
    while (std::getline(obj_file,line)){
        //从字符串line中读取数据
        std::istringstream data(line);
        std::string type;
        data >> type;
        //v 0.5614145 17.82617 -0.8834122
        if(type == "v"){
            Eigen::Vector3f v;
            for (int i = 0; i < 3; i++){
                if(!(data >> v[i])){
                    std::cerr << "read vertecies failed" << "\n";
                    return false;
                }
            }
            verticies.push_back(v);
        }
        //f 3695/3695/3695 3696/3696/3696 3697/3697/3697，每组第一个数为v索引（从1开始）
        else if(type == "f"){
            Eigen::Vector3i f;
            //从line中提取子串，以空格分隔
            std::string sub_string;
            for (int i = 0; i < 3; i++){
                if(!(data >> sub_string))
                {
                    std::cerr << "read faces failed" << "\n";
                    return false;
                }
                //从子串中提取被'/'隔开的第一部分
                f[i] = std::stoi(sub_string.substr(0,sub_string.find('/'))) - 1;
            }
            face_verticies.push_back(f);
        }   
    }
    return true;
}