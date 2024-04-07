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

#include "experimental/xrt_kernel.h"
#include "experimental/xrt_uuid.h"
#include "../../../common/common.h"
#include "../image_utils/image_utils.hpp"

// args indexes for setup_aie kernel
#define arg_setup_aie_in_tx 0
#define arg_setup_aie_in_ty 1
#define arg_setup_aie_in_ang 2
#define arg_setup_aie_in_n_couples 3

// args indexes for setup_mutualInfo kernel
#define arg_setminfo_in_flt_original_ptr 2
#define arg_setminfo_out_flt_transformed_ptr 3
#define arg_setminfo_in_n_couples 4

// args indexes for mutual_information_master kernel
#define arg_minfo_ref_ptr 1
#define arg_minfo_rslt_ptr 2
#define arg_minfo_ncouples_val 3

// buffer sizes
#define FLOAT_INPUT_BUFFER_SIZE   DIMENSION*DIMENSION*sizeof(uint8_t)  // floating volume size in bytes
#define MINFO_INPUT_BUFFER_SIZE   FLOAT_INPUT_BUFFER_SIZE  // floating volume size in bytes
#define MINFO_OUTPUT_BUFFER_SIZE  sizeof(float)           // size of mutual info data type


class Versal3DIR {
public:
    xrt::device& device;
    xrt::uuid& xclbin_uuid;
    int n_couples;
    int padding;
    size_t buffer_size;

    uint8_t* input_ref = NULL;
    uint8_t* input_flt = NULL;
    uint8_t* output_flt = NULL;

    xrt::kernel krnl_setup_aie;
    xrt::kernel krnl_setminfo;
    xrt::kernel krnl_minfo;

    xrtMemoryGroup bank_setminfo_flt_in;
    xrtMemoryGroup bank_setminfo_flt_transformed;
    xrtMemoryGroup bank_minfo_ref;
    xrtMemoryGroup bank_minfo_rslt;

    xrt::bo buffer_setminfo_flt_in;
    xrt::bo buffer_setminfo_flt_transformed;
    xrt::bo buffer_minfo_ref;
    xrt::bo buffer_minfo_rlst;

    xrt::run run_setup_aie;
    xrt::run run_setup_minfo;
    xrt::run run_minfo;

    //
    // Initialize the board configuring it for a specific volume-depth
    //
    Versal3DIR(xrt::device& device, xrt::uuid& xclbin_uuid, int n_couples) : 
        device(device), 
        xclbin_uuid(xclbin_uuid), 
        n_couples(n_couples), 
        padding((HIST_PE - (n_couples % HIST_PE)) % HIST_PE),
        buffer_size(FLOAT_INPUT_BUFFER_SIZE * (n_couples+padding))
    {
        // create kernel objects
        krnl_setup_aie  = xrt::kernel(device, xclbin_uuid, "setup_aie");
        krnl_setminfo   = xrt::kernel(device, xclbin_uuid, "setup_mutualInfo");
        krnl_minfo = xrt::kernel(device, xclbin_uuid, "mutual_information_master");

        // get memory bank groups for device buffer
        bank_setminfo_flt_in  = krnl_setminfo.group_id(arg_setminfo_in_flt_original_ptr);
        bank_setminfo_flt_transformed = krnl_setminfo.group_id(arg_setminfo_out_flt_transformed_ptr);
        bank_minfo_ref    = krnl_minfo.group_id(arg_minfo_ref_ptr);
        bank_minfo_rslt   = krnl_minfo.group_id(arg_minfo_rslt_ptr);

        // create device buffers
        buffer_setminfo_flt_in          = xrt::bo(device, buffer_size, xrt::bo::flags::normal, bank_setminfo_flt_in); 
        buffer_setminfo_flt_transformed = xrt::bo(device, buffer_size, xrt::bo::flags::normal, bank_setminfo_flt_transformed); 
        buffer_minfo_ref                = xrt::bo(device, buffer_size, xrt::bo::flags::normal, bank_minfo_ref);
        buffer_minfo_rlst               = xrt::bo(device, MINFO_OUTPUT_BUFFER_SIZE, xrt::bo::flags::normal, bank_minfo_rslt);

        // create kernel runner instances
        run_setup_aie   = xrt::run(krnl_setup_aie);
        run_setup_minfo = xrt::run(krnl_setminfo);
        run_minfo       = xrt::run(krnl_minfo);

        // set setup_setminfo kernel arguments
        run_setup_minfo.set_arg(arg_setminfo_in_flt_original_ptr, buffer_setminfo_flt_in);
        run_setup_minfo.set_arg(arg_setminfo_out_flt_transformed_ptr, buffer_setminfo_flt_transformed);
        run_setup_minfo.set_arg(arg_setminfo_in_n_couples, n_couples+padding);
        
        // set mutual_info kernel arguments
        run_minfo.set_arg(arg_minfo_ref_ptr, buffer_minfo_ref);
        run_minfo.set_arg(arg_minfo_rslt_ptr, buffer_minfo_rlst);
        run_minfo.set_arg(arg_minfo_ncouples_val, n_couples+padding);
    }

