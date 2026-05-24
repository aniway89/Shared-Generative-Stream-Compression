# Reference-Based Shared Stream Compression for Domain-Specific Image Reconstruction

**Author:** Mohd Ayan Khan  
**Role:** Independent Researcher  
**Email:** [yoru.ayan@gmail.com](mailto:yoru.ayan@gmail.com)

---

## Abstract

This repository presents an experimental **Reference-Based Shared Stream Compression (SGSC)** framework. It shifts the traditional paradigm of image compression away from localized mathematical approximations (e.g., discrete cosine transforms) toward a retrieval-based spatial search problem over a massive, pre-shared visual memory. 

By encoding images strictly as a sequence of stream offsets, segment lengths, and reconstruction coordinates, the framework achieves unprecedented compression ratios (up to **3.1M+ x**) on domain-specific datasets when high structural similarity is present. While theoretical transmission efficiency is extremely high, this study highlights the severe computational bottlenecks in search traversal complexity under constrained hardware, positioning this system as a prototype for exploratory systems research rather than a production replacement for standard codecs like JPEG, WebP, or AVIF.

---

## 🏗️ System Architecture

The core operational pipeline of the Shared Generative Stream Compression (SGSC) framework is divided into four highly dependent stages, built on a deterministic layout shared between the encoder (sender) and decoder (receiver).

```mermaid
flowchart TD
    subgraph Stage 1: Stream Generation (generate_stream)
        A[Training Images IMG/] --> B[BGR Row-Major Processing]
        B --> C{Generative Mode?}
        C -- Yes --> D[Append +/- Delta Neighbors]
        C -- No --> E[Direct Append]
        D --> F[Inject Separators 0xFF x 16]
        E --> F
        F --> G[Append Universal Palette Suffix 48MB]
        G --> H[Write STREAM/stream.bin]
    end

    subgraph Stage 2 & 3: Matching & Encoding (match_image)
        I[Target Image COMP_IMG/] --> J[Rabin-Karp Rolling Hash]
        H -. Loaded in Memory .-> K{Two-Zone Search Engine}
        J --> K
        K -- Zone 1: Training Data --> L[Rabin-Karp Segment Search]
        L --> M[Extend Match Fwd/Bwd]
        K -- Zone 2: Palette Suffix --> N[O1 Arithmetic BGR Lookup]
        M --> O[Combine & Segment Allocation]
        N --> O
        O --> P[Generate MATCH/image.bin Map file]
    end

    subgraph Stage 4: Decoding & Rebuilding (reconstruct)
        H -. Loaded in Memory .-> Q[OpenCV Mat Buffer Allocation]
        P --> R[Parse Header & Segment Array]
        R --> S[Direct Memory Contiguous Write memcpy]
        Q --> S
        S --> T[Verify: Pixel-Perfect Check]
        T --> U[Write OUT/image.png]
    end
    
    style H fill:#f9f,stroke:#333,stroke-width:2px
    style P fill:#bbf,stroke:#333,stroke-width:2px
    style U fill:#bfb,stroke:#333,stroke-width:2px
```

---

## ⚙️ Core Methodologies & System Internals

The C++ implementation operates in a strictly deterministic pipeline, utilizing modern systems engineering optimization techniques:

### 1. Two-Zone Stream Design
To achieve robust matching while keeping memory footprints reasonable, the pre-shared visual memory stream (`stream.bin`) is structured into two distinct operational zones:
*   **Zone 1 — Training Data (Indexed):** Compiled from curated domain images. It is indexed via a high-performance hash map using **Rabin-Karp rolling hashes** over a rolling sliding window (`INDEX_CHUNK = 16`).
*   **Zone 2 — Universal Palette Suffix (Non-Indexed):** To prevent Out-Of-Memory (OOM) failures from hashing millions of palette entries, the stream appends all $16,777,216$ possible BGR color triplets (adding exactly $48 \text{ MB}$). Since the palette follows a rigid mathematical order, the offset of any unmatched BGR pixel is computed using $O(1)$ arithmetic:
    $$\text{offset} = \text{palette\_start} + (B \times 65536 + G \times 256 + R) \times 3$$

