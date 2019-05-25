#include <stdio.h>

int main()
{
  int d, count = 0;
  FILE *I = fopen("out.bin", "rb");
  FILE *O = fopen("out.c", "w");

  while ((d = fgetc(I)) != -1)
  {
    fprintf(O, "VM->Memory.DmemSetByte(0x%.2X); ", d);
    count++;
    if ((count % 4) == 0)
    {
      fprintf(O, "\n");
    }
  }
  fclose(O);
  fclose(I);
  return 0;
}
