# CryptonightR 

This is proof of concept repository. It introduces random integer math into CryptonightV2 main loop.

### Random integer math modification

Division and square root are replaced with a sequence of random integer instructions:

OP|Description|Frequency|Comment
--|-----------|---------|-------
MUL|a\*b|3/8|Many multiplications ensure high latency
ADD|a+b+C|1/8|3-way addition with random constant
SUB|a-b|1/8|b is always different from a
ROR|ror(a,b)|1/8|Bit rotate right
ROL|rol(a,b)|1/8|Bit rotate left
XOR|a^b|1/8|b is always different from a

There are 8 registers named R0-R7. Registers R0-R3 are variable, registers R4-R7 are constant and can only be used as source register in each instruction. Registers R4-R7 are initialized with values from main loop registers on every main loop iteration.

All registers are 32 bit to enable efficient GPU implementation.

The random sequence changes every block. Block height is used as a seed for random number generator. This allows CPU/GPU miners to precompile optimized code for each block. It also allows to verify optimized code for all future blocks against reference implementation, so it'll be guaranteed safe to use in monero daemon/wallet software.

Reference implementation (variant4 code in the following files):
- https://github.com/SChernykh/slow_hash_test/blob/CryptonightR/variant4_random_math.h
- https://github.com/SChernykh/slow_hash_test/blob/CryptonightR/slow-hash.c

### Design choices

Instruction set is chosen from instructions that are efficient on CPUs/GPUs compared to ASIC: all of them except XOR are complex operations at logic circuit level and require O(logN) gate delay. These operations have been studied extensively for decades and modern CPUs/GPUs already have the best implementations.

SUB, XOR are never executed with the same operands to prevent degradation to zero. ADD is defined as a 3-way operation with random constant to fix trailing zero bits that tend to accumulate after multiplications.

Code generator ensures that minimal required latency for ASIC to execute random math is at least **3 times higher** than what was needed for DIV+SQRT in CryptonightV2: current settings ensure latency equivalent to a chain of 18 multiplications while optimal ASIC implementation of DIV+SQRT has latency equivalent to a chain of 6 multiplications.

It also accounts for super-scalar and out of order CPUs which can execute more than 1 instruction per clock cycle. If ASIC implements random math circuit as simple in-order pipeline, it'll be hit with **further 1.5x slowdown**.

### Performance on CPU/GPU and ASIC

CryptonightR parameters were chosen to have the same hashrate as CryptonightV2 on CPU/GPU.

ASIC will have to implement some simple and minimalistic instruction decoder and execution pipeline. While it's not impossible, it's much harder to create efficient out of order pipeline which can track all data dependencies and do more than 1 instruction per cycle. It will also have to use fixed clock cycle length, just like CPU, so for example XOR (single logic gate) won't be much faster anymore.

ASIC with external memory will have the same performance as they did on CryptonightV2, but they will require much more chip area to implement multiple CPU-like execution pipelines.
ASIC with on-chip memory will get 3-4.5 times slower due to increased math latency and randomness and they will also require more chip area.

### Further development plans

- Reference implementation in Monero's code base (`slow-hash.c`, dependent files and tests): December 9th, 2018
- Optimized CPU miner (xmrig): December 15th, 2018
- Optimized GPU miner (xmrig-amd): December 20th, 2018
- Pool software: December 24th, 2018
- Public testing: January 2019
