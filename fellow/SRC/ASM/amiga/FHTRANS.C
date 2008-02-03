#include <stdio.h>

FILE *I,*O;

void main(void) {
      int j;
        I=fopen("fhfile.cod","rb");
        O=fopen("fhfileou","wb");

        for (j=0; j < 194; j += 2) {
          fprintf(O,"dsetb(0x%.2X); ",fgetc(I));
          fprintf(O,"dsetb(0x%.2X);\n",fgetc(I));
	}
       fclose(O);
       fclose(I);
}
