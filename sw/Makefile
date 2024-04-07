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
	$(ECHO) "  make all"
	$(ECHO) ""
	$(ECHO) "  make clean"
	$(ECHO) "      Command to remove all the generated files."
	$(ECHO) ""

CONFIG = ../default.cfg
include ${CONFIG}


.phony: clean


################## software build for XRT Native API code
CXXFLAGS := -std=c++17 -Wno-deprecated-declarations
CXXFLAGS += -I$(XILINX_XRT)/include -I$(XILINX_HLS)/include

LDFLAGS := -L$(XILINX_XRT)/lib
LDFLAGS += $(LDFLAGS) -lxrt_coreutil

LIBS = `pkg-config --libs opencv`
CFLAGS = `pkg-config --cflags opencv`

EXECUTABLE := host_overlay.exe

HOST_SRCS := ./host_code.cpp
HOST_SRCS += ./include/image_utils/image_utils.cpp
HOST_SRCS += ./include/software_mi/software_mi.cpp

DATASET_OUTPUT_DIR := dataset_output
DATASET_SW_OUTPUT_DIR := dataset_sw_output

all: build_sw
build_sw: $(EXECUTABLE)

run_sw:
	mkdir -p $(DATASET_OUTPUT_DIR)
	mkdir -p $(DATASET_SW_OUTPUT_DIR)
	./$(EXECUTABLE)

$(EXECUTABLE): $(HOST_SRCS)
	$(CXX) -o $(EXECUTABLE) $^ $(CXXFLAGS) $(LDFLAGS) $(CFLAGS) $(LIBS)
	@rm -f ./overlay_hw.xclbin
	@rm -f ./overlay_hw_emu.xclbin
	@ln -s ../hw/overlay_hw.xclbin
	@ln -s ../hw/overlay_hw_emu.xclbin

switch_dataset:
	$(info )
	$(info **************** Generating new dataset ****************)
	$(info - DIMENSION        $(DIMENSION))
	$(info - N_COUPLES        $(N_COUPLES))
	$(info ********************************************************)
	$(info )
	rm -f dataset/IM*.png
	cp dataset/size_$(DIMENSION)/IM1.png dataset/
	$(info Duplicating slices $(N_COUPLES) times)
	cd dataset && ./duplicate_slices.sh 2 $(N_COUPLES)

################## clean up
clean:
	$(RM) -r _x .Xil *.ltx *.log *.jou *.info host_overlay.exe *.xo *.xo.* *.str *.xclbin .run
	
