
#include <stdio.h>
#include <stdlib.h>

int main(){
    int *my_bad_ptr = (int *)0x12345; //malloc(sizeof(int)); 

    printf("About to set a pointer at location %p with a value of 42\n", my_bad_ptr);

    *my_bad_ptr = 42;

    printf("I just set a pointer at memory address %p to the value of 42\n",my_bad_ptr);

    //free(my_bad_ptr);
}