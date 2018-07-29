#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "bbcopenup.h"

int read_int_data(FILE *in)
{
   int byte=0, scrap=0;

   scrap=fgetc(in);
   if (scrap != 0x40) { fprintf(stderr,"Cannot find an int type when expecting one!\n"); return -1; }

   byte=fgetc(in)<<24;
   byte+=fgetc(in)<<16;
   byte+=fgetc(in)<<8;
   byte+=fgetc(in);
   return byte;
}

char *read_char_data(FILE *in, char *result)
{
   char strscrap[255];
   int scrap, length, i;
   memcpy(strscrap,"\0",255);

   scrap=fgetc(in);
   if (scrap != 0x00) { fprintf(stderr,"Cannot find a char type when expecting one! %x\n",ftell(in)); return NULL; }

   length=fgetc(in);
   if (length == 0)
   {
      result[0]='\0';
      return result;
   }
   fread(strscrap, length, 1, in);

   for (i=0; i<length; i++)
   {
      result[i]=strscrap[length-i-1];
   }
   result[length]='\0';
   return result;
}

int twospower(int number)
{
   int i,result=2;
   
   for (i=1; i < number; i++)
   {
      result*=2;
   }
   
   return result;
}

double read_float_data(FILE *in)
{
   double result, divisor=1;
   int exponent, scrap, mantissa, i, j;

   scrap=fgetc(in);
   if (scrap != 0xff) { fprintf(stderr,"Cannot find a float type when expecting one!\n"); return -1; }
   
   mantissa=fgetc(in);
   mantissa+=fgetc(in)<<8;
   mantissa+=fgetc(in)<<16;
   mantissa+=fgetc(in)<<24;  
   exponent=fgetc(in);
   
   if (exponent == 0) return 0;
   exponent -= 0x80;
   
   // Now convert exponent & mantissa to a float
   result=0.5;
   for (i=1; i < 32; i++)
   {
      j=twospower(32-i);
      if (mantissa & j)
      {
         result += (divisor/twospower(i));
      }
   }
   
   // Now times by exponent until we finish
   while (exponent > 0)
   {
      result=result*2;
      exponent--;
   }

   return result;
}
