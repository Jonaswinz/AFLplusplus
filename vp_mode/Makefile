.PHONY: all vps harness clean

VP_DIR := VPs
VP_FOLDERS := $(wildcard $(VP_DIR)/*)

all: vps harness

vps:
	@for dir in $(VP_FOLDERS); do \
		echo "Building $$dir..."; \
		mkdir -p $$dir/build $$dir/install; \
		cmake -S $$dir -B $$dir/build -DCMAKE_INSTALL_PREFIX=$$dir/install; \
		$(MAKE) -C $$dir/build; \
		$(MAKE) -C $$dir/build install; \
	done

harness:
	@echo "Building harness"
	@mkdir -p harness/build
	@cd harness/build && cmake .. && make

clean:
	@echo "Cleaning all VP builds"
	@for dir in $(VP_FOLDERS); do \
		if [ -d "$$dir/build" ]; then $(MAKE) -C $$dir/build clean; fi; \
		rm -rf $$dir/build $$dir/install; \
	done
	@echo "Cleaning harness build"
	@if [ -d "harness/build" ]; then cd harness/build && make clean && cd ../..; fi
	@rm -rf harness/build
