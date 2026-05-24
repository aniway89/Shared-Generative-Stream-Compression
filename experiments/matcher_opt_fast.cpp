// matcher_opt_fast.cpp
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>   // ← for fast O(1) lookup

using namespace std;

const int KEY_SIZE = 5;
const int MAX_CANDIDATES = 8;

vector<uint8_t> read_file(const string& path) {
    ifstream f(path, ios::binary | ios::ate);
    if (!f) throw runtime_error("Cannot open " + path);
    size_t sz = f.tellg();
    f.seekg(0);
    vector<uint8_t> data(sz);
    f.read((char*)data.data(), sz);
    return data;
}

// Build unordered_set of all 5-grams from training data
unordered_set<uint64_t> build_5gram_set(const vector<uint8_t>& train) {
    unordered_set<uint64_t> gram_set;
    gram_set.reserve(train.size());           // pre‑allocate for speed
    for (size_t i = 0; i + 4 < train.size(); ++i) {
        uint64_t g = 0;
        for (int j = 0; j < 5; ++j) g = (g << 8) | train[i+j];
        gram_set.insert(g);
    }
    cout << "Unique 5‑grams: " << gram_set.size() << endl;
    return gram_set;
}

int main() {
    // 1. Load training data (concatenated PTIbin)
    vector<uint8_t> train = read_file("PTIbin_concat.bin");
    auto gram_set = build_5gram_set(train);   // now an unordered_set

    // 2. Load library and target
    vector<uint8_t> lib = read_file("universal_lib.bin");
    vector<uint8_t> target = read_file("search_image.bin");
    cout << "Library: " << lib.size() << " bytes, Target: " << target.size() << endl;

    // 3. Build hash index from library (skip keys not in gram_set)
    unordered_map<uint64_t, vector<uint32_t>> index;
    const uint32_t MAX_POS = lib.size() - KEY_SIZE + 1;
    for (uint32_t pos = 0; pos < MAX_POS; ++pos) {
        uint64_t key = 0;
        for (int j = 0; j < KEY_SIZE; ++j) key = (key << 8) | lib[pos+j];
        // O(1) existence check using unordered_set
        if (gram_set.find(key) == gram_set.end()) continue;
        auto& lst = index[key];
        if (lst.size() >= MAX_CANDIDATES) lst.erase(lst.begin());
        lst.push_back(pos);
        if (pos % 50'000'000 == 0) cout << "Indexed " << pos/1e6 << " MB\n";
    }
    cout << "Index built, keys: " << index.size() << endl;

    // 4. Greedy matching (unchanged)
    vector<pair<uint32_t,uint32_t>> matches;
    uint32_t p = 0;
    const uint32_t tlen = target.size();
    while (p < tlen) {
        uint32_t best_off = 0, best_len = 1;
        if (p + KEY_SIZE <= tlen) {
            uint64_t key = 0;
            for (int j = 0; j < KEY_SIZE; ++j) key = (key << 8) | target[p+j];
            auto it = index.find(key);
            if (it != index.end()) {
                for (uint32_t off : it->second) {
                    uint32_t maxlen = min({tlen - p, (uint32_t)(lib.size() - off), (uint32_t)4096});
                    uint32_t len = 0;
                    while (len < maxlen && target[p+len] == lib[off+len]) ++len;
                    if (len > best_len) {
                        best_len = len;
                        best_off = off;
                    }
                }
            }
        }
        if (best_len == 1) {   // fallback: single byte
            for (uint32_t i = 0; i < lib.size(); ++i)
                if (lib[i] == target[p]) { best_off = i; break; }
        }
        matches.emplace_back(best_off, best_len);
        p += best_len;
        if (p % 100000 == 0) cout << "Progress: " << p << "/" << tlen << "\n";
    }
    ofstream out("comImgLoc.txt");
    for (auto [off, len] : matches) out << off << " " << len << "\n";
    cout << "Saved " << matches.size() << " entries\n";
    return 0;
}

