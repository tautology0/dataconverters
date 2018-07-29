#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "bbcopenup.h"

int main(int argc, char **argv)
{
   FILE *infile;
   int byte,intvar;
   char string[255];
   double floatvar;
   if (argc != 2)
   {
      printf("Filename not provided\n");
      exit(1);
   }
   
   infile=fopen(argv[1],"rb");
   if (infile == NULL)
   {
      printf("Could not open file %s\n",argv[1]);
      exit(1);
   }
   
   while (!feof(infile))
   {
      byte=fgetc(infile);
      switch(byte)
      {
         case 0x40:
            // Is an integer
            fseek(infile,-1,SEEK_CUR);
            intvar=read_int_data(infile);
            printf("Integer: %d\n",intvar);
            break;
            
         case 0xff:
            // Is a float
            fseek(infile,-1,SEEK_CUR);
            floatvar=read_float_data(infile);
            printf("Float: %f\n",floatvar);
            break;
            
         case 0x0:
            // Is a string
            fseek(infile,-1,SEEK_CUR);
            read_char_data(infile, string);
            printf("String: %s\n",string);
            break;
         
         default:
            printf("Unknown: %x\n",byte);
            break;
      }
   }
   
   fclose(infile);
}
