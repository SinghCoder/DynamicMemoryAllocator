#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#define ALLOC_SIZE 48

void print_array(int *arr, int sz)
{
    fprintf(stderr, "Array contents: [");
    for(int i = 0; i < sz; i++)
    {
        fprintf(stderr, "%d", arr[i]);
        if(i != sz-1){
            fprintf(stderr, ", ");
        }
    }
    fprintf(stderr, "]\n");
}

int main(int argc, char *argv[])
{
    int *p1 = (int *)malloc(sizeof(int) * 5);
    p1[0] = 0, p1[1] = 1, p1[2] = 2, p1[3] = 3, p1[4] = 4;
    print_array(p1, 5);
    free(p1);
    p1 = calloc(5, sizeof(int));
    print_array(p1, 5);
    p1 = realloc(p1, sizeof(int) * 2);
    print_array(p1, 2);
    free(p1);
    p1 = malloc(sizeof(int) * 1500);
    free(p1);
    return 0;
}
