#include "print.h"
#include "init.h"
#include "fs.h"
#include "stdio.h"

int main(void)
{
   put_str("I am kernel\n");
   init_all();
   uint32_t fd = sys_open("/file1", O_RDONLY);
   printf("fd:%d\n", fd);
   sys_close(fd);
   printf("%d closed now\n", fd);
   while (1)
      ;
   return 0;
}
