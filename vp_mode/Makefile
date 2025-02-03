.PHONY: all avp64 harness clean

all: avp64 harness

avp64:
	@echo "Building Avp64"
	@cd avp64 && mkdir -p build && mkdir -p install && cd build && cmake -DCMAKE_INSTALL_PREFIX=../install .. && make && make install

harness:
	@echo "Building Avp64 harness"
	@cd harness && mkdir -p build && cd build && cmake .. && make

clean:
	@echo "Cleaning Avp64 build directory"
	@if [ -d "avp64/build" ]; then cd avp64/build && make clean && cd ../..; fi
	@echo "Cleaning Avp64 harness build directory"
	@if [ -d "harness/build" ]; then cd harness/build && make clean && cd ../..; fi
	@echo "Removing build directories"
	@rm -rf avp64/build harness/build
	@echo "Removing avp64 install directory"
	@rm -rf avp64/install