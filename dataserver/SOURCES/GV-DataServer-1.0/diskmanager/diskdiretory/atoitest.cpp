#include "tbsys.h"
#include "stdio.h"

using namespace tfs;
int main()
{
    uint64_t num;
    if(atoi64("12345678912345678912",20,num) < 0) 
        printf("atoi64 error\n");
    else
        printf("atoi64 num:%llu\n",num);
        
    uint64_t num1 = 12345678912345678912;//max 20 bytes
    uint32_t num2 = 1234567891;          //max 10 bytes
    char str1[21];
    char combinestr[32];
    itoa64(num1,str1,10);
    printf("itoa,num1 char:%s\n",str1);
    combine_itoa(num1,num2,combinestr,10);  
    printf("itoa,combine num:%Lu and %u,result %s\n",num1,num2,combinestr);
    return 0;
}