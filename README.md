# AutomationTemplate - FPGA101 Versal Class

## Info & Description
This repository contains the main infrastructure for an AIE-PL project, providing simple automation for testing your code.
Nothing here is provided as "the best way to do". Surely there are other solutions, maybe there are better ones. But still,
this is a useful starting point.

## Main Structure

**aie** - contains the code for AI Engine kernels.  
**data_movers** - contains the PL component.  
**common** - contains some useful included constants and headers.  
**hw** - contains the cfg file requiered to connect your components.  
**sw** - contains the software for your application.  

### aie
data - contains the input source for your simulation.  
src - contains the code.  

**Main Commands**

_make aie_compile_x86_ : compile your code for x86 architecture.  
_make aie_simulate_x86_ : simulate your x86 architecture.  
_make aie_compile_ : compile your code for VLIW architecture, as your final hardware for HW ad HW_EMU.  
_make aie_simulate_ : simulate your code for VLIW architecture, as your final hardware.  
_make clean_ : removes all the output file created by the commands listed above.  

### data_movers

testbench : it contains a testbench for each kernel

**Main Commands**

_make compile TARGET=HW/HW_EMU_: it compiles all your kernel, skipping the ones already compiled.  
_make run_testbench_setup_aie_ : compiles and execute the testbench for the kernel setup_aie.  
_make run_testbench_sink_from_aie_ : compiles and execute the testbench for the kernel setup_aie.  

### Hw

Contains the cfg file required to link the components. For the Versal case, you have also to link the AI Engine.

**Main Commands**

_make all TARGET=HW/HW_EMU_ : it builds the hardware or the hardware emu linking your componentsEMU TARGET=HW/HW_EMU
make clean: it removes all files.

### Sw

Once you have devised your accelerator, you need to create the host code for using it. Notice that the presented example is a minimal host code, which may be improved using all the capabilities of C++ code ( classes, abstraction and so on).

**Main Commands**
_make build_sw_ : it compiles the sw

_./setup_emu.sh -s on_ : enables the hardware emulation

i.e.: make build_sw && ./setup_emu.sh && ./host_overlay.exe : this will compile, prepare the emulation, and run it.


## General useful commands:
If you need to move your bitstream and executable on the target machine, you may want it prepared in a single folder that contains all the required stuff to be moved. In this case, you can use the

_make build_and_pack TARGET=hw/hw_emu_ :  it allows you to pack our build in a single folder. Notice that the hw_emu does not have to be moved on the device, it must be executed on the development machine.