#include "shader.h"

extern Eigen::Matrix4f view,projection,viewport;
extern ZBuffer zbuffer;

BlinnPhongShader::BlinnPhongShader(const Model& model,const std::vector<Eigen::Vector3f>& light_pos,const Eigen::Vector3f& campos) : model(model){
    for(auto l : light_pos){
        this->light_pos.push_back((view * Eigen::Vector4f(l.x(),l.y(),l.z(),1.0f)).head<3>()); 
    }
    this->campos = (view * Eigen::Vector4f(campos.x(),campos.y(),campos.z(),1.0f)).head<3>();
};
//输入模型中的面索引和面上点索引，将找到的点变换到相机空间并记入相机空间三角形，返回投影变换后的该点（裁剪空间）
Eigen::Vector4f BlinnPhongShader::vertex(const int f, const int f_index) {
    Eigen::Vector3f model_vertex = model.verticies[model.face_verticies[f][f_index]];
    Eigen::Vector3f model_vertex_normal = model.verticies_normal[model.face_verticies_normal[f][f_index]];
    Eigen::Vector4f view_vertex = view * Eigen::Vector4f(model_vertex.x(),model_vertex.y(),model_vertex.z(),1.0f);
    Eigen::Vector4f view_vertex_normal = view * Eigen::Vector4f(model_vertex_normal.x(),model_vertex_normal.y(),model_vertex_normal.z(),0.0f);
    view_triangle[f_index] = view_vertex.head<3>();
    view_triangle_normal[f_index] = view_vertex_normal.head<3>();
    triangle_texture[f_index] = model.verticies_texture[model.face_verticies_texture[f][f_index]];
    Eigen::Vector4f clip_vertex = projection * view_vertex;
    clip_vertex_w[f_index] = clip_vertex.w();
    return clip_vertex;
}

std::pair<bool,TGAColor> BlinnPhongShader::fragment(const Eigen::Vector3f bar) const {
    Eigen::Vector3f weight{bar.x()/clip_vertex_w.x(),bar.y()/clip_vertex_w.y(),bar.z()/clip_vertex_w.z()};
    weight = weight / weight.sum();
    Eigen::Vector3f fragment_pos = view_triangle[0] * weight.x() + view_triangle[1] * weight.y() + view_triangle[2] * weight.z();
    /* Flat shading
    Eigen::Vector3f n = (view_triangle[1] - view_triangle[0]).cross(view_triangle[2] - view_triangle[0]).normalized();
    */
    Eigen::Vector3f n = (view_triangle_normal[0] * weight[0] + view_triangle_normal[1] * weight[1] + view_triangle_normal[2] * weight[2]).normalized();
    Eigen::Vector3f v = (campos - fragment_pos).normalized();
    
    Eigen::Vector3f ambient = k_ads[0].cwiseProduct(Ia);
    Eigen::Vector3f diffuse = Eigen::Vector3f::Zero();
    Eigen::Vector3f specular = Eigen::Vector3f::Zero();

    for(auto light : light_pos){
        Eigen::Vector3f l = (light - fragment_pos).normalized();
        Eigen::Vector3f h = (v + l).normalized();
        float r_square = std::max((light - fragment_pos).squaredNorm(),1e-6f);
        Eigen::Vector3f diffuse = diffuse + (k_ads[1].cwiseProduct(I) * std::max(0.0f,n.dot(l)) / r_square);
        if(n.dot(l) >0){
            specular = specular + (k_ads[2].cwiseProduct(I) * std::pow(std::max(0.0f,n.dot(h)),16) / r_square);        
        }
    }
    
    Eigen::Vector3f color = ambient + diffuse + specular;
    color = color.cwiseMax(0.0f).cwiseMin(1.0f);
    TGAColor ret = {static_cast<uint8_t>(color.z() * 255.0f + 0.5f),static_cast<uint8_t>(color.y() * 255.0f + 0.5f),static_cast<uint8_t>(color.x() * 255.0f + 0.5f),255};
    return {false,ret};
}

TextureMapShader::TextureMapShader(const Model& model,const std::vector<Eigen::Vector3f>& light_pos,const Eigen::Vector3f& campos) : BlinnPhongShader(model,light_pos,campos){
    texture_nm.read_tga_file(model.file_path.substr(0,model.file_path.find_last_of('.')) + "_nm.tga");
    texture_diff.read_tga_file(model.file_path.substr(0,model.file_path.find_last_of('.')) + "_diffuse.tga");
    texture_spec.read_tga_file(model.file_path.substr(0,model.file_path.find_last_of('.')) + "_spec.tga");
}

