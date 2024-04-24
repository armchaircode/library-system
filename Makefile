configure:
	cmake -S . -B ./build -G Ninja
build:
	cmake --build ./build
clear:
	rm -rf build
all:
	$(MAKE) clear
	$(MAKE) configure
	$(MAKE) build
