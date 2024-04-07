#include "kernel_transform.h"
#include "common.h"
#include "aie_api/aie.hpp"
#include "aie_api/aie_adf.hpp"
#include "aie_api/utils.hpp"

void my_kernel_function (input_stream<float>* restrict input, output_stream<float>* restrict output)
{
    // read from one stream and write to another
    float x = params_in->readincr();
    coords_out->write(x);
}