std::pair<bool,TGAColor> TextureMapShader::fragment(const Eigen::Vector3f bar) const {
    Eigen::Vector3f weight{bar.x()/clip_vertex_w.x(),bar.y()/clip_vertex_w.y(),bar.z()/clip_vertex_w.z()};
    weight = weight / weight.sum();
    Eigen::Vector3f fragment_pos = view_triangle[0] * weight.x() + view_triangle[1] * weight.y() + view_triangle[2] * weight.z();

    Eigen::Vector2f uv_f(triangle_texture[0].x() * weight.x() + triangle_texture[1].x() * weight.y() + triangle_texture[2].x() * weight.z(),
                        1.0f - (triangle_texture[0].y() * weight.x() + triangle_texture[1].y() * weight.y() + triangle_texture[2].y() * weight.z()));
    Eigen::Vector2i uv_nm_i = Eigen::Vector2i(std::min<int>(uv_f.x() * texture_nm.width(),texture_nm.width() - 1.0f), std::min<int>(uv_f.y() * texture_nm.height(),texture_nm.height() - 1.0f));
    Eigen::Vector2i uv_diff_i = Eigen::Vector2i(std::min<int>(uv_f.x() * texture_diff.width(),texture_diff.width() - 1.0f), std::min<int>(uv_f.y() * texture_diff.height(),texture_diff.height() - 1.0f));
    Eigen::Vector2i uv_spec_i = Eigen::Vector2i(std::min<int>(uv_f.x() * texture_spec.width(),texture_spec.width() - 1.0f), std::min<int>(uv_f.y() * texture_spec.height(),texture_spec.height() - 1.0f));
    
    TGAColor n_rgb = texture_nm.get(uv_nm_i.x(),uv_nm_i.y());
    Eigen::Vector3f model_n = Eigen::Vector3f(n_rgb[2] , n_rgb[1] , n_rgb[0]) * 2.0f / 255.0f - Eigen::Vector3f::Ones();
    Eigen::Vector3f n = (view * Eigen::Vector4f(model_n.x(),model_n.y(),model_n.z(),0.0f)).head<3>().normalized();

    TGAColor diff_rgb = texture_diff.get(uv_diff_i.x(),uv_diff_i.y());
    Eigen::Vector3f kd = Eigen::Vector3f(diff_rgb[2],diff_rgb[1],diff_rgb[0]) / 255.0f;

    TGAColor spec_rgb = texture_spec.get(uv_spec_i.x(),uv_spec_i.y());
    
    Eigen::Vector3f v = (campos - fragment_pos).normalized();
    Eigen::Vector3f ambient = Eigen::Vector3f::Zero();
    Eigen::Vector3f diffuse = Eigen::Vector3f::Zero();
    Eigen::Vector3f specular = Eigen::Vector3f::Zero();

    for(auto light : light_pos){
        Eigen::Vector3f l = (light - fragment_pos).normalized();
        Eigen::Vector3f h = (v + l).normalized();
        float r_square = std::max((light - fragment_pos).squaredNorm(),1e-6f);
        diffuse = diffuse + kd.cwiseProduct(I) * std::max(0.0f,n.dot(l)) / r_square;
        if(n.dot(l) >0){
            if(texture_spec.get_bpp() == TGAImage::GRAYSCALE){
                float ks = spec_rgb[0] / 255.0f;
                specular = specular + (ks * I * std::pow(std::max(0.0f,n.dot(h)),16) / r_square);      
            }else if(texture_spec.get_bpp() == TGAImage::RGB){
                Eigen::Vector3f ks(spec_rgb[2] / 255.0f,spec_rgb[1] / 255.0f,spec_rgb[0] / 255.0f);
                specular = specular + (ks.cwiseProduct(I) * std::pow(std::max(0.0f,n.dot(h)),16) / r_square);       
            }  
        }
    }
    Eigen::Vector3f color = ambient + diffuse + specular;
    color = color.cwiseMax(0.0f).cwiseMin(1.0f);
    TGAColor ret = {static_cast<uint8_t>(color.z() * 255.0f + 0.5f),static_cast<uint8_t>(color.y() * 255.0f + 0.5f),static_cast<uint8_t>(color.x() * 255.0f + 0.5f),255};
    return {false,ret};
}
