build_args = build.c -o compiler -I include -g
clang $build_args | gcc $build_args 
./compiler some.c
