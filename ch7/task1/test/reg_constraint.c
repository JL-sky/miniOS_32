#include<stdio.h>
int main(){
    int in_a=1,in_b=2,out_sum;
    asm("addl %%ebx,%%eax":"=a"(out_sum):"a"(in_a),"b"(in_b));
    printf("sum is %d\n",out_sum);
    asm("addl %%ebx,%%eax":"+a"(in_a):"b"(in_b));
    printf("in_a after add is %d\n",in_a);
    return 0;
}
