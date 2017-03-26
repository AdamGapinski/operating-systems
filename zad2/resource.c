#include <stdlib.h>

int main() {
    for (int i = 0; i < 100000; ++i) {
        int *tab = malloc(100);
        for (int j = 0; j < 100; ++j) {
            tab[j] = 34;
        }
    }
}