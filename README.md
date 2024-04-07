# AutomationTemplate - FPGA101 Versal Class

## Info & Description
This repository contains the main infrastructure for an AIE-PL project, providing simple automation for testing your code.
Nothing here is provided as "the best way to do". Surely there are other solutions, maybe there are better ones. But still,
this is a useful starting point.

## Main Structure

aie - contains the code for AI Engine kernels.  
data_movers - contains the PL component.  
common - contains some useful included constants and headers.  
hw - contains the cfg file requiered to connect your components.  
sw - contains the software for your application.  

### aie
data - contains the input source for your simulation.  
src - contains the code.  

**Main Commands**

make aie_compile_x86 : compile your code for x86 architecture.  
make aie_simulate_x86 : simulate your x86 architecture.  
make aie_compile : compile your code for VLIW architecture, as your final hardware.  
make aie_compile_x86 : simulate your code for VLIW architecture, as your final hardware.  
make clean : removes all the output file created by the commands listed above.  

### data_movers

testbench : it contains a testbench for each kernel

**Main Commands**

make compile : it compiles all your kernel, skipping the ones already compiled.  
make run_testbench_setup_aie : compiles and execute the testbench for the kernel setup_aie.  
make run_testbench_sink_from_aie : compiles and execute the testbench for the kernel setup_aie.  


