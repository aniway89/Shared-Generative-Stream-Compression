# Shared Generative Stream Compression

A project implementing generative stream compression techniques for efficient data processing and face recognition.

## Overview

This repository contains implementations for converting images to binary formats, stream generation, and fast matching algorithms for face recognition and image processing tasks.

## Project Structure

```
├── script1_convert_to_bin.py          # Convert images to binary format
├── script2_stream_generator.py        # Generate data streams
├── script3_fast_matcher.py            # Fast matching algorithm
├── script3_image_matcher.py           # Image matching algorithm
├── script4_reconstruct.py             # Reconstruct images from binary
├── convert_single_image.py            # Convert single image utility
├── convert_to_bin.py                  # Batch image conversion
├── make_bin.py                        # Binary file creation
├── fast_match.cpp                     # C++ fast matching implementation
├── matcher_opt.cpp                    # C++ optimized matcher
└── matcher_opt_fast.cpp               # C++ optimized fast matcher
```

## Scripts

### Python Scripts

- **script1_convert_to_bin.py** - Main conversion script to transform images into binary format
- **script2_stream_generator.py** - Generates data streams for processing
- **script3_fast_matcher.py** - Fast image matching using optimized algorithms
- **script3_image_matcher.py** - Standard image matching implementation
- **script4_reconstruct.py** - Reconstructs images from binary representations
- **convert_to_bin.py** - Batch conversion utility for multiple images
- **make_bin.py** - Creates binary file archives

### C++ Components

- **fast_match.cpp** - Fast matching algorithm in C++
- **matcher_opt.cpp** - Optimized matcher implementation
- **matcher_opt_fast.cpp** - Highly optimized fast matcher

## Requirements

- Python 3.7+
- C++ compiler (for compiling .cpp files)
- Dependencies specified in requirements.txt (if available)

## Usage

### Convert Images to Binary

```bash
python script1_convert_to_bin.py
```

### Generate Streams

```bash
python script2_stream_generator.py
```

### Fast Matching

```bash
python script3_fast_matcher.py
```

### Reconstruct Images

```bash
python script4_reconstruct.py
```

### Compile C++ Components

```bash
g++ -O2 -o fast_match fast_match.cpp
g++ -O2 -o matcher_opt matcher_opt.cpp
g++ -O2 -o matcher_opt_fast matcher_opt_fast.cpp
```

## License

MIT License

## Author

Aniway89

## Notes

- Large binary files and image datasets are not included in the repository
- Compiled executables (.exe) should be generated locally from source code
- For optimal performance, use the C++ implementations (matcher_opt_fast)
