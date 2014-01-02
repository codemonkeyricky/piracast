#include <stdio.h>

int main()
{
    unsigned int i; 


    for(i = 0; i < 11379; i++)
    {
        printf("{ peer0_%d, sizeof(peer0_%d) }, \n", i, i); 
    }
}
