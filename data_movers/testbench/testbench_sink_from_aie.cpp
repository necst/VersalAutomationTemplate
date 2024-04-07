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
#include "../sink_from_aie.cpp"
#include <cmath>

void write_into_stream(uint8_t *buffer, hls::stream<INPUT_DATA_TYPE> &stream, size_t size) {
    for (unsigned int i = 0; i < size; i++) {
        stream.write(buffer[i]);
    }
}

void write_into_stream(float *buffer, hls::stream<ap_axis<32, 0, 0, 0>> &stream, size_t size) {
    for (unsigned int i = 0; i < size; i++) {
        ap_axis<32, 0, 0, 0> x;
        x.data = buffer[i];
        // x.keep_all();
        stream.write(x);
    }
}

void write_into_stream(int32_t *buffer, hls::stream<ap_axis<32, 0, 0, 0>> &stream, size_t size) {
    for (unsigned int i = 0; i < size; i++) {
        ap_axis<32, 0, 0, 0> x;
        x.data = buffer[i];
        // x.keep_all();
        stream.write(x);
    }
}

template<typename T>
void read_from_stream(T *buffer, hls::stream<T> &stream, size_t size) {
    for (unsigned int i = 0; i < size; i++) {
        buffer[i] = stream.read();
    }
}


int main(int argc, char *argv[]) { 
    // This testbech will test the sink_from_aie kernel
    // The kernel will receive a stream of data from the AIE
    // and will write it into memory

    // I will create a stream of data
    hls::stream<float> s;
    float size = 16;
    // I create the buffer to write into memory
    float *buffer = new float[16];

    // I have to read the output of AI Engine from the file. Otherwise, I have no input for my testbench
    //TODO: AGGIUNGERE LOGICA PER LEGGERE output AIE

    sink_from_aie(s,buffer,size);
    // if the kernel is correct, it will contains the expected data. I can print them, for example, to check
    for (unsigned int i = 0; i < size; i++) {
        std::cout << buffer[i] << std::endl;
    }
    delete[] buffer;
}