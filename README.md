# CryptonightR 

This is proof of concept repository and a proposal for the next Monero PoW. It introduces random integer math into CryptonightV2 main loop.

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

There are 9 registers named R0-R8. Registers R0-R3 are variable, registers R4-R8 are constant and can only be used as source register in each instruction. Registers R4-R8 are initialized with values from main loop registers on every main loop iteration.

All registers are 32 bit to enable efficient GPU implementation. It's possible to make registers 64 bit though - it's supported in miners below.

The random sequence changes every block. Block height is used as a seed for random number generator. This allows CPU/GPU miners to precompile optimized code for each block. It also allows to verify optimized code for all future blocks against reference implementation, so it'll be guaranteed safe to use in Monero daemon/wallet software.

Reference implementation (variant4 code in the following files):
- https://github.com/SChernykh/slow_hash_test/blob/CryptonightR/variant4_random_math.h
- https://github.com/SChernykh/slow_hash_test/blob/CryptonightR/slow-hash.c

Reference implementation in Monero code base:
- https://github.com/moneromooo-monero/bitmonero/tree/magic-beans-0.14

An example of generated random math:
- https://github.com/SChernykh/CryptonightR/blob/master/CryptonightR/random_math.inl

Optimized CPU miner:
- [xmrig](https://github.com/SChernykh/xmrig/tree/CryptonightR)

Optimized GPU miner:
- [xmrig-amd](https://github.com/SChernykh/xmrig-amd/tree/CryptonightR)
- [xmrig-nvidia](https://github.com/SChernykh/xmrig-nvidia/tree/CryptonightR)

Pool software:
- [node-multi-hashing](https://github.com/SChernykh/node-multi-hashing/tree/feature-cryptonight_r)
- [node-cryptonight-hashing](https://github.com/SChernykh/node-cryptonight-hashing/tree/CryptonightR)
- [nodejs-pool example](https://github.com/wowario/nodejs-pool)

**NOTE for pool operators: you'll have to pass active block height with each miner job and use correct height when checking submitted shares, don't forget that stale shares use older block height**

**Test pools:**
- http://killallasics.moneroworld.com/

### Design choices

Instruction set is chosen from instructions that are efficient on CPUs/GPUs compared to ASIC: all of them except XOR are complex operations at logic circuit level and require O(logN) gate delay. These operations have been studied extensively for decades and modern CPUs/GPUs already have the best implementations.

SUB, XOR are never executed with the same operands to prevent degradation to zero. ADD is defined as a 3-way operation with random 32-bit constant to fix trailing zero bits that tend to accumulate after multiplications.

Code generator ensures that minimal required latency for ASIC to execute random math is at least **2.5 times higher** than what was needed for DIV+SQRT in CryptonightV2: current settings ensure latency equivalent to a chain of 15 multiplications while optimal ASIC implementation of DIV+SQRT has latency equivalent to a chain of 6 multiplications.

It also accounts for super-scalar and out of order CPUs which can execute more than 1 instruction per clock cycle. If ASIC implements random math circuit as simple in-order pipeline, it'll be hit with **further up to 1.5x slowdown**.

A number of simple checks is implemented to prevent algorithmic optimizations of the generated code. Current instruction mix also helps to prevent algebraic optimizations of the code. My tests show that generated C++ code compiled with all optimizations on is only 5% faster on average than direct translation to x86 machine code - this is synthetic test with just random math in the loop, but the actual Cryptonight loop is still dominated by memory access, so this number is needed to estimate the limits of possible gains for ASIC.

### Random code generator description

There are several partially contradicting requirements for the generated code:
- It should be completely random
- It should have at least 1 long dependency chain to satisfy minimal latency requirement for ASIC with unlimited parallel execution resources
- It shouldn't be significantly slower on CPU with limited amount of ALUs of which only 1 ALU can do multiplications
- It shouldn't allow significant compile-time optimizations

Current code generator implementation tries to achieve the last 3 requirements by sacrificing code randomness: some instructions are discarded, some other are replaced when needed.

Repeated Blake256 hashing is used to generate a random sequence of bytes for the code generator. Instruction encoding rules are such that any sequence of bytes translates to valid sequence of instructions.

Instruction length can be from 1 to 5 bytes. Byte 0 defines opcode (3 bits), destination (2 bits) and source (3 bits) registers: MUL (opcodes 0-2), ADD (opcode 3), SUB (opcode 4), ROR/ROL (opcode 5), XOR (opcodes 6-7). MUL, SUB and XOR instructions are 1 byte long. Rotation instructions are 2 bytes long, they share the same opcode: ROR is selected if the next byte is non-negative (bit 7 is 0), ROL is selected when the next byte is negative (bit 7 is 1). ADD instruction is 5 bytes long, bytes 1-4 are 32-bit unsigned constant C encoded in little-endian order.

Code generator keeps track of required latency to execute the code generated so far, both for CPU and ASIC. It assumes that ASIC has unlimited parallel execution resources and CPU has 3 parallel ALUs of which only 1 ALU can do multiplications. It stops as soon as there is long enough dependency chain in the code to ensure required latency for ASIC. It also tries to generate instruction sequence that doesn't overload CPU with its limited execution resources.

A number of checks are implemented to discard instruction sequences that can be optimized during compilation:
- ADD/SUB/XOR with the same source and destination registers: SUB(x,x)/XOR(x,x) can be optimized away completely, ADD(x,x) can be replaced with logic shift. Such instructions are replaced with ADD(x,R8)/SUB(x,R8)/XOR(x,R8) - source register is changed to R8.
- Sequence of two rotations with the same destination register can be optimized to single rotation and is discarded
- Sequence of two ADD/SUB/XOR instructions when source register is the same can be optimized to 1 ADD/SUB and 1 logic shift; XOR->XOR with the same register can be optimized away; it's also discarded
- Some instructions are discarded if there are no available ALUs left for CPU
- Some instructions are discarded if they reduce dependency chain length (some register is left unchanged for more than 7 cycles)

Instruction selection mix (40% MUL, 23% XOR) combined with these checks ensures very little possibility of compile-time optimizations:
- it's not possible to optimize consecutive multiplications (unlike consecutive ADD/SUB)
- relatively frequent non-arithmetic operation (XOR) reduces chances for arithmetic optimizations.
- most remaining cases where optimizations are possible are handled in code generator.

This approach results in only ~5% faster code generated by compiler vs direct translation to x86 code.

### The second tweak from @vtnerd

* The values used in the first shuffle operation are now used to modify the selection of the second memory address.
* The values used in the second shuffle operation are now used to modify the input to the next AES operation (next loop iteration).
* The output (destination registers) of the random program are used in the writeback value of the second memory address and the memory address calculation for the next AES operation (next loop iteration).
* Tweak2-2 was dropped.
 
The operations done are fairly simple, but AFAIK cannot be optimized out. They enforce a stronger order of operations.


### Performance on CPU/GPU and ASIC

CryptonightR parameters were chosen to:
- have the same hashrate as CryptonightV2 on CPU/GPU
- have a bit smaller power consumption on CPU/GPU

Actual numbers (hashrate and power consumption for different CPUs and GPUs) will be available in January 2019.

ASIC will have to implement some simple and minimalistic instruction decoder and execution pipeline. While it's not impossible, it's much harder to create efficient out of order pipeline which can track all data dependencies and do more than 1 instruction per cycle. It will also have to use fixed clock cycle length, just like CPU, so for example XOR (single logic gate) won't be much faster anymore.

ASIC with external memory will have the same performance as they did on CryptonightV2, but they will require much more chip area to implement multiple CPU-like execution pipelines.
ASIC with on-chip memory will get 2.5-3.75 times slower due to increased math latency and randomness and they will also require more chip area.

