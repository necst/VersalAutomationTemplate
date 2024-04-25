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
#include <sys/stat.h>
#include <string>
#include "experimental/xrt_kernel.h"
#include "experimental/xrt_uuid.h"
#include "../common/common.h"

// For hw emulation, run in sw directory: source ./setup_emu.sh -s on

#define DEVICE_ID 0


// every top function input that must be passed from the host to the kernel must have a unique index starting from 0

// args indexes for setup_aie kernel
#define arg_setup_aie_size 0
#define arg_setup_aie_input 1

// args indexes for sink_from_aie kernel
#define arg_sink_from_aie_output 1
#define arg_sink_from_aie_size 2

bool get_xclbin_path(std::string& xclbin_file);
std::ostream& bold_on(std::ostream& os);
std::ostream& bold_off(std::ostream& os);

int checkResult(float* input, float* output, int size) {
    for (int i = 0; i < size; i++) {
        if (input[i] != output[i]) {
            std::cout << "Error at index " << i << ": " << input[i] << " != " << output[i] << std::endl;
            return EXIT_FAILURE;
        }
    }
    std::cout << "Test passed!" << std::endl;
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    
    int size = 32;

    float nums[size];
    for (int i = 0; i < size; i++)
        nums[i] = i + 1;


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

    // create kernel objects
    xrt::kernel krnl_setup_aie  = xrt::kernel(device, xclbin_uuid, "setup_aie");
    xrt::kernel krnl_sink_from_aie  = xrt::kernel(device, xclbin_uuid, "sink_from_aie");

    // get memory bank groups for device buffer - required for axi master input/ouput
    xrtMemoryGroup bank_output  = krnl_sink_from_aie.group_id(arg_sink_from_aie_output);
    xrtMemoryGroup bank_input  = krnl_setup_aie.group_id(arg_setup_aie_input);

    // create device buffers - if you have to load some data, here they are
    xrt::bo buffer_setup_aie= xrt::bo(device, size * sizeof(float), xrt::bo::flags::normal, bank_input); 
    xrt::bo buffer_sink_from_aie = xrt::bo(device, size * sizeof(float), xrt::bo::flags::normal, bank_output); 

    // create runner instances
    xrt::run run_setup_aie   = xrt::run(krnl_setup_aie);
    xrt::run run_sink_from_aie = xrt::run(krnl_sink_from_aie);

    // set setup_aie kernel arguments
    run_setup_aie.set_arg(arg_setup_aie_size,  size);
    run_setup_aie.set_arg(arg_setup_aie_input, buffer_setup_aie);

    // set sink_from_aie kernel arguments
    run_sink_from_aie.set_arg(arg_sink_from_aie_output, buffer_sink_from_aie);
    run_sink_from_aie.set_arg(arg_sink_from_aie_size, size);

    // write data into the input buffer
    buffer_setup_aie.write(nums);
    buffer_setup_aie.sync(XCL_BO_SYNC_BO_TO_DEVICE);

    // run the kernel
    run_sink_from_aie.start();
    run_setup_aie.start();

    // wait for the kernel to finish
    run_setup_aie.wait();
    run_sink_from_aie.wait();

    // read the output buffer
    buffer_sink_from_aie.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    float output_buffer[size];
    buffer_sink_from_aie.read(output_buffer);

    // ---------------------------------CONFRONTO PER VERIFICARE L'ERRORE--------------------------------------
        
    // Here there should be a code for checking correctness of your application, like a software application

    return checkResult(nums, output_buffer, size);
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

