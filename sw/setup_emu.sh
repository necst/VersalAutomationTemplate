#!/bin/bash

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

# Function to print the usage of the script
print_usage () {
  echo "Usage: "
  echo "  setup_hw_emu.sh -s <on/off> --shell <qdma/xdma>    Set emulation mode to on or off"
  echo ""
}

# Check if no arguments are provided
if [ "$1" = "" ]; then
  print_usage
  exit 1
fi

switch=""
platform=""

# Parsing the arguments passed to the script
while [[ $# -gt 0 ]]; do
    key="$1"
    case $key in
        -s)
            case "$2" in
                on) switch="on"; shift 2;;
                off) switch="off"; shift 2;;
                *) print_usage; exit 1;;
            esac ;;
        --shell)
            SHELL_PARAM="$2"; shift 2;;
        -p)
            platform="$2"; shift 2;;
        *) print_usage; exit 1;;
    esac
done

# Check if the --shell parameter was specified correctly
if [[ "$SHELL_PARAM" != "qdma" && "$SHELL_PARAM" != "xdma" ]]; then
    echo "Error: Please specify --shell with 'qdma' or 'xdma'"
    print_usage
    exit 1
fi

# Choose which configuration script to source based on --shell
if [[ "$SHELL_PARAM" == "qdma" ]]; then
    platform="xilinx_vck5000_gen4x8_qdma_2_202220_1"
elif [[ "$SHELL_PARAM" == "xdma" ]]; then
    platform="xilinx_vck5000_gen4x8_xdma_2_202210_1"
fi

# Handle the switch for emulation mode
if [ "$switch" = "off" ]; then
  echo "Exit Emulation Mode"
  unset XCL_EMULATION_MODE
elif [ "$switch" = "on" ]; then
  echo "Generating emulation config file for platform $platform.."
  export XCL_EMULATION_MODE=hw_emu
  emconfigutil --platform $platform
  echo "Enter Hardware Emulation Mode"
else
  print_usage
  exit 1
fi
