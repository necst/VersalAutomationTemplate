#include "my_kernel_1.h"
#include "common.h"
#include "aie_api/aie.hpp"
#include "aie_api/aie_adf.hpp"
#include "aie_api/utils.hpp"

//API REFERENCE for STREAM: 
// https://docs.amd.com/r/ehttps://docs.amd.com/r/en-US/ug1079-ai-engine-kernel-coding/Reading-and-Advancing-an-Input-Streamn-US/ug1079-ai-engine-kernel-coding/Reading-and-Advancing-an-Input-Stream

void my_kernel_function (input_stream<float>* restrict input, output_stream<float>* restrict output)
{
    // read from one stream and write to another
    uint8 tot_num = readincr(input); // the first number tells me how many 32_vectors are in input

    for (int i = 0; i < tot_num; i++)
    {
        aie::vector<float,4> x = readincr_v<4>(input); // 1 Float = 32 bit. 32 x 4 = 128 bit -> 128-bit wide stream operation.
        writeincr(output,x);
    }   
}