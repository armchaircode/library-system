## Library Management System

### Build
Make sure you have these installed.
- cmake
- ninja

First clone this repo
```bash
git clone --recursive-submodules https://github.com/armchaircode/library-system.git
cd library-system
```
Afterwards, if you have ```make``` installed, do the following.
```bash
make configure
make compile
```

On systems without make
```bash
cmake -S . -B ./build -G Ninja
cmake --build ./build
```
