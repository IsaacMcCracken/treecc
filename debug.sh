build_args="build.c -o c4c -I include -I src -g -lm -lpthread  -ldl"
gcc $build_args
gdb --args ./c4c test/holyf
r
n
