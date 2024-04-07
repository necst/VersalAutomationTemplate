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

#pragma once
#include <adf.h>
#include "my_kernel_1.h"

using namespace adf;

class my_graph: public graph
{

private:
	// ------kernel declaration------
	kernel my_kernel_1;
	// kernel my_kernel_2;

public:
	// ------Input and Output PLIO declaration------

	input_plio in_1;
	// input_plio in_2;
	output_plio out_1;
	// output_plio out_2;

	my_graph()
	{
		// ------kernel creation------
		my_kernel_1 = kernel::create(my_kernel_function); // the input is the kernel function name

		// ------Input and Output PLIO creation------
		// I argument: a name, that will be used to refer to the port in the block design
		// II argument: the type of the PLIO that will be read/written. Test both plio_32_bits and plio_128_bits to verify the difference
		// III argument: the path to the file that will be read/written for simulation

		in_1 = input_plio::create("in_plio_1", plio_32_bits, "data/in_plio_source_1.txt");
		out_1 = output_plio::create("out_plio_1", plio_32_bits, "data/out_plio_sink_1.txt");

		// ------kernel connection------
		// it is possible to have stream or window. This is just an example. Try both to see the difference
		connect<stream>(in_1.out[0], my_kernel_1.in[0]);
		connect<stream>(my_kernel_1.out[0], out_1.in[0]);
		// set kernel source and headers
		source(my_kernel_1)  = "src/my_kernel_1.cpp";
		headers(my_kernel_1) = {"src/my_kernel_1.h","../common/common.h"};// you can specify more than one header to include

		// set ratio
		runtime<ratio>(my_kernel_1) = 0.9; // 90% of the time the kernel will be executed. This means that 1 AIE will be able to execute just 1 Kernel
	};

};
