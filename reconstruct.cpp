// ============================================================
// FILE: reconstruct.cpp
// PURPOSE:
// Reconstruct image from stream + coordinate map
// ============================================================

#include <iostream>
#include <fstream>
#include <vector>
#include <opencv2/opencv.hpp>

constexpr int TILE_SIZE = 4;

struct MatchRect {
    uint32_t stream_offset;
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
};

struct Tile {
    uint8_t data[TILE_SIZE * TILE_SIZE * 3];
};

std::vector<Tile> stream_tiles;

void load_stream() {

    std::ifstream in("STREAM/tiles.bin", std::ios::binary);

    while (true) {

        Tile t;

        in.read((char*)t.data, sizeof(t.data));

        if (!in)
            break;

        stream_tiles.push_back(t);
    }

    in.close();

    std::cout << "Loaded Tiles: " << stream_tiles.size() << "\n";
}

int main() {

    load_stream();

    std::ifstream in("MATCH/image.map", std::ios::binary);

    uint32_t count;

    in.read((char*)&count, sizeof(count));

    std::vector<MatchRect> matches(count);

    in.read((char*)matches.data(), count * sizeof(MatchRect));

    in.close();

    int width = 1024;
    int height = 1024;

    cv::Mat output(height, width, CV_8UC3);

    for (const auto& m : matches) {

        Tile& t = stream_tiles[m.stream_offset];

        int idx = 0;

        for (int ty = 0; ty < TILE_SIZE; ty++) {
            for (int tx = 0; tx < TILE_SIZE; tx++) {

                cv::Vec3b& p =
                    output.at<cv::Vec3b>(m.y + ty, m.x + tx);

                p[0] = t.data[idx++];
                p[1] = t.data[idx++];
                p[2] = t.data[idx++];
            }
        }
    }

    cv::imwrite("OUT/reconstructed.png", output);

    std::cout << "Reconstruction Complete\n";

    return 0;
}