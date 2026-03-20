// Compiled with: cc -o simple simple.c
// on macOS (Mach-O output)

#include <stdlib.h>

int add(int a, int b) { return a + b; }
void die(int code) { exit(code); }
int main(void) { return add(1, 2); }
