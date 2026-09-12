#include <iostream>
#include <cstring>
#include "tgaimage.h"

TGAImage::TGAImage(const int w, const int h, const int bpp) : w(w), h(h), bpp(bpp), data(w*h*bpp, 0) {}// :stat(val)赋值给成员

bool TGAImage::read_tga_file(const std::string filename) {
    std::ifstream in;
    // ios::binary原样读取字节
    in.open(filename, std::ios::binary);
    if (!in.is_open()) {
        std::cerr << "can't open file " << filename << "\n";
        return false;
    }
    //存储文件头
    TGAHeader header; 
    // (&header)取header起始地址，reinterpret_cast<char *>按照char*解释该指针,读取文件的header大小个字节到header的内存空间
    in.read(reinterpret_cast<char *>(&header), sizeof(header)); 
    if (!in.good()) {
        std::cerr << "an error occured while reading the header\n";
        return false;
    }
    w   = header.width;
    h   = header.height;
    //bpp：每像素字节数；bitsperpixel：每像素bit数
    bpp = header.bitsperpixel>>3;
    if (w<=0 || h<=0 || (bpp!=GRAYSCALE && bpp!=RGB && bpp!=RGBA)) {
        std::cerr << "bad bpp (or width/height) value\n";
        return false;
    }
    //data大小为像素数乘以每像素字节数
    size_t nbytes = bpp*w*h;
    //用0初始化data数组
    data = std::vector<std::uint8_t>(nbytes, 0);
    //判断格式
    if (3==header.datatypecode || 2==header.datatypecode) {
        //未压缩，data.data()返回data起始地址指针，读取nbytes个字节到data里
        in.read(reinterpret_cast<char *>(data.data()), nbytes);
        if (!in.good()) {
            std::cerr << "an error occured while reading the data\n";
            return false;
        }
    } else if (10==header.datatypecode||11==header.datatypecode) {
        //使用了RLE压缩，调用load_rle_data解压并写入data
        if (!load_rle_data(in)) {
            std::cerr << "an error occured while reading the data\n";
            return false;
        }
    } else {
        std::cerr << "unknown file format " << (int)header.datatypecode << "\n";
        return false;
    }
    //根据标志位imagedescriptor的4，5位决定是否翻转
    if (!(header.imagedescriptor & 0x20))
        flip_vertically();
    if (header.imagedescriptor & 0x10)
        flip_horizontally();
    std::cerr << w << "x" << h << "/" << bpp*8 << "\n";
    return true;
}

bool TGAImage::load_rle_data(std::ifstream &in) {
    size_t pixelcount = w*h;
    size_t currentpixel = 0;
    size_t currentbyte  = 0;
    TGAColor colorbuffer;
    do {
        std::uint8_t chunkheader = 0;
        //get读一个字节给8bit的RLE压缩块头chunkheader
        chunkheader = in.get();
        if (!in.good()) {
            std::cerr << "an error occured while reading the data\n";
            return false;
        }
        //chunkheader<128，后面是chunkheader+1个未压缩的像素
        if (chunkheader<128) {
            chunkheader++;
            for (int i=0; i<chunkheader; i++) {
                //从块内读取bpp个字节的数据到颜色缓冲
                in.read(reinterpret_cast<char *>(colorbuffer.bgra), bpp);
                if (!in.good()) {
                    std::cerr << "an error occured while reading the header\n";
                    return false;
                }
                //颜色缓冲写入data，由bpp控制写入量
                for (int t=0; t<bpp; t++)
                    data[currentbyte++] = colorbuffer.bgra[t];
                currentpixel++;
                if (currentpixel>pixelcount) {
                    std::cerr << "Too many pixels read\n";
                    return false;
                }
            }
          // 如果大于128，则有chunkheader-127个同像素
        } else {
            chunkheader -= 127;
            //读取颜色到缓冲，只读一次反复写入data
            in.read(reinterpret_cast<char *>(colorbuffer.bgra), bpp);
            if (!in.good()) {
                std::cerr << "an error occured while reading the header\n";
                return false;
            }
            for (int i=0; i<chunkheader; i++) {
                for (int t=0; t<bpp; t++)
                    data[currentbyte++] = colorbuffer.bgra[t];
                currentpixel++;
                if (currentpixel>pixelcount) {
                    std::cerr << "Too many pixels read\n";
                    return false;
                }
            }
        }
    } while (currentpixel < pixelcount);
    return true;
}

