#include "matrix.h"

#include <fstream>
#include <string>

Matrix::Matrix(int R_)
  : R(R_), buf(R*R*R, Void) {}

Matrix::Matrix(const std::string& input_path)
  : R(0xff) {
    std::FILE* fp = std::fopen(input_path.c_str(), "rb");

    std::fread(&R, 1, 1, fp);
    // ASSERT_NE(R, 0xff);

    std::vector<uint8_t> buf((R * R * R + 8 - 1) / 8, 0);
    std::fread(buf.data(), 1, buf.size(), fp);
    std::fclose(fp);

    buf.resize(R * R * R, Voxel::Void);
    for (int z = 0; z < R; ++z) {
        for (int y = 0; y < R; ++y) {
            for (int x = 0; x < R; ++x) {
                const size_t bit = (x * R + y) * R + z;
                (*this)(x, y, z) = (buf[bit >> 3] & (1 << (bit & 7))) ? Full : Void;
            }
        }
    }
}
