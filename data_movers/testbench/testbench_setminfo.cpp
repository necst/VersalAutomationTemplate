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
#include "../../mutual_info/include/hw/mutualInfo/mutual_info.hpp"
#include "../../mutual_info/mutual_information_master.cpp"
#include "../../sw/include/image_utils/image_utils.cpp"
#include "../setup_mutualInfo.cpp"
#include <cmath>

// // #include <ap_axi_sdata.h>
// void setup_mutualInfo(
//     hls::stream<qdma_axis<32, 0, 0, 0>  >& coords_in, 
//     // hls::stream<qdma_axis<PIXEL_SIZE, 0, 0, 0>>& float_out,
//     ap_int<8>* float_original, 
//     ap_int<8>* float_transformed, 
//     int n_couples);
//


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
    if (argc != 5) {
        std::cerr << "usage: ./testbench_setminfo <tx> <ty> <ang> <n_couples>" << std::endl;
        return -1;
    }

    const float TX = atof(argv[1]);
    const float TY = atof(argv[2]);
    const float ANG_DEG = atof(argv[3]);
    const float ANG = (ANG_DEG * M_PI) / 180.f; ; // radians
    const int n_couples = static_cast<int>(atof(argv[4]));

    std::cout << "TESTBENCH PARAMETERS:\n";
    std::cout << "tx: " << TX << std::endl;
    std::cout << "ty: " << TY << std::endl;
    std::cout << "ang (deg): " << ANG_DEG << std::endl;
    std::cout << "dimension: " << DIMENSION << std::endl;
    std::cout << "n_couples: " << n_couples << std::endl;

//------------------------------------------------GENERATING VOLUMES------------------------------------------
    // int n_couples = 512;
    const int padding = (HIST_PE - (n_couples % HIST_PE)) % HIST_PE;

    printf("padding: %d\n", padding);
    printf("total: %d\n", n_couples + padding);

    uint8_t* input_ref = new uint8_t[DIMENSION*DIMENSION * (n_couples + padding)];
    uint8_t* input_flt = new uint8_t[DIMENSION*DIMENSION * (n_couples + padding)];
    uint8_t* output_flt = new uint8_t[DIMENSION*DIMENSION * (n_couples + padding)];
    float output_data;
    
    int32_t* params = new int32_t[DIMENSION*DIMENSION];
    
    std::cout << argv[0]; 
    if (read_volume_from_file(input_ref, DIMENSION, n_couples, padding, "../../sw/dataset/") == -1) {
        std::cout << "could not open image" << std::endl;
    }
    if (read_volume_from_file(input_flt, DIMENSION, n_couples, padding, "../../sw/dataset/") == -1) {
        std::cout << "could not open image" << std::endl;
    }

    write_volume_to_file(input_flt, DIMENSION, n_couples, padding, "dataset_output/"); // sd
    return 0;

    std::ifstream file;
    file.open("coords_in.txt");
    for (int i = 0; i < DIMENSION*DIMENSION; i++) {
        float value;
        file >> value;
        params[i] = value;
    }
    
    hls::stream<ap_axis<COORD_BITWIDTH, 0, 0, 0>> stream_coords_in;
    hls::stream<ap_uint<INPUT_DATA_BITWIDTH>> stream_float_out;

    write_into_stream(params, stream_coords_in, DIMENSION*DIMENSION);
    
    setup_mutualInfo(stream_coords_in, stream_float_out, (ap_uint<INPUT_DATA_BITWIDTH>*)input_flt, (ap_uint<INPUT_DATA_BITWIDTH>*)output_flt, n_couples);
    
    write_volume_to_file(output_flt, DIMENSION, n_couples, padding, "dataset_output/"); // sd

    // hls::stream<INPUT_DATA_TYPE> stream_input_img;
    // write_into_stream(output_flt, stream_input_img, DIMENSION*DIMENSION * n_couples);

    std::cout << "executing kernel" << std::endl;
    mutual_information_master(stream_float_out, (INPUT_DATA_TYPE*)input_ref, &output_data, n_couples);

// ----------------------------------------------------CALCOLO SOFTWARE DELLA MI-----------------------------------------
    std::cout << "computing software mi" << std::endl;

    double j_h[J_HISTO_ROWS][J_HISTO_COLS];
    for(int i=0;i<J_HISTO_ROWS;i++){
        for(int j=0;j<J_HISTO_COLS;j++){
            j_h[i][j]=0.0;
        }
    }

    uint8_t* transformed = new uint8_t[DIMENSION*DIMENSION*n_couples];
    // const float ANG = (15 * M_PI) / 180.f; ; // radians
    transform_volume(input_flt, transformed, TX, TY, ANG, DIMENSION, n_couples);
    write_volume_to_file(transformed, DIMENSION, n_couples, padding,  "dataset_output_sw/");

    for(int k = 0; k < n_couples; k++) {
        for(int i=0;i<DIMENSION;i++){
            for(int j=0;j<DIMENSION;j++){
                unsigned int a=input_ref[k * DIMENSION * DIMENSION + i * DIMENSION + j];
                unsigned int b=output_flt[k * DIMENSION * DIMENSION + i * DIMENSION + j];
                j_h[a][b]= (j_h[a][b])+1;
            }
        }
    }

    for (int i=0; i<J_HISTO_ROWS; i++) {
        for (int j=0; j<J_HISTO_COLS; j++) {
            j_h[i][j] = j_h[i][j]/(n_couples*DIMENSION*DIMENSION);
        }
    }


    float entropy = 0.0;
    for (int i=0; i<J_HISTO_ROWS; i++) {
        for (int j=0; j<J_HISTO_COLS; j++) {
            float v = j_h[j][i];
            if (v > 0.000000000000001) {
            entropy += v*log2(v);///log(2);
            }
        }
    }
    entropy *= -1;

    double href[ANOTHER_DIMENSION];
    for(int i=0;i<ANOTHER_DIMENSION;i++){
        href[i]=0.0;
    }

    for (int i=0; i<ANOTHER_DIMENSION; i++) {
        for (int j=0; j<ANOTHER_DIMENSION; j++) {
            href[i] += j_h[i][j];
        }
    }

    double hflt[ANOTHER_DIMENSION];
    for(int i=0;i<ANOTHER_DIMENSION;i++){
        hflt[i]=0.0;
    }

    for (int i=0; i<J_HISTO_ROWS; i++) {
        for (int j=0; j<J_HISTO_COLS; j++) {
            hflt[i] += j_h[j][i];
        }
    }


    double eref = 0.0;
    for (int i=0; i<ANOTHER_DIMENSION; i++) {
        if (href[i] > 0.000000000001) {
            eref += href[i] * log2(href[i]);///log(2);
        }
    }
    eref *= -1;


    double eflt = 0.0;
    for (int i=0; i<ANOTHER_DIMENSION; i++) {
        if (hflt[i] > 0.000000000001) {
            eflt += hflt[i] * log2(hflt[i]);///log(2);
        }
    }
    eflt =  eflt * (-1);

    double mutualinfo = eref + eflt - entropy;
    printf("Software MI %lf\n", mutualinfo);
    printf("Hardware MI %lf\n", output_data);
// ----------------------------------------------------CONFRONTO PER VERIFICARE L'ERRORE
    
    printf("error = %f", (mutualinfo - output_data));
    if(((data_t)mutualinfo - output_data > 0.01)){
    printf("\noh no!");
        return 1;
    }
    

    printf("\nyeee");
}