### 2. Generative Data Streaming
During Stream Generation, enabling `ENABLE_GENERATIVE_MODE` instructs the compiler to append not only the original BGR pixel triplets but also nearby color variants (modified by a customizable $\pm\text{delta}$ offset). This expands the stream's coverage over fine color gradients, allowing matching segments to grow longer through compression noise and lighting discrepancies.

### 3. Rabin-Karp Segment Matching Engine
When a target image is processed, the system searches for duplicate byte runs:
1.  It attempts an exact **Full Match Search** using Rabin-Karp to see if the entire image already exists in the stream (reducing the image to a single $24$ to $28\text{-Byte}$ segment).
2.  If no full match is found, it runs a **Segmented Match Search**, matching $16\text{-byte}$ chunks and growing them dynamically using `extend_match` in both forward and backward directions.
3.  Any remaining unmatched gaps undergo **Palette Fallback**, querying the $O(1)$ Universal Palette and inserting 3-byte pixel matches to ensure **100% reconstruction coverage** with no black areas.

---

## 📈 Empirical Benchmarks & Performance Analysis

### 🖥️ Experimental Setup & Hardware Constraints
The testbench utilized for benchmarking consisted of a resource-constrained system, exposing critical boundaries for computational traversals:
*   **CPU:** AMD Ryzen 3 3250U CPU (2 Cores, 4 Logical Processors) operating at a base speed of $2.60 \text{ GHz}$ (peaking at $2.74 \text{ GHz}$).
*   **Cache:** $194 \text{ KB}$ L1, $1 \text{ MB}$ L2, and $4 \text{ MB}$ L3.
*   **RAM:** $12 \text{ GB}$ DDR4 (with only $\sim 5.6 \text{ GB}$ available for experimental execution due to heavy OS overhead).
*   **Graphics:** Integrated AMD Radeon™ Graphics ($2 \text{ GB}$ dedicated, $4.9 \text{ GB}$ shared).
*   **Thermal Bottleneck:** The CPU idled at an exceptionally high **$93^\circ\text{C}$** with negligible (4%) base utilization before running benchmarks, leading to extreme thermal throttling and shutdowns during exhaustive sequential searches.

### 📊 Benchmark Metrics
The following results were recorded directly from the experimental evaluations using the C++ pipeline:

| Benchmark Configuration | Original Size | Compressed Coordinate Map | Compression Ratio | Matching Time | Reconstruction Time | Verification Status |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: |
| **Mars Map** (`Big mars map 2.png`) | $82.95 \text{ MB}$ | **$28 \text{ Bytes}$** | **$3,106,258.11 \times$** | $2.18 \text{ sec}$ | N/A | **PIXEL PERFECT** |
| **Exact Stream Match** (`PTI2.png`) | $3.44 \text{ MB}$ | **$24 \text{ Bytes}$** | **$150,180.25 \times$** | $0.08 \text{ sec}$ | $0.22 \text{ sec}$ | **PIXEL PERFECT** |
| **Segment-Based Match** (`PTI1.jpg`) | $247.30 \text{ KB}$ | **$28 \text{ Bytes}$** | **$9,044.18 \times$** | $0.01 \text{ sec}$ | N/A | **PIXEL PERFECT** |
| **Unseen Domain Match** (`PTI1 cp.jpg`) | $15.57 \text{ KB}$ | $5.21 \text{ KB}$ | **$2.98 \times$** | **$527.96 \text{ sec}$** | $0.001 \text{ sec}$ | **PARTIAL MATCH** |

> [!TIP]
> **Key Insight:** Stream quality and structural domain similarity matter far more than raw stream size. High-similarity target data matches instantly, whereas unmatched "unseen" domains trigger massive segmented chunk queries and fallback loops, generating $333$ separate segments and taking nearly **$9$ minutes** to match.

---

## 🛠️ Repository File Structure

The repository is organized to maintain a clean, professional root directory while archiving all exploratory scripts and historical drafts in the `experiments` folder:

