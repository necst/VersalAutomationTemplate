/*
MIT License

Copyright (c) 2023 Paolo Salvatore Galfano, Giuseppe Sorrentino

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "image_utils.hpp"

int transform_coords(const int SIZE, const int LAYERS, const int TX,const int TY,const float ANG, const int i, const int j, const int k);

void transform_volume(
    uint8_t *volume_src,
    uint8_t *volume_dest,
    const int TX,
    const int TY,
    const float ANG,
    const int SIZE,
    const int LAYERS
) {
    // note: iterating over (j -> i) -> k is how we do it in hardware
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            for (int k = 0; k < LAYERS; k++) {
                // compute source index (transform [i,j] coordinates)
                int old_index = transform_coords(SIZE, LAYERS, TX, TY, ANG, i, j, k);

                // read pixel from input volume
                #ifndef TRACK_READS
                uint8_t pixel = (old_index != -1 ? volume_src[old_index] : 0);
                #else
                uint8_t pixel = (old_index != -1 ? track_reads(volume_src, old_index) : 0); // count sequential reads for stas
                #endif

                // compute dest index
                #ifndef USE_OLD_FORMAT
                const int dest_index = j*SIZE*LAYERS + i*LAYERS + k; // new formula
                #else
                const int dest_index = k*SIZE*SIZE + j*SIZE + i; // old formula
                #endif

                // write pixel in output volume
                volume_dest[dest_index] = pixel;

                // volume_dest[k*SIZE*SIZE + j*SIZE + i] = (old_index != -1 ? track_reads(volume_src, old_index) : 0); // old formula
            }
        }
    }

    // note: iterating over k -> (j -> i) is best for CPU
    // for (int k = 0; k < LAYERS; k++) {
    //     for (int j = 0; j < SIZE; j++) {
    //         for (int i = 0; i < SIZE; i++) {
    //             int old_index = transform_coords(SIZE, LAYERS, TX, TY, ANG, i, j, k);
    //             // volume_dest[k*SIZE*SIZE + j*SIZE + i] = (old_index != -1 ? volume_src[old_index] : 0);
    //             volume_dest[k*SIZE*SIZE + j*SIZE + i] = (old_index != -1 ? track_reads(volume_src, old_index) : 0);
    //         }
    //     }
    // }
}

inline 
#ifndef USE_FLOAT_INDEX
int 
#else
float
#endif
compute_buffer_offset(
    const int SIZE, const int LAYERS, 
    #ifndef USE_FLOAT_INDEX
    const int i, const int j, const int k
    #else
    const float i, const float j, const float k
    #endif
) {
    #ifndef USE_OLD_FORMAT
        return j * SIZE * LAYERS + i * LAYERS + k; // new formula
    #else
        return k * SIZE*SIZE + j * SIZE + i; // old formula
    #endif
}

// allows translation along x-y plane, and rotation around z-axis
int transform_coords(const int SIZE, const int LAYERS, const int TX,const int TY,const float ANG, const int i, const int j, const int k)
{   
    #ifndef USE_FLOAT_INDEX
    #ifndef USE_FLOOR
    const int new_i = std::round((i-SIZE/2.f)*std::cos(ANG) - (j-SIZE/2.f)*std::sin(ANG) - (float)TX + (SIZE/2.f));
    const int new_j = std::round((i-SIZE/2.f)*std::sin(ANG) + (j-SIZE/2.f)*std::cos(ANG) - (float)TY + (SIZE/2.f));
    #else
    const int new_i = std::floor((i-SIZE/2.f)*std::cos(ANG) - (j-SIZE/2.f)*std::sin(ANG) - (float)TX + (SIZE/2.f));
    const int new_j = std::floor((i-SIZE/2.f)*std::sin(ANG) + (j-SIZE/2.f)*std::cos(ANG) - (float)TY + (SIZE/2.f));
    #endif
    const int new_k = k;
    #else
    const float new_i = (i-SIZE/2.f)*std::cos(ANG) - (j-SIZE/2.f)*std::sin(ANG) - (float)TX + (SIZE/2.f);
    const float new_j = (i-SIZE/2.f)*std::sin(ANG) + (j-SIZE/2.f)*std::cos(ANG) - (float)TY + (SIZE/2.f);
    const float new_k = k;
    #endif

    // identify out-of-bound coordinates
    int out_index;
    if (new_i < 0 || new_i >= SIZE ||
        new_j < 0 || new_j >= SIZE ||
        new_k < 0 || new_k >= LAYERS)
        out_index = -1;
    else {
        #ifndef USE_FLOAT_INDEX
        out_index = compute_buffer_offset(SIZE, LAYERS, new_i, new_j, new_k);
        #else
        #ifndef USE_FLOOR
        out_index = std::round(compute_buffer_offset(SIZE, LAYERS, new_i, new_j, new_k));
        #else
        out_index = std::floor(compute_buffer_offset(SIZE, LAYERS, new_i, new_j, new_k));
        #endif
        #endif
    }
    
    return out_index;
}

void write_slice_in_buffer(uint8_t *src, uint8_t *dest, const int slice_index, const int SIZE, const int LAYERS) {
    for (int i = 0; i < SIZE*SIZE; i++) {
        #ifndef USE_OLD_FORMAT
        const int dest_index = i * LAYERS + slice_index; // new formula
        #else
        const int dest_index = slice_index * SIZE * SIZE + i; // old formula
        #endif
        dest[dest_index] = src[i];
    }
}

void read_slice_from_buffer(uint8_t *src, uint8_t *dest, const int slice_index, const int SIZE, const int LAYERS) {
    for (int i = 0; i < SIZE*SIZE; i++) {
        #ifndef USE_OLD_FORMAT
        const int src_index = i * LAYERS + slice_index; // new formula
        #else
        const int src_index = slice_index * SIZE * SIZE + i; // old formula
        #endif
        dest[i] = src[src_index];
    }
}

/// Round up to next higher power of 2 (return x if it's already a power of 2).
inline unsigned int pow2roundup(unsigned int x)
{
    if (x < 0)
        return 0;
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return x+1;
}

inline uint8_t convertDepth8(uint32_t pixel, const int originalDepth) {
    const uint32_t maxPixelValue = ((int32_t)1 << originalDepth) - 1;
    const float normalized = pixel / (float)maxPixelValue;
    return std::round(255.0f * normalized);
}

int read_volume_from_file_DICOM(uint8_t *volume, const int SIZE, const int N_COUPLES, const std::string &path) {
    #ifndef COMPILE_WITHOUT_DCMTK
    uint8_t* imgData = new uint8_t[SIZE * SIZE];

    for (int i = 0; i < N_COUPLES; i++) {
        std::string s = path + "IM" + std::to_string(i+1) + ".dcm";
        DcmFileFormat fileformat;
        OFCondition status = fileformat.loadFile(s.c_str());

        if (!status.good()) {
            std::cerr << "Error: cannot load DICOM file (" << status.text() << ")" << std::endl;
            return -1;
        }
        
        DicomImage* dcmImage = new DicomImage(&fileformat, EXS_Unknown);
        if (dcmImage == nullptr || !dcmImage->isMonochrome()) {
            std::cerr << "Error: cannot read DICOM image (" << status.text() << ")" << std::endl;
            return -1;
        }

        const int bitsPerPixel = dcmImage->getDepth();
        const unsigned long numPixels = dcmImage->getWidth() * dcmImage->getHeight();
        if (numPixels != SIZE*SIZE) {
            std::cerr << "Error: size of image (" << dcmImage->getWidth() << "*" << dcmImage->getHeight() << ") different from required size (" << SIZE << "*" << SIZE << ")" << std::endl;
            return -1;
        }

        const size_t bufferSize = numPixels * (pow2roundup(bitsPerPixel) / 8);

        if (imgData == nullptr) {
            std::cerr << "Error: cannot allocate " << bufferSize << " bytes" << std::endl;
            perror("Error");
            return -1;
        }

        if (dcmImage->getOutputData(imgData, bufferSize, 8, 0) == false) {
            std::cerr << "Error: cannot read pixels (" << status.text() << ")" << std::endl;
            return -1;
        }
        
        write_slice_in_buffer((uint8_t*)imgData, volume, i, SIZE, N_COUPLES);
    }
    delete[] imgData;
    return 0;
    
    #else
    std::cerr << "Error: DICOM support is disabled" << std::endl;
    return -1;
    #endif
}

int read_volume_from_file_PNG(uint8_t *volume, const int SIZE, const int N_COUPLES, const int PADDING, const std::string &path) {
    for (int i = 0; i < N_COUPLES; i++) {
        std::string s = path + "IM" + std::to_string(i+1) + ".png";
        cv::Mat image = cv::imread(s, cv::IMREAD_GRAYSCALE);
        if (!image.data) return -1;

        // copy the slice into the buffer
        std::vector<uint8_t> tmp(SIZE*SIZE);
        tmp.assign(image.begin<uint8_t>(), image.end<uint8_t>());
        write_slice_in_buffer(tmp.data(), volume, i, SIZE, N_COUPLES+PADDING);
    }

    for (int i = 0; i < PADDING; i++) {
        // copy the slice into the buffer
        std::vector<uint8_t> tmp(SIZE*SIZE);
        tmp.assign(tmp.size(), 0);
        write_slice_in_buffer(tmp.data(), volume, N_COUPLES+i, SIZE, N_COUPLES+PADDING);
    }

    return 0;
}


void write_volume_to_file(uint8_t *volume, const int SIZE, const int N_COUPLES, const int PADDING, const std::string &path) {
    for (int i = 0; i < N_COUPLES; i++) {
        std::vector<uint8_t> tmp(SIZE*SIZE);
        read_slice_from_buffer(volume, tmp.data(), i, SIZE, N_COUPLES+PADDING);
        cv::Mat slice = (cv::Mat(SIZE, SIZE, CV_8U, tmp.data())).clone();
        std::string s = path + "IM" + std::to_string(i+1) + ".png";
        cv::imwrite(s, slice);
    }
}

int read_volume_from_file(uint8_t *volume, const int SIZE, const int N_COUPLES, const int PADDING, const std::string &path, const ImageFormat imageFormat) {
    switch (imageFormat) {
        case ImageFormat::PNG:
            return read_volume_from_file_PNG(volume, SIZE, N_COUPLES, PADDING, path);
        case ImageFormat::DICOM:
            return read_volume_from_file_DICOM(volume, SIZE, N_COUPLES, path);
    }
    return -1;
}



uint8_t track_reads(uint8_t *mem, const int index, float *ratio) {
    static unsigned int sequential_count = 0;
    static unsigned int total_count = 0;
    static unsigned int last_index = -2;

    if (ratio == NULL) {
        ++total_count;
        if (index == last_index + 1) {
            ++sequential_count;
        }
        last_index = index;

        #ifdef DEBUG_ACCESSED_INDEXES
        std::cout << index << " ";
        #endif

        return mem[index];
    }

    if (total_count != 0)
        *ratio = 100.0f * sequential_count / (float)total_count;
    else
        *ratio = 0.0f;
    
    return -1;
}
