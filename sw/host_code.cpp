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

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <unistd.h>
#include "opencv2/opencv.hpp"
#include <sys/stat.h>
#include <string>
#include "experimental/xrt_kernel.h"
#include "experimental/xrt_uuid.h"
#include "../common/common.h" // TODO non dovrebbe servire

#include "./include/software_mi/software_mi.hpp"

// For hw emulation, run in sw directory: source ./setup_emu.sh -s on

#define DEVICE_ID   0

#include "./include/versal_3dir/Versal3DIR.cpp"

bool get_xclbin_path(std::string& xclbin_file);
std::ostream& bold_on(std::ostream& os);
std::ostream& bold_off(std::ostream& os);
void print_header(int n_couples, float TX, float TY, float ANG_DEG);

float hw_transform(Versal3DIR& board, float TX, float TY, float ANG, double* duration_exec = NULL) {
    board.set_transform_params(TX, TY, ANG);
    board.run(duration_exec);
    return board.read_mutual_information();
}

int main(int argc, char *argv[]) {
    
    int n_couples = 512;
    if (argc == 2) {
        n_couples = atoi(argv[1]);
        if (n_couples > N_COUPLES_MAX)
            n_couples = N_COUPLES_MAX;
    }

    const float TX = 30;
    const float TY = 30;
    const float ANG_DEG = 25;
    const float ANG = (ANG_DEG * M_PI) / 180.f; // radians
    const int N_RUNS = 1;

    print_header(n_couples, TX, TY, ANG_DEG);

//------------------------------------------------LOADING XCLBIN------------------------------------------    
    std::string xclbin_file;
    if (!get_xclbin_path(xclbin_file))
        return EXIT_FAILURE;

    // Load xclbin
    std::cout << "1. Loading bitstream (" << xclbin_file << ")... ";
    xrt::device device = xrt::device(DEVICE_ID);
    xrt::uuid xclbin_uuid = device.load_xclbin(xclbin_file);
    std::cout << "Done" << std::endl;

//----------------------------------------------INITIALIZING THE BOARD------------------------------------------
    Versal3DIR board = Versal3DIR(device, xclbin_uuid, n_couples);

    printf("Padding: %d\n", board.padding);

    // Read input volumes from disk
    std::cout << "2. Reading input volumes from disk... ";
    if (board.read_volumes_from_file("dataset/", "dataset/") == -1)
        return -1;
    std::cout << "Done" << std::endl;

    // Write input volumes to board's main memory
    double duration_write_flt_sec = 0;
    std::cout << "3. Transfering volumes to board's main memory... ";
    board.write_floating_volume(&duration_write_flt_sec);
    board.write_reference_volume(&duration_write_flt_sec);   
    std::cout << "Done\t\t[" << duration_write_flt_sec << " s]" << std::endl;

//------------------------------------------------RUNNING KERNELS------------------------------------------
    float output_data = -1.0f;

    if (N_RUNS > 1)
        std::cout << "4. Running " << N_RUNS << " times" << std::endl;
    else
        std::cout << "4. Running 1 time" << std::endl;

    // files contaning the execution times
    std::ofstream outfile;
    outfile.open("time.csv", std::ios_base::app); // append instead of overwrite
    outfile << "time,"<< std::endl;
    std::ofstream outfile_sw;
    outfile_sw.open("time_sw.csv", std::ios_base::app); // append instead of overwrite
    outfile_sw << "time,"<< std::endl;

    double duration_sw_sec = 0;
    double duration_execution_sec = 0;
    double duration_read_flt_sec = 0;
    double mutualinfo;

    for (int i = 0; i < N_RUNS; i++) {
        duration_execution_sec = 0;
        duration_read_flt_sec = 0;
        duration_sw_sec = 0;

        // Running on the board
        std::cout <<  bold_on << "5. Running 3DIR step on hardware... ";
        output_data = hw_transform(board, TX, TY, ANG, &duration_execution_sec);
        std::cout << "Done\t\t\t[" << duration_execution_sec << " s]" << bold_off << std::endl;

        //------------------------------------------------SAVING RESULTS------------------------------------------
        std::cout << "6. Transfering output volume from board's main memory... ";
        board.read_flt_transformed(&duration_read_flt_sec);
        std::cout << "Done\t[" << duration_read_flt_sec << " s]" << std::endl;
        outfile << duration_execution_sec + duration_write_flt_sec + duration_read_flt_sec <<std::endl;
        
        std::cout << "7. Writing output volume on disk... ";
        write_volume_to_file(board.output_flt, DIMENSION , n_couples, board.padding, "dataset_output/");
        std::cout << "Done" << std::endl;

        // ---------------------------------CONFRONTO PER VERIFICARE L'ERRORE--------------------------------------
        std::cout << bold_on << "8. Running software implementation... " << std::flush;
        mutualinfo = software_mi(n_couples, TX, TY, ANG, "dataset/", &duration_sw_sec);
        std::cout << "Done\t\t\t[" << duration_sw_sec << " s]" << bold_off << std::endl;
        std::cout << std::endl;

        outfile_sw << duration_sw_sec << std::endl;
    }
    
    double tot_time = duration_write_flt_sec + duration_execution_sec + duration_read_flt_sec;

    std::cout << "--- RESULTS --------------------------------------------------------------------\n\n";
    std::cout << "Mutual information:" << std::endl;
    std::cout << bold_on << "  HW:\t\t" << output_data << bold_off << std::endl;
    std::cout << "  SW:\t\t" << mutualinfo << std::endl;
    std::cout << "  Error:\t" << abs(mutualinfo - output_data) << std::endl;
    std::cout << std::endl;

    float speedup = duration_sw_sec / duration_execution_sec; // sw time / hw time
    std::cout << "Execution time:" << std::endl;
    std::cout << bold_on << "  HW:\t\t" << duration_execution_sec << bold_off << std::endl;
    std::cout << "  SW:\t\t" << duration_sw_sec << std::endl << std::endl;

    std::cout << std::fixed << std::setprecision(2);
    std::cout << bold_on << "Speedup: " << speedup << bold_off << std::endl << std::endl;

    if(((data_t)mutualinfo - output_data > 0.0001)){
        printf("oh no!");
        return 1;
    }

    return 0;
}






