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
    // In a testbench, you will use you kernel as a C function
    // You will need to create the input and output of your function
    hls::stream<float> s;
    float size = 32;
    float *input = new float[32];
    for (unsigned int i = 0; i < size; i++) {
        input[i] = i;
    }
    setup_aie(size, input, s);

    // If the function worked I can print values in the stream and check them
    for(unsigned int i = 0; i < size +1 ; i++) {
        std::cout << s.read() << std::endl;
    }
    // Here you will se a warning: THIS IS THE MOST IMPORTANT PART OF THE TESTBENCH

    // Indeed, in testbench you can check if you stream and loop are correctly sized. 
    // Warning may be caused by your testbench, as now, or by wrongly sized loops in your code.

    // try to change the for-loop size, from "size" to "size+1" and see that the output is correct...
    // why we have to add +1?

    // And now? Since you want to effectively test your AIE...this code may practically write the AIE input
    // write into data 
    
    std::ofstream file;
    file.open("../../aie/data/in_plio_source_1.txt");
    if (file.is_open()) {
        //firstly I write the size, which will be, according to setup_aie, size/4
        file << size/4 << std::endl;
        for (unsigned int i = 0; i < size; i++) {
            file << input[i] << std::endl;
        }
        file.close();
    } else {
        std::cout << "Error opening file" << std::endl;
    }

    // In a different, complete, test, here you may even run the AIE and then continue your test. But for this
    // modular test...it's enough to check the stream and the file :=).
    return 0;
}
