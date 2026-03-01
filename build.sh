build_args="build.c -o c4c -I include -I src -g -lm -lpthread  -ldl"
gcc $build_args
./c4c test/branches
