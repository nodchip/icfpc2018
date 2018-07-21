#include "matrix.h"

#include <string>
#include <iostream>

Matrix::Matrix(int R_) : R(R_), buf(R * R * R, Void) {
}

Matrix::Matrix(const std::string& input_path) : R(0xff) {
    std::FILE* fp = std::fopen(input_path.c_str(), "rb");
    if (!fp)
      std::cerr << "fail to load " << input_path << "\n";

    std::fread(&R, 1, 1, fp);
    // ASSERT_NE(0xff, R);

    std::vector<uint8_t> binary_buffer((R * R * R + 8 - 1) / 8, 0);
    std::fread(binary_buffer.data(), 1, binary_buffer.size(), fp);
    std::fclose(fp);

    buf.resize(R * R * R, Voxel::Void);
    for (int z = 0; z < R; ++z) {
        for (int y = 0; y < R; ++y) {
            for (int x = 0; x < R; ++x) {
                const size_t bit = (x * R + y) * R + z;
                (*this)(x, y, z) = (binary_buffer[bit >> 3] & (1 << (bit & 7))) ?
                  Voxel::Full : Voxel::Void;
            }
        }
    }
}
