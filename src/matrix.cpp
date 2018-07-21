#include "matrix.h"

#include <string>
#include <iostream>

Matrix::Matrix() : R(0), buf() {
}
Matrix::Matrix(int R_) : R(R_), buf(R * R * R, Void) {
}
Matrix::Matrix(int R_, Voxel filled_value) : R(R_), buf(R * R * R, filled_value) {
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

bool Matrix::dump(std::string output_path) {
    // assert a valid m.
    std::vector<uint8_t> buffer((R * R * R + 8 - 1) / 8, 0);
    for (int z = 0; z < R; ++z) {
        for (int y = 0; y < R; ++y) {
            for (int x = 0; x < R; ++x) {
                const size_t bit = (x * R + y) * R + z;
                if ((*this)(x, y, z) != Void) {
                    buffer[bit >> 3] |= (1 << (bit & 7));
                }
            }
        }
    }

    std::FILE* fp = std::fopen(output_path.c_str(), "wb");
    std::fwrite(&R, 1, 1, fp);
    std::fwrite(buffer.data(), 1, buffer.size(), fp);
    std::fclose(fp);

    return true;
}
