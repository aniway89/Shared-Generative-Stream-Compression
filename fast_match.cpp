// fast_match.cpp
#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <string>
#include <algorithm>
#include <cstdint>

using namespace std;

const int KEY_SIZE = 4;
const int MAX_POS_PER_KEY = 8;       // keep last 8 occurrences
const int MAX_MATCH_LEN = 4096;

vector<uint8_t> read_file(const string& path) {
    ifstream f(path, ios::binary | ios::ate);
    if (!f) throw runtime_error("Cannot open " + path);
    size_t size = f.tellg();
    f.seekg(0);
    vector<uint8_t> data(size);
    f.read(reinterpret_cast<char*>(data.data()), size);
    return data;
}

void write_locations(const string& path, const vector<pair<uint32_t, uint32_t>>& matches) {
    ofstream f(path);
    for (auto& [off, len] : matches)
        f << off << " " << len << "\n";
}

int main() {
    cout << "Loading library..." << endl;
    auto lib = read_file("universal_lib.bin");
    cout << "Loading target (must be .bin from script1)..." << endl;
    auto target = read_file("search_image.bin");
    cout << "Library size: " << lib.size() << " bytes" << endl;
    cout << "Target size: " << target.size() << " bytes" << endl;

    // Build bounded index
    unordered_map<string, vector<uint32_t>> index;
    cout << "Building index..." << endl;
    for (uint32_t pos = 0; pos + KEY_SIZE <= lib.size(); ++pos) {
        string key((char*)&lib[pos], KEY_SIZE);
        auto& lst = index[key];
        if (lst.size() >= MAX_POS_PER_KEY)
            lst.erase(lst.begin());
        lst.push_back(pos);
        if (pos % 50'000'000 == 0)
            cout << "  Indexed " << pos/1e6 << " MB" << endl;
    }
    cout << "Index built, unique keys: " << index.size() << endl;

    // Greedy match
    vector<pair<uint32_t, uint32_t>> matches;
    uint32_t p = 0;
    const uint32_t tlen = target.size();

    while (p < tlen) {
        uint32_t best_off = 0;
        uint32_t best_len = 1;
        if (p + KEY_SIZE <= tlen) {
            string key((char*)&target[p], KEY_SIZE);
            auto it = index.find(key);
            if (it != index.end()) {
                for (uint32_t off : it->second) {
                    uint32_t max_len = min({tlen - p, (uint32_t)(lib.size() - off), (uint32_t)MAX_MATCH_LEN});
                    uint32_t len = 0;
                    while (len < max_len && target[p+len] == lib[off+len]) ++len;
                    if (len > best_len) {
                        best_len = len;
                        best_off = off;
                        if (best_len == MAX_MATCH_LEN) break;
                    }
                }
            }
        }
        // fallback: any single byte
        if (best_len == 1) {
            uint8_t byte = target[p];
            for (uint32_t i = 0; i < lib.size(); ++i)
                if (lib[i] == byte) { best_off = i; break; }
        }
        matches.emplace_back(best_off, best_len);
        p += best_len;
        if (p % 100000 == 0)
            cout << "Progress: " << p << "/" << tlen << " bytes" << endl;
    }

    write_locations("comImgLoc.txt", matches);
    cout << "Saved " << matches.size() << " entries to comImgLoc.txt" << endl;
    return 0;
}