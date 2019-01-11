# CryptonightR 

This is proof of concept repository. It introduces random integer math into CryptonightV2 main loop.

### Random integer math modification

Division and square root are replaced with a sequence of random integer instructions:

OP|Description|Frequency|Comment
--|-----------|---------|-------
MUL|a\*b|40.05%|Many multiplications ensure high latency
ADD|a+b+C|11.88%|3-way addition with random constant
SUB|a-b|12.21%|b is always different from a
ROR|ror(a,b)|7.52%|Bit rotate right
ROL|rol(a,b)|5.57%|Bit rotate left
XOR|a^b|22.78%|b is always different from a

Program size is between 60 and 69 instructions, 63 instructions on average.

There are 8 registers named R0-R7. Registers R0-R3 are variable, registers R4-R7 are constant and can only be used as source register in each instruction. Registers R4-R7 are initialized with values from main loop registers on every main loop iteration.

All registers are 32 bit to enable efficient GPU implementation.

The random sequence changes every block. Block height is used as a seed for random number generator. This allows CPU/GPU miners to precompile optimized code for each block. It also allows to verify optimized code for all future blocks against reference implementation, so it'll be guaranteed safe to use in Monero daemon/wallet software.

Reference implementation (variant4 code in the following files):
- https://github.com/SChernykh/slow_hash_test/blob/CryptonightR/variant4_random_math.h
- https://github.com/SChernykh/slow_hash_test/blob/CryptonightR/slow-hash.c

Reference implementation in Monero code base:
- https://github.com/SChernykh/monero/tree/CryptonightR

An example of generated random math:
- https://github.com/SChernykh/CryptonightR/blob/master/CryptonightR/random_math.inl

Optimized CPU miner:
- [xmrig](https://github.com/SChernykh/xmrig/tree/CryptonightR)

Optimized GPU miner:
- [xmrig-amd](https://github.com/SChernykh/xmrig-amd/tree/CryptonightR)
- [xmrig-nvidia](https://github.com/SChernykh/xmrig-nvidia/tree/CryptonightR)

Pool software:
- [node-multi-hashing](https://github.com/SChernykh/node-multi-hashing)
- [node-cryptonight-hashing](https://github.com/SChernykh/node-cryptonight-hashing/tree/CryptonightR)
- [nodejs-pool mod example](https://github.com/wowario/nodejs-pool/commit/3b3838a8703d43932cc85897ab2b77a78012be41)

**Test pools:**
- https://testnet.wowne.ro/

### Design choices

Instruction set is chosen from instructions that are efficient on CPUs/GPUs compared to ASIC: all of them except XOR are complex operations at logic circuit level and require O(logN) gate delay. These operations have been studied extensively for decades and modern CPUs/GPUs already have the best implementations.

SUB, XOR are never executed with the same operands to prevent degradation to zero. ADD is defined as a 3-way operation with random 32-bit constant to fix trailing zero bits that tend to accumulate after multiplications.

Code generator ensures that minimal required latency for ASIC to execute random math is at least **2.5 times higher** than what was needed for DIV+SQRT in CryptonightV2: current settings ensure latency equivalent to a chain of 15 multiplications while optimal ASIC implementation of DIV+SQRT has latency equivalent to a chain of 6 multiplications.

A number of simple checks is implemented to prevent algorithmic optimizations of the generated code. Current instruction mix also helps to prevent algebraic optimizations of the code. My tests show that generated C++ code compiled with all optimizations on is only 5% faster on average than direct translation to x86 machine code - this is synthetic test with only random math in the loop, but the actual Cryptonight loop is still dominated by memory access, so these possible optimizations are only needed to estimate possible gains ASIC.

It also accounts for super-scalar and out of order CPUs which can execute more than 1 instruction per clock cycle. If ASIC implements random math circuit as simple in-order pipeline, it'll be hit with **further 1.5x slowdown**.

### Performance on CPU/GPU and ASIC

CryptonightR parameters were chosen to:
- have the same hashrate as CryptonightV2 on CPU/GPU
- have a bit smaller power consumption on CPU/GPU

Actual numbers (hashrate and power consumption for different CPUs and GPUs) will be available in January 2019.

ASIC will have to implement some simple and minimalistic instruction decoder and execution pipeline. While it's not impossible, it's much harder to create efficient out of order pipeline which can track all data dependencies and do more than 1 instruction per cycle. It will also have to use fixed clock cycle length, just like CPU, so for example XOR (single logic gate) won't be much faster anymore.

ASIC with external memory will have the same performance as they did on CryptonightV2, but they will require much more chip area to implement multiple CPU-like execution pipelines.
ASIC with on-chip memory will get 2.5-3.75 times slower due to increased math latency and randomness and they will also require more chip area.

### Further development plans

- Public testing: [first round is over](https://github.com/SChernykh/CryptonightR/issues/2#issuecomment-453193397), second round will start on January 14th, 2019
