configure:
	cmake -DSQLITECPP_RUN_CPPLINT:BOOL=OFF -S . -B ./build -G Ninja
compile:
	cmake --build ./build
clear:
	rm -rf build
all:
	$(MAKE) clear
	$(MAKE) configure
	$(MAKE) compile
