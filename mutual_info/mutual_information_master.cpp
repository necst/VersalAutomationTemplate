
/***************************************************************
*
* High-Level-Synthesis implementation file for Mutual Information computation
*
****************************************************************/
#include "./include/hw/mutualInfo/histogram.h"
#include "./include/hw/mutualInfo/entropy.h"
#include <stdio.h>
#include <string.h>
#include "assert.h"
#include "include/hw/mutualInfo/mutual_info.hpp"
#include "hls_math.h"
#include "include/hw/mutualInfo/utils.hpp"
#include "stdlib.h"

const unsigned int fifo_in_depth =  (N_COUPLES_MAX*MYROWS*MYCOLS)/(HIST_PE);
const unsigned int fifo_out_depth = 1;
const unsigned int pe_j_h_partition = HIST_PE;
const unsigned int maxCouples=N_COUPLES_MAX;

typedef MinHistBits_t HIST_TYPE;
typedef MinHistPEBits_t HIST_PE_TYPE;


typedef enum FUNCTION_T {
	LOAD_IMG = 0,
	COMPUTE = 1
} FUNCTION;


void compute(hls::stream<INPUT_DATA_TYPE> &flt_stream, INPUT_DATA_TYPE * input_ref,  data_t * mutual_info, unsigned int n_couples){
	//The end_reset params resets the content of j_h;
	//If not set, the PE memories will accumulate over different iterations.
	//It is set to 1 at the end of the data flow.


// ATTENTO:
	// INLINE era default, quindi faceva l'INLINE di una sola funzione.
		// opzione 1: lo tolgo, perchè VITIS fa automaticamente l'Inline di piccole funzioni
		// opzione 2: lo lascio OFF, cosi forzo a non fare l'INLINE
		// opzione 3: lo lascio RECURSIVE, per fare inlining su più livelli
// Cosa fa Inline?
/*
 * Removes a function as a separate entity in the hierarchy.
 * After inlining, the function is dissolved into the calling function and
 * no longer appears as a separate level of hierarchy in the RTL.
 *
 * In some cases, inlining a function allows operations within the function
 * to be shared and optimized more effectively with the calling function.
 * However, an inlined function cannot be shared or reused,
 * so if the parent function calls the inlined function multiple times,
 * this can increase the area required for implementing the RTL.
 * */

// per adesso lascio commentato
// #ifndef CACHING
// 	#pragma HLS INLINE RECURSIVE
// #endif

#pragma HLS DATAFLOW

static	hls::stream<INPUT_DATA_TYPE> ref_stream("ref_stream");
#pragma HLS STREAM variable=ref_stream depth=2 dim=1
// static	hls::stream<INPUT_DATA_TYPE> flt_stream("flt_stream");
// #pragma HLS STREAM variable=flt_stream depth=2 dim=1

static  hls::stream<UNPACK_DATA_TYPE> ref_pe_stream[HIST_PE];
#pragma HLS STREAM variable=ref_pe_stream depth=2 dim=1
static  hls::stream<UNPACK_DATA_TYPE> flt_pe_stream[HIST_PE];
#pragma HLS STREAM variable=flt_pe_stream depth=2 dim=1

static	hls::stream<PACKED_HIST_PE_DATA_TYPE> j_h_pe_stream[HIST_PE];
#pragma HLS STREAM variable=j_h_pe_stream depth=2 dim=1

static	hls::stream<PACKED_HIST_DATA_TYPE> joint_j_h_stream("joint_j_h_stream");
#pragma HLS STREAM variable=joint_j_h_stream depth=2 dim=1
static	hls::stream<PACKED_HIST_DATA_TYPE> joint_j_h_stream_0("joint_j_h_stream_0");
#pragma HLS STREAM variable=joint_j_h_stream_0 depth=2 dim=1
static	hls::stream<PACKED_HIST_DATA_TYPE> joint_j_h_stream_1("joint_j_h_stream_1");
#pragma HLS STREAM variable=joint_j_h_stream_1 depth=2 dim=1
static	hls::stream<PACKED_HIST_DATA_TYPE> joint_j_h_stream_2("joint_j_h_stream_2");
#pragma HLS STREAM variable=joint_j_h_stream_2 depth=2 dim=1

static	hls::stream<PACKED_HIST_DATA_TYPE> row_hist_stream("row_hist_stream");
#pragma HLS STREAM variable=row_hist_stream depth=2 dim=1
static	hls::stream<PACKED_HIST_DATA_TYPE> col_hist_stream("col_hist_stream");
#pragma HLS STREAM variable=col_hist_stream depth=2 dim=1

static	hls::stream<OUT_ENTROPY_TYPE> full_entropy_stream("full_entropy_stream");
#pragma HLS STREAM variable=full_entropy_stream depth=2 dim=1
static	hls::stream<OUT_ENTROPY_TYPE> row_entropy_stream("row_entropy_stream");
#pragma HLS STREAM variable=row_entropy_stream depth=2 dim=1
static	hls::stream<OUT_ENTROPY_TYPE> col_entropy_stream("col_entropy_stream");
#pragma HLS STREAM variable=col_entropy_stream depth=2 dim=1

static	hls::stream<HIST_TYPE> full_hist_split_stream[ENTROPY_PE];
#pragma HLS STREAM variable=full_hist_split_stream depth=2 dim=1
static	hls::stream<HIST_TYPE> row_hist_split_stream[ENTROPY_PE];
#pragma HLS STREAM variable=row_hist_split_stream depth=2 dim=1
static	hls::stream<HIST_TYPE> col_hist_split_stream[ENTROPY_PE];
#pragma HLS STREAM variable=col_hist_split_stream depth=2 dim=1

static	hls::stream<OUT_ENTROPY_TYPE> full_entropy_split_stream[ENTROPY_PE];
#pragma HLS STREAM variable=full_entropy_split_stream depth=2 dim=1
static	hls::stream<OUT_ENTROPY_TYPE> row_entropy_split_stream[ENTROPY_PE];
#pragma HLS STREAM variable=row_entropy_split_stream depth=2 dim=1
static	hls::stream<OUT_ENTROPY_TYPE> col_entropy_split_stream[ENTROPY_PE];
#pragma HLS STREAM variable=col_entropy_split_stream depth=2 dim=1


static	hls::stream<data_t> mutual_information_stream("mutual_information_stream");
#pragma HLS STREAM variable=mutual_information_stream depth=2 dim=1


	// Step 1: read data from DDR and split them
	
	//axi2stream<INPUT_DATA_TYPE, NUM_INPUT_DATA>(flt_stream, input_img);
#ifndef CACHING
	axi2stream_volume<INPUT_DATA_TYPE, NUM_INPUT_DATA>(ref_stream, input_ref,n_couples);
#else
	bram2stream<INPUT_DATA_TYPE, NUM_INPUT_DATA>(ref_stream, input_ref);
#endif

	split_stream_volume<INPUT_DATA_TYPE, UNPACK_DATA_TYPE, UNPACK_DATA_BITWIDTH, NUM_INPUT_DATA, HIST_PE>(ref_stream, ref_pe_stream,n_couples);
	split_stream_volume<INPUT_DATA_TYPE, UNPACK_DATA_TYPE, UNPACK_DATA_BITWIDTH, NUM_INPUT_DATA, HIST_PE>(flt_stream, flt_pe_stream,n_couples);
	// End Step 1


	// Step 2: Compute two histograms in parallel
	WRAPPER_HIST(HIST_PE)<UNPACK_DATA_TYPE, NUM_INPUT_DATA, HIST_PE_TYPE, PACKED_HIST_PE_DATA_TYPE, MIN_HIST_PE_BITS>(ref_pe_stream, flt_pe_stream, j_h_pe_stream,n_couples);
	sum_joint_histogram<PACKED_HIST_PE_DATA_TYPE, J_HISTO_ROWS*J_HISTO_COLS/ENTROPY_PE, PACKED_HIST_DATA_TYPE, HIST_PE, HIST_PE_TYPE, MIN_HIST_PE_BITS, HIST_TYPE, MIN_HIST_BITS>(j_h_pe_stream, joint_j_h_stream);
	// End Step 2


	// Step 3: Compute histograms per row and column
	tri_stream<PACKED_HIST_DATA_TYPE, J_HISTO_ROWS*J_HISTO_COLS/ENTROPY_PE>(joint_j_h_stream, joint_j_h_stream_0, joint_j_h_stream_1, joint_j_h_stream_2);

	hist_row<PACKED_HIST_DATA_TYPE, J_HISTO_ROWS, J_HISTO_COLS/ENTROPY_PE, PACKED_HIST_DATA_TYPE, HIST_TYPE, MIN_HIST_BITS>(joint_j_h_stream_0, row_hist_stream);
	hist_col<PACKED_HIST_DATA_TYPE, J_HISTO_ROWS, J_HISTO_COLS/ENTROPY_PE>(joint_j_h_stream_1, col_hist_stream);
	// End Step 3


	// Step 4: Compute Entropies
	WRAPPER_ENTROPY(ENTROPY_PE)<PACKED_HIST_DATA_TYPE, HIST_TYPE, OUT_ENTROPY_TYPE, J_HISTO_ROWS*J_HISTO_COLS/ENTROPY_PE>(joint_j_h_stream_2, full_hist_split_stream, full_entropy_split_stream, full_entropy_stream);
	WRAPPER_ENTROPY(ENTROPY_PE)<PACKED_HIST_DATA_TYPE, HIST_TYPE, OUT_ENTROPY_TYPE, J_HISTO_ROWS/ENTROPY_PE>(row_hist_stream, row_hist_split_stream, row_entropy_split_stream, row_entropy_stream);
	WRAPPER_ENTROPY(ENTROPY_PE)<PACKED_HIST_DATA_TYPE, HIST_TYPE, OUT_ENTROPY_TYPE, J_HISTO_COLS/ENTROPY_PE>(col_hist_stream, col_hist_split_stream, col_entropy_split_stream, col_entropy_stream);
	// End Step 4


	// Step 6: Mutual information
	compute_mutual_information<OUT_ENTROPY_TYPE, data_t>(row_entropy_stream, col_entropy_stream, full_entropy_stream, mutual_information_stream, n_couples);
	// End Step 6


	// Step 7: Write result back to DDR
	stream2axi<data_t, fifo_out_depth>(mutual_info, mutual_information_stream);

}


