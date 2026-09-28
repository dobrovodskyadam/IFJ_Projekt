// @file test_file.c
// @brief Test file for deciding optimal formating and conventions
// @date 22-09-2026
//
// @author Lukas Sofka (xsofkal00)
// @author Email: xsofkal00@stud.fit.vutbr.cz
// Faculty of Information Technology, Brno University of Technology

#include <stdio.h>

// Debug macro (possible layout)
// Possible to place in standalone file
#ifdef DEBUG
    #ifdef DEBUG_MSG
        #define DEBUG_PRINT(fn_args, ...)                                                     \
            fprintf(stderr, "[DEBUG] %s:%d (%s): Message: " fn_args "\n", __FILE__, __LINE__, \
                    __func__, ##__VA_ARGS__)
    #else
        #define DEBUG_PRINT(fn_args, ...) \
            fprintf(stderr, "[DEBUG] %s:%d (%s) \n", __FILE__, __LINE__, __func__)
    #endif
#else
    #define DEBUG_PRINT(fn_args, ...) \
        do {                          \
        } while (0)
#endif


int addition(int a, int b) {
    DEBUG_PRINT("Entering addition with arguments: a=%d, b=%d", a, b);
    return a + b;
}


int main(void) {
    int result = addition(5, 3);

    DEBUG_PRINT("Result: %d", result);

    printf("Result: %d\n", result);
    printf("Hello, World!\n");
    return 0;
}

// End of file