bool get_xclbin_path(std::string& xclbin_file) {
    // Judge emulation mode accoring to env variable
    char *env_emu;
    if (env_emu = getenv("XCL_EMULATION_MODE")) {
        std::string mode(env_emu);
        if (mode == "hw_emu")
        {
            std::cout << "Program running in hardware emulation mode" << std::endl;
            xclbin_file = "overlay_hw_emu.xclbin";
        }
        else
        {
            std::cout << "[ERROR] Unsupported Emulation Mode: " << mode << std::endl;
            return false;
        }
    }
    else {
        std::cout << bold_on << "Program running in hardware mode" << bold_off << std::endl;
        xclbin_file = "overlay_hw.xclbin";
    }

    std::cout << std::endl << std::endl;
    return true;
}

std::ostream& bold_on(std::ostream& os)
{
    return os << "\e[1m";
}

std::ostream& bold_off(std::ostream& os)
{
    return os << "\e[0m";
}

void print_header(int n_couples, float TX, float TY, float ANG_DEG) {
    std::cout << std::endl << std::endl;
    std::cout << "   __      __                 _   ____  _____ _____ _____  " << std::endl;
    std::cout << "   \\ \\    / /                | | |___ \\|  __ \\_   _|  __ \\ " << std::endl;
    std::cout << "    \\ \\  / /__ _ __ ___  __ _| |   __) | |  | || | | |__) |" << std::endl;
    std::cout << "     \\ \\/ / _ \\ '__/ __|/ _` | |  |__ <| |  | || | |  _  / " << std::endl;
    std::cout << "      \\  /  __/ |  \\__ \\ (_| | |  ___) | |__| || |_| | \\ \\ " << std::endl;
    std::cout << "       \\/ \\___|_|  |___/\\__,_|_| |____/|_____/_____|_|  \\_\\" << std::endl;
    std::cout << std::endl << std::endl;

    std::cout << "+-----------------------+" << std::endl;
    std::cout << "| DATASET:              |" << std::endl;
    std::cout << "|   Resolution\t" << bold_on << DIMENSION << 'x' << DIMENSION << bold_off << " | " << std::endl;
    std::cout << "|   Depth\t" << bold_on << n_couples << bold_off << "     |" << std::endl;
    std::cout << "+-----------------------+" << std::endl;
    std::cout << "| TRANSFORMATION:       |" << std::endl;
    std::cout << "|   TX\t\t" << bold_on << TX << bold_off << " px   |" << std::endl;
    std::cout << "|   TY\t\t" << bold_on << TY << bold_off << " px   |" << std::endl;
    std::cout << "|   ANG\t\t" << bold_on << ANG_DEG << bold_off << " deg  |" << std::endl;
    std::cout << "+-----------------------+" << std::endl;
    std::cout << std::endl;
    std::cout << std::fixed << std::setprecision(6);
}