    //
    // Read volumes from file
    //
    int read_volumes_from_file(const std::string &path_ref, const std::string &path_flt, const ImageFormat imageFormat = ImageFormat::PNG) {
        input_ref  = new uint8_t[DIMENSION*DIMENSION * (n_couples+padding)];
        input_flt  = new uint8_t[DIMENSION*DIMENSION * (n_couples+padding)];
        output_flt = new uint8_t[DIMENSION*DIMENSION * (n_couples+padding)];
        
        if (read_volume_from_file(input_ref, DIMENSION, n_couples, padding, path_ref, imageFormat) == -1) {
            std::cerr << "Error: Could not open reference volume. Some file in path \"" << path_ref << "\" might not exist" << std::endl;
            return -1;
        }

        if (read_volume_from_file(input_flt, DIMENSION, n_couples, padding, path_flt, imageFormat) == -1) {
            std::cerr << "Error: Could not open floating volume. Some file in path \"" << path_flt << "\" might not exist" << std::endl;
            return -1;
        }
    }

    //
    // Set the transformation parameters
    //
    void set_transform_params(float TX, float TY, float ANG) {
        // set setup_aie kernel arguments
        run_setup_aie.set_arg(arg_setup_aie_in_tx,  TX);
        run_setup_aie.set_arg(arg_setup_aie_in_ty,  TY);
        run_setup_aie.set_arg(arg_setup_aie_in_ang, ANG);
        run_setup_aie.set_arg(arg_setup_aie_in_n_couples, (float)(n_couples+padding));
    }

    // 
    // Transform the floating volume to the board
    //
    void write_floating_volume(double* duration = NULL) {
        Timer timer_transfer_flt_write;
        if (duration != NULL) timer_transfer_flt_write.start();
        buffer_setminfo_flt_in.write(input_flt);
        buffer_setminfo_flt_in.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        if (duration != NULL) *duration += timer_transfer_flt_write.getElapsedSeconds();
    }

    // 
    // Transform the reference volume to the board
    //
    void write_reference_volume(double* duration = NULL) {
        Timer timer_transfer_ref_write;
        if (duration != NULL) timer_transfer_ref_write.start();
        buffer_minfo_ref.write(input_ref);
        buffer_minfo_ref.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        if (duration != NULL) *duration += timer_transfer_ref_write.getElapsedSeconds();
    }

    //
    // Run the kernels
    //
    void run(double* duration = NULL) {
        // run the pl kernels
        Timer timer_execution;
        if (duration != NULL) timer_execution.start();
        run_setup_aie.start();
        run_setup_minfo.start();
        run_minfo.start();
        // waiting for kernels to finish
        run_minfo.wait();
        run_setup_minfo.wait();
        run_setup_aie.wait();
        if (duration != NULL) *duration += timer_execution.getElapsedSeconds();
    }

    //
    // Read the transformed floating volume from the board
    //
    void read_flt_transformed(double* duration = NULL) {
        Timer timer_transfer_read_flt;
        if (duration != NULL) timer_transfer_read_flt.start();
        buffer_setminfo_flt_transformed.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
        buffer_setminfo_flt_transformed.read(output_flt);
        if (duration != NULL) *duration += timer_transfer_read_flt.getElapsedSeconds();
    }

    // 
    // Read the mutual information from the board
    //
    float read_mutual_information() {
        float output_data;
        buffer_minfo_rlst.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
        buffer_minfo_rlst.read(&output_data);
        return output_data;
    }

    ~Versal3DIR() {
        delete[] input_ref;
        delete[] input_flt;
        delete[] output_flt;
    }
};