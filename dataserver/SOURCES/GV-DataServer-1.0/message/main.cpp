#include "ClientMessage.h"

int main()
{
    ClientPut* M_Put = (ClientPut*) malloc(sizeof(struct ClientPut) + 
                                            MAXBACKUPSERVER*sizeof(address_t));
    ClientGet M_Get;
    delete M_Put;
}