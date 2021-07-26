# Bytecodes to dot

This project defines a LLVM pass that performs and optimizes code based on Range Analysis.

## How to run

~~just walk but faster lol~~

1. Unpack this directory into /llvm/lib/Transforms
2. Edit the file CMakeLists.txt to add `add_subdirectory(dead-code-elim)`
3. Rebuild LLVM by executing `make` in the `/build` directory
4. Open `run_RA.sh` with a text editor and edit the values attributed to the variables CLANG, OPT and RANGE_LIB. The first 2 must refer to llvm's build folder, whereas the latter must point to llvm's lib folder. 
5. To execute the pass, run 

   `./run_RA.sh <c file to be examined>`

The steps above will result in a brief report containing the statistics 
collected during the pass and also generate a `.dot` file for each analysed 
function. 

**Note:** we used LLVM 8.0.

## Improvement points

Our analysis is able to remove obviously redundant code, but we were unable to 
achieve everything we wanted. For example, we were not able to:

- Remove variable declarations that were being used to compute branch-if 
instructions that were removed by our analysis
- Fully merge some basic-blocks. We tried using MergeBlockIntoPredecessor, but
some wild bugs came up, so we developed our own solution, which presents
imperfections.

## Authors

This project was developed by [João Pedro Rosa](https://github.com/jotaRenan) and [Paula Ribeiro](https://github.com/paula-mr) for the Static Analysis (DCC888) course from UFMG.
