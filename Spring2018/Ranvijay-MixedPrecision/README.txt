Intro:
------
This is for the Automatuc Mixed Precision tuning of GPUs, ie, determining the optimal precision configuration for variables of an application to obtain best performance while maintaining error within allowable levels. This repo contains the LLVM IR pass which builds the dependency graph and outputs a score for a kernel.

The binaries generated here are used in conjunction with a MATLAB script (currently with Pradeep) to obtain the best configuration for a kernel.

This is under submission to SC '18.
Collaborators: Paul Wood, Saurabh Bagchi, Pradeep Kotipalli

Dependencies:
-------------
- LLVM
- compiled and tested on macOS

Instuctions:
------------
1. Build:
	Normal out of source build using CMake

2. Test:
	Test as any other opt pass, will generate model files for the LLVM functions defined in the file
	Sample run in run.sh

3. Output:
	Generates model files corresponding to the functions in the IR file and binaries which use the model files to obtain the score:
	STAscore: call as STAscore [ker_num] [ker_types]
		where ker_num is Lulesh kernel 1, 2 or 3 and ker_types is a string of space separated types, f for float, d for double.
		For example, to get the score for the all float configuration for kernel 1:
		./STAscore 1 f f f … f f
		(where f occurs 37 times corresponding to each variable in kernel 1)

4. Files included:
	Compilation files (*.cpp, *.hpp, CMakeLists.txt)
	Sample run for opt pass (run.sh)
	Generated model files for the 11 Lulesh-CUDA kernels (_Z*)
	Some test LLVM IR files (test/*)
