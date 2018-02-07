#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned char magic[] = { 0xc3, 0xe8, 0xe1, 0xfa, 0xca, 0xe5, 0xfa, 0xca, 0xe9, 0xed, 0xc9, 0xe1, 0xee, 0x01, 0x00, 0x00 };

int main(int argc, char **argv)
{
   FILE *inhandle;
   int i;
   unsigned char byte;   
   
   inhandle=fopen(argv[1], "rb");
   
   // read magic
   
   for (i=0; i<16;i++)
   {
      byte=fgetc(inhandle);
      //if (byte != magic[i])
      //{
      //  printf("Not an Interword file: magic doesnt match\n");
      //   exit(1);
      //}
   }
   
   // Now read information
   int textstart;
   fseek(inhandle,0x32,SEEK_SET);
   textstart=fgetc(inhandle)+(fgetc(inhandle)<<8);

 #ifdef DEBUG
   fseek(inhandle,0x43,SEEK_SET);
   byte=fgetc(inhandle);
   printf("Cursor Line: %d\n",byte);
   byte=fgetc(inhandle);
   printf("Number of Lines: %d\n",byte);
   byte=fgetc(inhandle);
   printf("Number of Columns: %d\n",byte);
   byte=fgetc(inhandle);
   printf("Unknown: %d\n",byte);
   byte=fgetc(inhandle);
   printf("Flags: ",byte);
   if (byte&0x40) printf("show_line_numbers ");
   if (byte&0x02) printf("show_print_codes ");
   if (byte&0x01) printf("interlace ");
   printf("\n");
   byte=fgetc(inhandle);
   printf("Unknown: %d\n",byte);
   byte=fgetc(inhandle);
   printf("Unknown: %d\n",byte);
   byte=fgetc(inhandle);
   printf("Unknown: %d\n",byte);
   byte=fgetc(inhandle);
   printf("Foreground Colour: %d\n",byte);
   byte=fgetc(inhandle);
   printf("Background Colour: %d\n",byte);
   byte=fgetc(inhandle);
   printf("Unknown: %d\n",byte);
   byte=fgetc(inhandle);
   printf("Keyboard repeat rate: %d\n",byte);
   byte=fgetc(inhandle);
   printf("Keyboard repeat delay: %d\n",byte);
#endif
 
   fseek(inhandle,textstart,SEEK_SET);
   int ruler[255];
   unsigned char pos=0;
   for (i=0;i<150;i++) ruler[i]=0;
   char underline,bold,italic,centre,right,justify;   
   while (!feof(inhandle))
   {
      byte=fgetc(inhandle);
      byte&=0x7f;
      if (byte < 32 || byte == 0x7f)
      {
         switch (byte)
         {
            case 0x0d:
            {
               // reset to nowt special
               if (italic==1) { printf("</em>"); italic=0; }
               if (centre==1) { printf("</center>"); centre=0; }
               if (right==1) { printf("</right>"); right=0; }
               if (justify==1) { printf("</justify>"); justify=0; }
               printf("\n");
               pos=0;
               break;
            }
               
            case 0x05:
            {
               // In ruler
               fgetc(inhandle);
               // copy ruler to array rule, 0 = nowt 1 = LM, 2 = RM, 3= RetM, 4 = Tab
               ruler[fgetc(inhandle)]=1;
               ruler[fgetc(inhandle)]=2;
               ruler[fgetc(inhandle)]=3;
               int tab=0;
               do
               {
                  tab=fgetc(inhandle);
                  if (tab != 0x87)
                     ruler[tab]=4;
               } while (tab != 0x87);
               break;
            }
            case 0x09:
            {
               /*for(i=pos;i<106;i++)
               {
                  if (ruler[i]==4)
                  {
                     break;
                  }
                  else
                  {
                     printf(" ");
                  }
               }*/
               printf("\t");               
               break;
            }
            case 0x0b:
            {
               printf("<strong>");
               bold=1;
               break;
            }
            case 0x0c:
            {
               printf("<u>");
               underline=1;
               break;
            }
            case 0x13:
            {
               if (bold==1) { printf("</strong>"); bold=0; }
               break;
            }            
            case 0x14:
            {
               if (underline==1) { printf("</u>"); underline=0; }
               break;
            }
            
            default:
               break;
         }
      }
      else
      {
         printf("%c",byte);
         pos++;
      }
   }         
   fclose(inhandle);
}
   
   