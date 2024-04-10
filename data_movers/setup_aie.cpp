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


#include <ap_int.h>
#include <hls_stream.h>
#include <ap_axi_sdata.h>


extern "C" {

void setup_aie(int size, float* input, hls::stream<float>& s) {

	#pragma HLS interface m_axi port=input depth=100 offset=slave bundle=gmem0
	#pragma HLS interface axis port=s
	#pragma HLS interface s_axilite port=input bundle=control
	#pragma HLS interface s_axilite port=size bundle=control
	#pragma HLS interface s_axilite port=return bundle=control

	// size represents the number of elements. But the AI Engine uses the number of loops, and each
	// loop uses 4 elements. So we need to convert the number of elements to the number of loops.
	float size_loop = (float) size/4;

	s.write(size_loop);
	for (int j = 0; j < size_loop; j++) {
		s.write(input[j*4+0]);
		s.write(input[j*4+1]);
		s.write(input[j*4+2]);
		s.write(input[j*4+3]);
	}
}
}
