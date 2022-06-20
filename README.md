# RazerSteelseriesWave
a C implementation of my wave effect

# Building
```shell
mkdir build
cd build
cmake ..
make
```


## Dependencies 
- DBus
- [RazerInterface](https://github.com/Coolio4691/RazerInterface)
- HIDAPI

### Bugs
Might have a few memory leaks & bugs (which I will try to fix.)

### Setup
Requires password.h in src/include with content 
```c
static char* sysPass = "password";
```
This is unsafe but it is required for toggling VM

### TODO
- Virtual Machine checker.