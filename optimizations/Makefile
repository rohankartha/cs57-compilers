# Macros
OPTIMIZER = optimizer.c
VALGRIND = valgrind -s --leak-check=full --show-leak-kinds=all --track-origins=yes

# Generating main executable
optimizer: 
	g++ -g -I /usr/include/llvm-c-15 -c optimizations.c -o optimizations.o
	g++ -g -I /usr/include/llvm-c-15 -c optimizer.c -o optimizer.o
	g++ -g optimizations.o optimizer.o `llvm-config-15 --cxxflags --ldflags --libs core` -I /usr/include/llvm-c-15 -o optimizer

# Basic testing
test: 
	./optimizer optimizer_test_results/p5_const_prop.ll > testing.out

# Checking for memory leaks
valgrind:
	$(VALGRIND) ./optimizer optimizer_test_results/p3_const_prop.ll

# Clean directory
clean:
	rm -rf optimizer optimizer.o testing.out optimizations.o
