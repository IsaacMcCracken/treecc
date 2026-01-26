build_args="build.c -o compiler -I include -I src -g -lm -lpthread  -ldl"
gcc $build_args
./compiler test/unittest
