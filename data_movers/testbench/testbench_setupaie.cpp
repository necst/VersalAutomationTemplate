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

#include <unistd.h>
#include <sys/stat.h>
#include <fstream>
#include <ap_axi_sdata.h>
#include <cmath>
#include "../setup_aie.cpp"
#include <iostream>

void read_from_stream(float *buffer, hls::stream<float> &stream, size_t size) {
    for (unsigned int i = 0; i < size; i++) {
        buffer[i] = stream.read();
    }
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::cerr << "usage: ./testbench_setupaie <tx> <ty> <ang> <n_couples>" << std::endl;
        return -1;
    }

    const float TX = atof(argv[1]);
    const float TY = atof(argv[2]);
    const float ANG_DEG = atof(argv[3]);
    const float ANG = (ANG_DEG * M_PI) / 180.f; ; // radians
    const float n_couples = atof(argv[4]);

    // std::printf("input: tx=%f, ty=%f, ang=%f, n_couples=%f\n", TX, TY, ANG, n_couples);

    hls::stream<float> coords_in;

    setup_aie(TX, TY, ANG, n_couples, coords_in);

    float output[5];
    read_from_stream(output, coords_in, 4); // TODO, sono 5, non 4 (manca la scrittura di kernel_index in setup_aie)

    // std::printf("coords_in: tx=%f, ty=%f, ang=%f, n_couples=%f, k_index=%f\n", output[0], output[1], output[2], output[3], output[4]);
    std::printf("%f\n%f\n%f\n%f\n%f\n", output[0], output[1], output[2], output[3], output[4]);
}
