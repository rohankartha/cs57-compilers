# Macros
OPTIMIZER = optimizer.c
VALGRIND = valgrind -s --leak-check=full --show-leak-kinds=all
MINIC = test.c

# Generating main executable
optimizer: 
	gcc -g -I /usr/include/llvm-c-15 -c optimizations.c -o optimizations.o
	gcc -g -I /usr/include/llvm-c-15 -c optimizer.c -o optimizer.o
	g++ optimizations.o optimizer.o `llvm-config-15 --cxxflags --ldflags --libs core` -I /usr/include/llvm-c-15 -o optimizer

# Generating LLVM IR code from mini-c file
input: $(MINIC)
	clang -S -emit-llvm $(MINIC) -o llvm_output.ll

# Clean directory
clean:
	rm -rf optimizer optimizer.o