template<typename T, unsigned int size>
void copyData(T* in, T* out){
	for(int i = 0; i < size; i++){
#pragma HLS PIPELINE
		out[i] = in[i];
	}
}


#ifndef CACHING

#ifdef KERNEL_NAME
extern "C"{
	void KERNEL_NAME
#else
	void mutual_information_master
#endif //KERNEL_NAME
(hls::stream<INPUT_DATA_TYPE> & stream_input_img, INPUT_DATA_TYPE * input_ref, data_t * mutual_info, unsigned int n_couples){

//#pragma HLS INTERFACE m_axi port=input_img depth=fifo_in_depth offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=input_ref depth=fifo_in_depth offset=slave bundle=gmem1
#pragma HLS INTERFACE m_axi port=mutual_info depth=1 offset=slave bundle=gmem2

//#pragma HLS INTERFACE s_axilite port=input_img bundle=control
#pragma HLS INTERFACE s_axilite port=input_ref bundle=control
#pragma HLS INTERFACE s_axilite port=mutual_info register bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control
#pragma HLS INTERFACE s_axilite port=n_couples register bundle=control
// N-couples pragma correctly added
	if(n_couples > N_COUPLES_MAX)
		n_couples = N_COUPLES_MAX;

	compute(stream_input_img, input_ref, mutual_info, n_couples);
}



#else // CACHING


#ifdef KERNEL_NAME
extern "C"{
	void KERNEL_NAME
#else
	void mutual_information_master
#endif //KERNEL_NAME
(INPUT_DATA_TYPE * input_img,  data_t * mutual_info, unsigned int functionality, int *status, unsigned int n_couples){
#pragma HLS INTERFACE m_axi port=input_img depth=fifo_in_depth offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=mutual_info depth=1 offset=slave bundle=gmem2
#pragma HLS INTERFACE m_axi port=status depth=1 offset=slave bundle=gmem1

#pragma HLS INTERFACE s_axilite port=input_img bundle=control
#pragma HLS INTERFACE s_axilite port=mutual_info register bundle=control
#pragma HLS INTERFACE s_axilite port=functionality register bundle=control
#pragma HLS INTERFACE s_axilite port=status register bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control
#pragma HLS INTERFACE s_axilite port=n_couples register bundle=control


static INPUT_DATA_TYPE ref_img[n_couples * DIMENSION * DIMENSION] = {0};

#ifdef URAM
#pragma HLS RESOURCE variable=ref_img core=RAM_1P_URAM
#endif //URAM

	switch(functionality){
	case LOAD_IMG:	copyData<INPUT_DATA_TYPE, NUM_INPUT_DATA>(input_img, ref_img);
					*status = 1;
					*mutual_info = 0.0;
					break;
	case COMPUTE:	if(n_couples > N_COUPLES_MAX)
						n_couples = N_COUPLES_MAX;
					compute_loop_2: for(int k = 0; k < n_couples; k++) {
						#pragma HLS LOOP_TRIPCOUNT min=1 max=maxCouples
						compute(input_img + k * DIMENSION * DIMENSION / HIST_PE, input_ref + k * DIMENSION * DIMENSION / HIST_PE, mutual_info, k == n_couples - 1, n_couples);
					}
					*status = 1;
					break;
	default:		*status = -1;
					*mutual_info = 0.0;
					break;
	}
}

#endif //CACHING

#ifdef KERNEL_NAME

} // extern "C"

#endif //KERNEL_NAME
