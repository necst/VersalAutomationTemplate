# MIT License

# Copyright (c) 2023 Paolo Salvatore Galfano, Giuseppe Sorrentino

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

ECHO=@echo

.PHONY: help

help::
	$(ECHO) "Makefile Usage:"
	$(ECHO) ""
	$(ECHO) "  make compile"
	$(ECHO) "      Command to generate xo kernel file"
	$(ECHO) ""
	$(ECHO) "  make clean"
	$(ECHO) "      Command to remove all the generated files."

PLATFORM := xilinx_vck5000_gen4x8_xdma_2_202210_1

TARGET := hw

CONFIG = ../default.cfg
include ${CONFIG}

.phony: clean

################## hardware build 

XOCCFLAGS := --platform $(PLATFORM) -t $(TARGET)  -s -g

compile: setup_aie_$(TARGET).xo setup_mutualInfo_$(TARGET).xo 

setup_aie_$(TARGET).xo: ./setup_aie.cpp
	v++ $(XOCCFLAGS) --kernel setup_aie -c -o $@ $<

setup_mutualInfo_$(TARGET).xo: ./setup_mutualInfo.cpp
	v++ $(XOCCFLAGS) --kernel setup_mutualInfo -c -o $@ $<

testbench_setminfo: testbench/testbench_setminfo.cpp
	g++ -std=c++14 -I. -I$(XILINX_HLS)/include -o testbench/$@ $^ -O2 `pkg-config --libs opencv` `pkg-config --cflags opencv`

run_testbench_setminfo:
	@mkdir -p testbench/dataset_output
	@mkdir -p testbench/dataset_output_sw
	cd testbench && ./testbench_setminfo $(TX) $(TY) $(ANG) $(N_COUPLES)

testbench_setupaie: testbench/testbench_setupaie.cpp
	g++ -std=c++14 -I. -I$(XILINX_HLS)/include -o testbench/$@ $^ -O2

run_testbench_setupaie:
	cd testbench && ./testbench_setupaie $(TX) $(TY) $(ANG) $(N_COUPLES)

testbench_all: testbench_setupaie testbench_setminfo
	cd testbench && (./testbench_setupaie $(TX) $(TY) $(ANG) $(N_COUPLES) > ../../aie/data/s0.txt)
	$(warning NOT TESTED: AIE X86 MUST BE COMPILED MANUALLY)
	make -C ../aie aie_simulate_x86
	cp ../aie/x86simulator_output/data/result.txt testbench/coords_in.txt
	@mkdir -p testbench/dataset_output
	@mkdir -p testbench/dataset_output_sw
	cd testbench && ./testbench_setminfo $(TX) $(TY) $(ANG) $(N_COUPLES)

testbench_noaie: testbench_setminfo
	@mkdir -p testbench/dataset_output
	@mkdir -p testbench/dataset_output_sw
	cd testbench && ./testbench_setminfo $(TX) $(TY) $(ANG) $(N_COUPLES)

################## clean up
clean:
	$(RM) -rf *.xo *.xclbin *.xclbin.info *.xclbin.link_summary *.jou *.log *.xo.compile_summary _x .Xil
