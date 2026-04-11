#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: custom_rm <file>\n");
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        if (remove(argv[i]) == 0) {
            printf("Deleted: %s\n", argv[i]);
        } else {
            printf("Error deleting: %s\n", argv[i]);
        }
    }

    return 0;
}
