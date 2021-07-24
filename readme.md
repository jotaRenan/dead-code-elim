# Bytecodes to dot

This project defines a LLVM pass that performs and optimizes code based on Range Analysis.

## How to run

~~just walk but faster lol~~

1. Unpack this directory into /llvm/lib/Transforms
2. Edit the file CMakeLists.txt to add `add_subdirectory(dead-code-elim)`
3. Rebuild LLVM by executing `make` in the `/build` directory
4. Execute the pass

   `opt -load build/lib/DeadCodeElim.<so | dylib> -enable-new-pm=0 -makeDot -disable-output`

   - The .dylib extension is for shared libraries on MacOS. If you're running on Linux you should use the .so extension
   - The `-enable-new-pm=0` flag disables the new Pass Manager from LLVM and is necessary to run the pass if you're running the newest distribution of LLVM
   - The `-disable-output` is used to avoid printing the binary outputs to the terminal

**Note:** we used LLVM 13.0, since that's the latest stable version from the project's github repo. We noticed there were some API changes regarding the C++ classes we used in comparison to those in LLVM 10.0 and below.

## Authors

This project was developed by [João Pedro Rosa](https://github.com/jotaRenan) and [Paula Ribeiro](https://github.com/paula-mr) for the Static Analysis (DCC888) course from UFMG.
