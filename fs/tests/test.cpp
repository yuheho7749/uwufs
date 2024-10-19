#include <cstdio>

int main() {
    // open the file
    FILE* file = fopen("../mnt/test.txt", "w");
    if (file == nullptr) {
        perror("fopen");
        return 1;
    }
    return 0;
}