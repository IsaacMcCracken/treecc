build_args="build.c core/build_core.c -o compiler -I include -g"
clang $build_args | gcc $build_args
./compiler test/ifchains.c
