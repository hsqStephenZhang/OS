#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc,char* argv[])
{
    int i;
    char buffer[]="00006";
    i = atoi (buffer);
    printf ("The value entered is %d", i);
    return 0;
}