```
├── experiments/                 # Archived prototypes, visualizations, and datasets
│   ├── Dv1/                     # V1-V3 C++ codebase drafts
│   ├── outputs/                 # CSV and Markdown benchmark outputs
│   ├── graph.py                 # Matplotlib visualization dashboard script
│   ├── compress_tile.py         # Historical tiling compression models
│   └── ...                      # Python compression prototypes
│
├── IMG/                         # [User Input] Source images used to train the stream
│   └── PTI1.jpg
├── COMP_IMG/                    # [User Input] Target images to be compressed
│   └── PTI1.jpg
├── STREAM/                      # [Auto-generated] Holds output pre-shared stream.bin
├── MATCH/                       # [Auto-generated] Holds output compressed coordinate binaries
├── OUT/                         # [Auto-generated] Holds reconstructed output PNG images
│
├── config.h                     # Global constants, deltas, and Rabin-Karp parameters
├── logger.h                     # Timer, file-size formats, and logging utilities
├── generate_stream.cpp          # Stage 1: Stream builder
├── match_image.cpp              # Stage 2-3: Rabin-Karp rolling encoder
├── reconstruct.cpp              # Stage 4: Decompressor & pixel-perfect verifier
│
├── build.sh                     # GCC compilation utility
├── runpipeline.sh               # Executes stream generation, matching, and reconstruction
├── benchmark.log                # Live system execution logs
└── README.md                    # This document
```

---

## 🚀 Getting Started & Execution

### Prerequisites
*   A C++ compiler supporting `C++17` or higher (`g++` recommended).
*   OpenCV 4 C++ runtime libraries (`opencv4`).

### Quick Start Pipeline

1.  **Clone the Repository:**
    ```bash
    git clone https://github.com/aniway89/Shared-Generative-Stream-Compression.git
    cd Shared-Generative-Stream-Compression
    ```

2.  **Add Your Datasets:**
    *   Place your pre-shared training images in `IMG/`.
    *   Place the target images you wish to compress in `COMP_IMG/`.

3.  **Compile the C++ Executables:**
    ```bash
    chmod +x build.sh
    ./build.sh
    ```

4.  **Execute the Reconstruction Pipeline:**
    ```bash
    chmod +x runpipeline.sh
    ./runpipeline.sh
    ```

5.  **Review Reconstructed Outputs:**
    *   The pre-shared dictionary will be saved to `STREAM/stream.bin`.
    *   The compressed coordinate files will be in `MATCH/`.
    *   The final decompressed images will be verified in `OUT/`.

---

## ⚠️ System Limitations & Failure Modes

*   **Search Complexity CPU Bottleneck:** Sequential CPU scanning of large streams (ranging from $5\text{ MB}$ to $700\text{ MB}$) is computationally demanding for large datasets, leading to severe thermal throttling on standard consumer CPUs.
*   **Vast Pre-Shared Sync Lock:** The receiver must own a byte-perfect copy of the `stream.bin` file used during encoding. If the pre-shared visual memory loses synchronization by even a single byte, decoding fails completely.
*   **Domain Dependency:** A stream trained on face datasets (`IMG/`) will fail to compress landscape or geographical images, dropping the compression ratio to baseline levels.

---

## 🔮 Future Research Directions

1.  **CUDA Parallel Traversal:** Porting the Rabin-Karp sequential matching loop into parallel GPU threads to scale stream traversals to gigabytes in real-time.
2.  **Approximate Nearest-Neighbor (ANN):** Utilizing high-dimensional vector search indices (such as HNSW or FAISS) over image embeddings instead of raw pixel byte matching.
3.  **Spatial 2D Patch Indexing:** Shifting from 1D sequential byte matches to 2D hierarchical rectangular patch indexing to better capture local spatial transformations.

---

## 📜 References

*   **[1]** R. M. Karp and M. O. Rabin, "Efficient randomized pattern-matching algorithms," *IBM J. Res. Develop.*, vol. 31, no. 2, pp. 249–260, Mar. 1987.
*   **[2]** M. Okade and J. Mukherjee, "Discrete Cosine Transform: A Revolutionary Transform That Transformed Human Lives," *IEEE Circuits Syst. Mag.*, vol. 22, no. 4, pp. 58–61, 2022.
*   **[3]** G. K. Wallace, "The JPEG still picture compression standard," *Commun. ACM*, vol. 34, no. 4, pp. 30–44, Apr. 1991.
