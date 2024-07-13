#!/bin/bash

# Function to print the correct usage of the script
print_usage() {
    echo "Usage: $0 [--shell <qdma|xdma>]"
    echo "Options:"
    echo "  --shell <qdma|xdma>   Specify the shell environment to enable"
    exit 1
}

# Variables for the paths of configuration scripts
VITIS_22_1="/home/xilinx/software/Vitis/2022.1/settings64.sh"
VITIS_22_2="/home/xilinx/software/Vitis/2022.2/settings64.sh"

# Variable for the --shell parameter, defaulting to null
SHELL_PARAM=null

# Parsing the arguments passed to the script
while [[ $# -gt 0 ]]; do
    key="$1"
    case $key in
        --shell)
            SHELL_PARAM="$2"
            shift # move to the value of --shell
            shift # move to the next argument
            ;;
        *)
            # Unknown option
            echo "Error: Unknown option $1"
            print_usage
            ;;
    esac
done

# Check if the --shell parameter was specified correctly
if [[ "$SHELL_PARAM" != "qdma" && "$SHELL_PARAM" != "xdma" ]]; then
    echo "Error: Please specify --shell with 'qdma' or 'xdma'"
    print_usage
fi

# Choose which configuration script to source based on --shell
if [[ "$SHELL_PARAM" == "qdma" ]]; then
    source "$VITIS_22_2"
else
    source "$VITIS_22_1"
fi

# Enable the devtoolset-7 development environment
scl enable devtoolset-7 bash