bool TGAImage::write_tga_file(const std::string filename, const bool vflip, const bool rle) const {
    /*TGA文件格式：Header(18Byte)；
                  图像数据；
                  Extension Area偏移(4Byte)；
                  Developer Area偏移(4Byte)；
                  TGA2.0签名"TRUEVISION-XFILE.\0"
    */
    constexpr std::uint8_t developer_area_ref[4] = {0, 0, 0, 0};
    constexpr std::uint8_t extension_area_ref[4] = {0, 0, 0, 0};
    constexpr std::uint8_t footer[18] = {'T','R','U','E','V','I','S','I','O','N','-','X','F','I','L','E','.','\0'};
    std::ofstream out;
    out.open(filename, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "can't open file " << filename << "\n";
        return false;
    }
    TGAHeader header = {};
    header.bitsperpixel = bpp<<3;
    header.width  = w;
    header.height = h;
    //根据是否为灰度图和是否启用压缩决定图像数据类型
    header.datatypecode = (bpp==GRAYSCALE ? (rle?11:3) : (rle?10:2));
    //如果启用垂直翻转，图像原点在左下，否则在左上
    header.imagedescriptor = vflip ? 0x00 : 0x20; // top-left or bottom-left origin
    out.write(reinterpret_cast<const char *>(&header), sizeof(header));
    if (!out.good()) goto err;
    //如果没启用压缩，则直接写data数据
    if (!rle) {
        out.write(reinterpret_cast<const char *>(data.data()), w*h*bpp);
        if (!out.good()) goto err;
      //如果启用压缩，则调用unload_rle_data压缩data，写压缩后的数据
    } else if (!unload_rle_data(out)) goto err;
    out.write(reinterpret_cast<const char *>(developer_area_ref), sizeof(developer_area_ref));
    if (!out.good()) goto err;
    out.write(reinterpret_cast<const char *>(extension_area_ref), sizeof(extension_area_ref));
    if (!out.good()) goto err;
    out.write(reinterpret_cast<const char *>(footer), sizeof(footer));
    if (!out.good()) goto err;
    return true;
err:
    std::cerr << "can't dump the tga file\n";
    return false;
}

bool TGAImage::unload_rle_data(std::ofstream &out) const {
    const std::uint8_t max_chunk_length = 128;
    size_t npixels = w*h;
    size_t curpix = 0;
    //写块的循环
    while (curpix<npixels) {
        //chunkstart：当前块在data中开始字节；curbyte：当前处理到的字节
        size_t chunkstart = curpix*bpp;
        size_t curbyte = curpix*bpp;
        //当前块中像素数，初始为1（data中没有块头）
        std::uint8_t run_length = 1;
        //是否为原始像素块，默认为是
        bool raw = true;
        //分析块，处理的第二个像素未到末尾，且run_length在限定范围内
        while (curpix+run_length<npixels && run_length<max_chunk_length) {
            //相邻像素是否相同
            bool succ_eq = true;
            //bpp限定比较宽度，比较相邻像素的各个通道是否均相同，如果都相同，则为同一像素
            for (int t=0; succ_eq && t<bpp; t++)
                succ_eq = (data[curbyte+t]==data[curbyte+t+bpp]);
            //比较完毕，处理像素向前移一个
            curbyte += bpp;
            //第一次比较决定此块是否压缩，如果第一次得出两个相邻像素相同，则启用压缩，raw为false
            if (1==run_length)
                raw = !succ_eq;
            //如果是原始像素块，比较到了两个相同像素，则应该撤销上次比较后开新块
            if (raw && succ_eq) {
                run_length--;
                break;
            }
            //如果是压缩像素块，比较到了两个不同像素，无需撤销比较，开新块
            if (!raw && !succ_eq)
                break;
            //更新当前块内有多少个像素
            run_length++;
        }
        //推进处理进度
        curpix += run_length;
        //写一字节：若未压缩，则run_length = chunkheader + 1；若压缩，则run_length = chunkheader - 127
        out.put(raw ? run_length-1 : run_length+127);
        if (!out.good()) return false;
        //从chunkstart处写当前块，如果未压缩则有多少写多少，压缩了就只写一个
        out.write(reinterpret_cast<const char *>(data.data()+chunkstart), (raw?run_length*bpp:bpp));
        if (!out.good()) return false;
    }
    return true;
}

TGAColor TGAImage::get(const int x, const int y) const {
    //data.size()返回data数组长度
    if (!data.size() || x<0 || y<0 || x>=w || y>=h) return {};
    //创建返回元素，返回{bgra[0],bgra[1],bgra[2],bgra[3],bytespp}
    TGAColor ret = {0, 0, 0, 0, bpp};
    //(x+y*w)*bpp在data的起始地址上寻找指定地址
    const std::uint8_t *p = data.data()+(x+y*w)*bpp;
    //复制BGRA通道
    for (int i=bpp; i--; ret.bgra[i] = p[i]);
    return ret;
}

void TGAImage::set(int x, int y, const TGAColor &c) {
    if (!data.size() || x<0 || y<0 || x>=w || y>=h) return;
    //memcpy(target,source,bytenum)
    memcpy(data.data()+(x+y*w)*bpp, c.bgra, bpp);
}

void TGAImage::flip_horizontally() {
    //对每个左半行
    for (int i=0; i<w/2; i++)
        //对每个左半行上的整列
        for (int j=0; j<h; j++)
            for (int b=0; b<bpp; b++)
                //交换各个通道值
                std::swap(data[(i+j*w)*bpp+b], data[(w-1-i+j*w)*bpp+b]);
}

void TGAImage::flip_vertically() {
    //对每个上半列上的整行
    for (int i=0; i<w; i++)
        //对每个上半列
        for (int j=0; j<h/2; j++)
            for (int b=0; b<bpp; b++)
                std::swap(data[(i+j*w)*bpp+b], data[(i+(h-1-j)*w)*bpp+b]);
}

int TGAImage::width() const {
    return w;
}

int TGAImage::height() const {
    return h;
}

int TGAImage::get_bpp() const {
    return static_cast<int>(bpp);
}
