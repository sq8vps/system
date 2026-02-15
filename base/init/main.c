#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "tty.h"

int main(int argc, char **argv)
{
    if(!OpenVt(argc, argv))
    {

    }

    puts("Hello world from init\n");

    char t[32];

    while(1)
    {
        fgets(t, sizeof(t) - 1, stdin);
        puts(t);
    }

    return 0;
}