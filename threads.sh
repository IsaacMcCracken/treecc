clang -g -O0 test/multithread.c core/build_core.c -I include/ -lpthread -o threads
./threads
