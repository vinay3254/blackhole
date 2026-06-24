#include "BlackHoleRenderer.h"
#include <iostream>
#include <vector>
#include <chrono>
#include <cstdio>

// Exports the RGB pixel buffer to a standard 24-bit RGB Bitmap file
bool SaveBMP(const char* filename, const unsigned char* rgb_buffer, int width, int height) {
    unsigned char header[54] = {
        'B', 'M', 0, 0, 0, 0, 0, 0, 0, 0, 54, 0, 0, 0, 40, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 24, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

    int row_size = (width * 3 + 3) & ~3; // Align to 4 bytes
    int pixel_data_size = row_size * height;
    int file_size = 54 + pixel_data_size;

    header[2] = (unsigned char)(file_size);
    header[3] = (unsigned char)(file_size >> 8);
    header[4] = (unsigned char)(file_size >> 16);
    header[5] = (unsigned char)(file_size >> 24);

    header[18] = (unsigned char)(width);
    header[19] = (unsigned char)(width >> 8);
    header[20] = (unsigned char)(width >> 16);
    header[21] = (unsigned char)(width >> 24);

    header[22] = (unsigned char)(height);
    header[23] = (unsigned char)(height >> 8);
    header[24] = (unsigned char)(height >> 16);
    header[25] = (unsigned char)(height >> 24);

    FILE* f = std::fopen(filename, "wb");
    if (!f) return false;

    std::fwrite(header, 1, 54, f);

    unsigned char* row = new unsigned char[row_size];
    for (int y = 0; y < height; ++y) {
        // Bitmap expects rows bottom-up, and BGR sequence
        int buffer_y = height - 1 - y;
        for (int x = 0; x < width; ++x) {
            int bi = (buffer_y * width + x) * 3;
            int ri = x * 3;
            row[ri]     = rgb_buffer[bi + 2]; // B
            row[ri + 1] = rgb_buffer[bi + 1]; // G
            row[ri + 2] = rgb_buffer[bi];     // R
        }
        for (int x = width * 3; x < row_size; ++x) {
            row[x] = 0;
        }
        std::fwrite(row, 1, row_size, f);
    }

    delete[] row;
    std::fclose(f);
    return true;
}

int main() {
    std::cout << "========================================================\n";
    std::cout << " C++ Kerr (Spinning) Black Hole Visual Renderer Runner\n";
    std::cout << "========================================================\n\n";

    int width = 1024;
    int height = 768;
    std::vector<unsigned char> rgb_buffer(width * height * 3);

    RenderParams params;
    params.M = 1.0;
    params.a = 0.9;          // High spin parameter (Kerr black hole)
    params.cam_distance = 15.0;
    params.cam_pitch = 0.15; // Slightly tilted down (Interstellar-style)
    params.cam_yaw = 0.4;    // Slightly rotated perspective
    params.enable_disk = true;
    params.disk_inner = 2.0; // ISCO moves inward for spin, so we can start inner disk closer (e.g. 2.0M)
    params.disk_outer = 12.0;
    params.disk_bright = 1.5;
    params.disk_temp = 1.4;
    params.enable_beaming = true;
    params.dt_base = 0.08;
    params.time = 2.0;

    std::cout << "Rendering " << width << "x" << height << " frame...\n";
    
    auto start = std::chrono::high_resolution_clock::now();
    BlackHoleRenderer::RenderFrame(rgb_buffer.data(), width, height, params);
    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> diff = end - start;
    std::cout << "Render completed in " << diff.count() << " seconds.\n";

    const char* filename = "blackhole.bmp";
    std::cout << "Saving image to " << filename << "...\n";
    if (SaveBMP(filename, rgb_buffer.data(), width, height)) {
        std::cout << "Success! Image saved successfully.\n";
    } else {
        std::cerr << "Error: Failed to save BMP image.\n";
        return 1;
    }

    std::cout << "\n========================================================\n";
    return 0;
}
