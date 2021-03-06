#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <png.h>
#include "riscosmodes.h"
#include "riscoscolpal.h"

#define OBJECT_FONTTABLE   0
#define OBJECT_TEXT        1
#define OBJECT_PATH        2
#define OBJECT_SPRITE      5
#define OBJECT_GROUP       6
#define OBJECT_TAGGED      7
#define OBJECT_TEXTAREA    9
#define OBJECT_TEXTCOLUMN  10
#define OBJECT_OPTIONS     11
#define OBJECT_TRANSTEXT   12
#define OBJECT_TRANSSPRITE 13

#define PATH_END           0
#define PATH_MOVE          2
#define PATH_CLOSE_SUB     5
#define PATH_BEZIER        6
#define PATH_DRAW          8

#define MAX_FONTS 255
#define FONTNAME_SIZE 255

typedef struct
{
   signed int x;
   signed int y;
} coords;

typedef struct
{
   unsigned char reserved;
   unsigned char red;
   unsigned char green;
   unsigned char blue;
} colourtype;

typedef struct
{
   unsigned int joinstyle:2;
   unsigned int endcapstyle:2;
   unsigned int startcapstyle:2;
   unsigned int winding:1;
   unsigned int dash:1;
   unsigned int reserved:8;
   unsigned int tricapwidth:8;
   unsigned int tricaplength:8;
} pathstyletype;

typedef struct
{
   unsigned int number;
   char *originalname;
   char *name;
   char *weight;
   char *style;
} fonttable;

typedef struct
{
   unsigned int magic;
   unsigned int major;
   unsigned int minor;
   char creator[11];
   coords low;
   coords high;
} drawheader;

typedef struct
{
   unsigned int type;
   unsigned int length;
   coords low;
   coords high;
} objectheader;

typedef struct
{
   colourtype colour;
   unsigned int bgcolourhint;
   unsigned int style;
   unsigned int xsize;
   unsigned int ysize;
   coords baseline;
} textheader;

typedef struct
{
   colourtype fillcolour;
   colourtype outlinecolour;
   unsigned int outlinewidth;
   pathstyletype style;
} pathheader;

typedef struct
{
   unsigned int a;
   unsigned int b;
   unsigned int c;
   unsigned int d;
   unsigned int e;
   unsigned int f;
} transmatrix;

typedef struct
{
   unsigned int nextsprite;
   unsigned char name[12];
   unsigned int width;
   unsigned int height;
   unsigned int firstbit;
   unsigned int lastbit;
   unsigned int image;
   unsigned int mask;
   unsigned int mode;
} spritectrlblock;

static void hexdump(const void * memory, size_t bytes)
{
   const unsigned char * p, * q;
   int i;
   printf("   ");
   p = memory;
   while (bytes) 
   {
      q = p;
      printf("%p: ", (void *) p);
      for (i = 0; i < 16 && bytes; ++i)
      {
         printf("%02X ", *p);
         ++p;
         --bytes;
      }
      bytes += i;
      while (i < 16)
      {
         printf("XX ");
         ++i;
      }
      printf("| ");
      p = q;
      for (i = 0; i < 16 && bytes; ++i)
      {
         printf("%c", isprint(*p) && !isspace(*p) ? *p : ' ');
         ++p;
         --bytes;
      }
      while (i < 16)
      {
         printf(" ");
         ++i;
      }
      printf(" |\n   ");
   }
}

char objectnames[][50] = \
   {  "Font Table", "Text", "Path", "Unknown", "Unknown", "Sprite", \
      "Group", "Tagged", "Unknown", "Text Area", "Text Column", \
      "Options", "Transformed Text", "Transformed Sprite" };

fonttable fonts[MAX_FONTS];  
int numfonts;
coords master;

// Need to define this first so we can call it
void read_objects(FILE *infile, FILE *outfile, int length);

double topt(int in)
{
   double out;
   
   // Convert to pt
   out=in/640;
   // Convert to px
   out*=1.25;
   return out;
}

void dumpmatrix(transmatrix matrix)
{
   printf("  Transformation Matrix:\n");
   printf("   %d\t%d\t0\n", matrix.a, matrix.b);
   printf("   %d\t%d\t0\n", matrix.c, matrix.d);
   printf("   %d\t%d\t0\n", matrix.e, matrix.f);
}

double converttransunits(long unit)
{
   return (double) unit / (double) (1<<16);
}

char *findfontname(unsigned int style)
{
   int i;
   for (i=0;i<numfonts;i++)
   {
      if (fonts[i].number == style) break;
   }
   return fonts[i].name;
}

char *findfontweight(unsigned int style)
{
   int i;
   for (i=0;i<numfonts;i++)
   {
      if (fonts[i].number == style) break;
   }
   return fonts[i].weight;
}

char *findfontstyle(unsigned int style)
{
   int i;
   for (i=0;i<numfonts;i++)
   {
      if (fonts[i].number == style) break;
   }
   return fonts[i].style;
}

void read_font_table_object(FILE *infile, objectheader object, int curptr)
{
   int i=1;
   unsigned char byte;
   char *charptr, *charptr2, *charptr3;

   // The font table doesn't have the bounding box stuff, so we need to work
   // from curptr
   
   fseek(infile, curptr + 8, SEEK_SET);
   
   // Add system font
   fonts[0].number=0;
   fonts[0].name="System";
   
   while (ftell(infile) < curptr+object.length)
   {
      fonts[i].number=fgetc(infile);
      fonts[i].originalname=malloc(FONTNAME_SIZE);
      fonts[i].name=malloc(FONTNAME_SIZE);
      fonts[i].weight=malloc(FONTNAME_SIZE);
      fonts[i].style=malloc(FONTNAME_SIZE);
      charptr=fonts[i].originalname;
      do
      {
         byte=fgetc(infile);
         *charptr++=byte;
      } while (byte != '\0');
      printf("  Font number %d is %s\n",fonts[i].number,fonts[i].originalname);
      // Now convert it if we can
      charptr=strchr(fonts[i].originalname,'.');
      if (charptr!=NULL)
      {
         strncpy(fonts[i].name, fonts[i].originalname, charptr-fonts[i].originalname);
         fonts[i].name[charptr-fonts[i].originalname]='\0';
         if (!strcmp(fonts[i].name,"Homerton")) strcpy(fonts[i].name,"sans-serif");
         if (!strcmp(fonts[i].name,"Trinity")) strcpy(fonts[i].name,"serif");
         if (!strcmp(fonts[i].name,"Corpus") || !strcmp(fonts[i].name,"System")) strcpy(fonts[i].name,"monospace");      
         
         charptr2=strchr(charptr+1,'.');
         if (charptr2!=NULL)
         {
            strncpy(fonts[i].weight,charptr+1,charptr2-charptr-1);
            fonts[i].weight[charptr2-charptr-1]='\0';
         }
         else
         {
            strcpy(fonts[i].weight,charptr+1);
         }
         if (!strcmp(fonts[i].weight,"Bold")) strcpy(fonts[i].weight,"bold");
         else strcpy(fonts[i].weight,"normal");
         strcpy(fonts[i].style,"normal");
         if (charptr2 != NULL)
         {
            if (!strcmp(charptr2+1,"Italic")) strcpy(fonts[i].style,"italic");
            if (!strcmp(charptr2+1,"Oblique")) strcpy(fonts[i].style,"italic");
         }
      }

      i++;
   }
   numfonts=i;
}

void read_text_object(FILE *infile, FILE *outfile, objectheader object)
{
   textheader text;
   char *text_text;
   char *text_temp;
   char *textptr;
   int text_size;
   
   text_size=object.length - sizeof(objectheader) - sizeof(textheader);
   text_text=malloc(text_size);
   fread(&text, sizeof(textheader), 1, infile);
   fread(text_text, 1, text_size, infile);

   textptr=strchr(text_text,'&');

   if (textptr!=NULL)
   {
      // We've found an orphan &
      text_temp=malloc(strlen(text_text)+5);
      strncpy(text_temp,text_text,textptr-text_text);
      strcat(text_temp,"&amp;");
      strcat(text_temp,textptr+1);
      free(text_text);
      text_text=text_temp;
   }

   printf("  Text Object: %s\n   Font: %d (%s)\n   Fontsize: %g pt x %g pt\n   Colour: (%x %x %x)\n", \
            text_text, \
            text.style, findfontname(text.style), \
            topt(text.xsize), topt(text.ysize), 
            text.colour.red, text.colour.green, text.colour.blue);
   fprintf(outfile, "<text x=\"%g\" y=\"%g\" style=\"font-family: %s; font-size: %gpt; font-weight: %s; font-style: %s; fill: #%02x%02x%02x\">%s</text>\n", \
            topt(text.baseline.x), topt(master.y-text.baseline.y), \
            findfontname(text.style), (double) (text.xsize/640)*0.90, \
            findfontweight(text.style), findfontstyle(text.style), \
            text.colour.red, text.colour.green, text.colour.blue, \
            text_text);
  free(text_text);
}

void read_path_object(FILE *infile, FILE *outfile, objectheader object)
{
   pathheader path;
   int end;
   char fill[256],stroke[256];
   
   fread(&path, sizeof(pathheader), 1, infile);
   printf("  Path Object:\n   Fill: (%x %x %x)\n   Outline: colour (%x %x %x) width %d\n", \
            path.fillcolour.red, path.fillcolour.green, path.fillcolour.blue, \
            path.outlinecolour.red, path.outlinecolour.green, path.outlinecolour.blue, path.outlinewidth);
   printf("   Style:\n    Join: %d\n    Endcap %d\n    Startcap %d\n", \
            path.style.joinstyle, path.style.endcapstyle, \
            path.style.startcapstyle);
   printf("    Winding: %d\n    Dash: %d\n    Tri cap width: %d\n    Tri cap length: %d\n", \
            path.style.winding, path.style.dash, \
            path.style.tricapwidth, path.style.tricaplength);    

   if (path.fillcolour.reserved == 0xff)
   {
      strcpy(fill,"fill=\"none\"");
   }
   else
   {
      sprintf(fill,"fill=\"#%02x%02x%02x\"", path.fillcolour.red, path.fillcolour.green, path.fillcolour.blue);
   }
   sprintf(stroke,"stroke=\"#%02x%02x%02x\"", path.outlinecolour.red, path.outlinecolour.green, path.outlinecolour.blue);
   if (path.outlinecolour.reserved == 0xff)
   {
      strcat(stroke," stroke-opacity=\"0\"");
   }

   fprintf(outfile,"<path %s fill-rule=\"%s\" %s stroke-width=\"%g\" stroke-linecap=\"%s\" stroke-linejoin=\"%s\"", \
            fill, \
            (path.style.winding==0)?"nonzero":"evenodd", \
            stroke, \
            (topt(path.outlinewidth)==0)?1:topt(path.outlinewidth), \
            (path.style.endcapstyle==0)?"butt":(path.style.endcapstyle==1)?"round":"square", \
            (path.style.joinstyle==0)?"miter":(path.style.joinstyle==1)?"round":"bevel");

   if (path.style.dash)
   {
      unsigned int offset, length, i;
      // Read dasharray
      fread(&offset, sizeof(int), 1, infile);
      fread(&length, sizeof(int), 1, infile);
      fprintf(outfile," style=\"stroke-dasharray: ");
      if (offset > 0)
      {
         fprintf(outfile, " %g",topt(offset));
      }
      printf("%g\n",offset);
      for (i=0; i<length; i++)
      {
         fread(&offset, sizeof(int), 1, infile); 
         fprintf(outfile," %g",topt(offset));
      }
      fprintf(outfile,"\"");
   }
      
   fprintf(outfile," d=\"");
   do
   {
      end=read_path_components(infile, outfile, path);
   } while (!end);
   fprintf(outfile,"\" />");
}

int read_path_components(FILE *infile, FILE *outfile, pathheader path)
{
   unsigned int tag, end=0;
   coords point;
   
   fread(&tag, sizeof(int), 1, infile);
   switch (tag & 0x7f)
   {
      case PATH_END:
         end=1;
         //fprintf(outfile,"Z");
         break;
      case PATH_MOVE:
         fread(&point, sizeof(coords), 1, infile);
         printf("     MOVE %g pt,%g pt\n", topt(point.x), topt(point.y));
         fprintf(outfile,"M%g %g\n",topt(point.x), topt(master.y-point.y));
         break;
      case PATH_CLOSE_SUB:
         fprintf(outfile,"Z");
         break;
      case PATH_DRAW:
         fread(&point, sizeof(coords), 1, infile);
         printf("     DRAW %g pt,%g pt\n", topt(point.x), topt(point.y));
         fprintf(outfile,"L%g %g\n",topt(point.x), topt(master.y-point.y));
         break;
      case PATH_BEZIER:
         fread(&point, sizeof(coords), 1, infile);
         printf("     BEZIER %g pt,%g pt,", topt(point.x), topt(point.y));
         fprintf(outfile,"C%g %g ",topt(point.x), topt(master.y-point.y));         
         fread(&point, sizeof(coords), 1, infile);
         printf("%g pt,%g pt,", topt(point.x), topt(point.y));
         fprintf(outfile,"%g %g ",topt(point.x), topt(master.y-point.y));         
         fread(&point, sizeof(coords), 1, infile);
         printf("%g pt,%g pt\n", topt(point.x), topt(point.y));
         fprintf(outfile,"%g %g\n",topt(point.x), topt(master.y-point.y));                

        break;
   }
   return end;
}

void read_group_object(FILE *infile, FILE *outfile, objectheader object)
{
   char groupname[12];
   char *chrptr;
   // read name
   fread(&groupname, 12, 1, infile);
   chrptr=strchr(groupname,' ');
   if (chrptr != NULL)
   {
      *chrptr='\0';
   }
   printf("  Group Name: %s\n",groupname);
   fprintf(outfile, "<g");
   if (strlen(groupname))
   {
      fprintf(outfile, " id=\"%s\">\n",groupname);
   }
   else
   {
      fprintf(outfile, ">\n");
   }
   // Now read objects in the group
   read_objects(infile, outfile, object.length - sizeof(objectheader) - 12);
   fprintf(outfile,"</g>\n");
}

unsigned int read_sprite(spritectrlblock sprite, unsigned char *buffer, unsigned int length)
{
   // Most of this has been stolen from Ian Jeffray 'cos I'm lazy
   unsigned int width, height, realwidth, mode, reps=1, xf, yf, bpp, ncols=256;
   unsigned int i, firstbit, lastbit;
   unsigned char *palette=buffer, *spritebits, *maskbits;
   unsigned char trans[2]={255,0};
   png_color cpal[17], *colpal;
   
   // convert width to bytes
   width=(sprite.width + 1) * 4;
   height=sprite.height + 1;
   mode=sprite.mode;
   spritebits=(unsigned char *)buffer;
   maskbits=(sprite.image==sprite.mask)?NULL:(unsigned char *)buffer+(sprite.mask);
   firstbit=sprite.firstbit;
   lastbit=sprite.lastbit;
   
   // Check for bad modes
   if (mode > 44 && mode < 256)
   {
      printf("   Cannot convert sprite as it has a custom mode. Skipping\n");
      return;
   }
   bpp=modes[mode].bpp;   
   if (bpp == 0)
   {
      printf("   Mode $d is a non-graphical mode, this is weird.\n", sprite.mode);
      return;
   }
   
   // Account for bpp 
   yf=modes[mode].yf;
   xf=modes[mode].xf;
   realwidth = width;
   if (bpp < 8)
   {
      realwidth <<= (4 - (bpp == 4)?3:bpp);
   }
   realwidth -= (31 - sprite.lastbit)/bpp;
   realwidth -= sprite.firstbit/bpp;
   
   if (yf==2)
   {
      reps = 2;
   }

   // Work out palettes
   if (bpp < 8)
   {
      int row, col, bitmask=(1 << bpp), shift=0, i;
      unsigned char *tmp, *inm, *ins;
      tmp = (unsigned char *) malloc(realwidth * height);
      ncols = (1 << bpp) + 1;
      
      colpal = (bpp==4)?colpal16:(bpp==2)?colpal4:colpal2;
      if (sprite.image > 0x2c)
      {
         colpal = cpal;
         memset(&cpal[0], 0, 17*sizeof(png_color));
         for(i=0; i<(1<<bpp); i++)
         {
            cpal[i+1].red  = palette[1];
            cpal[i+1].green= palette[2];
            cpal[i+1].blue = palette[3];
            palette+=8;
         }
      }
      ins = spritebits;
      spritebits = tmp;
      inm = maskbits;
      for(row=0; row<height; row++)
      {
         printf("row: %d\n",row);
         unsigned char *tmpinm = inm;
         unsigned char *tmpins = ins;
         ins += firstbit>>3;
         inm += firstbit>>3;
          
         shift=firstbit & 7;

         for(col=0; col<realwidth; col++)
         {
            unsigned char pixel = *ins;
            unsigned char mask = 255;
            if (maskbits)
            {
               mask = *inm;
               mask >>= shift;
               mask &= bitmask;
            }
              
            pixel >>= shift;
            pixel &= bitmask;
              
            *tmp = pixel+1;
            if (!mask)
               *tmp = 0;
              
            tmp++;
            shift += bpp;
              
            if (shift>7) { ins++; inm++; shift=0; }
              
         }
         ins = tmpins + width;
         inm = tmpinm + width;
      }
      firstbit=0;
      width=realwidth; 
   }
   if (bpp==8)
   {
      makeriscos256colpal();
      // make sure pointers are advances
      spritebits=(unsigned char *)buffer + sprite.image;
   }

   // Right, make the PNG - libpng does indeed sucketh
   png_infop info_ptr;
   png_structp png_ptr;
   FILE *pngfile;
   
   // Create the output struct
   png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, (png_voidp) NULL, NULL, NULL);
   info_ptr = png_create_info_struct(png_ptr);
   
   // For now write it to a temporary file
   char tmpname[256];
   sprintf(tmpname,"%s.png",sprite.name);
   pngfile=fopen(tmpname, "wb");
   png_init_io(png_ptr, pngfile);
   
   // compression
   png_set_compression_level(png_ptr, PNG_COMPRESSION_TYPE_DEFAULT);
   
   // Header stuff
   png_set_IHDR(png_ptr, info_ptr, realwidth, height*reps, 8 /* bpp */, PNG_COLOR_TYPE_PALETTE, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT );
   
   // Palette
   png_set_PLTE(png_ptr, info_ptr,  bpp==8?riscos256colpal:colpal, ncols );
  
   // Mask
   if (maskbits) png_set_tRNS(png_ptr, info_ptr, bpp==8?trans:trans+1, bpp==8?2:1, NULL );

   png_write_info(png_ptr, info_ptr);
   png_set_packing(png_ptr);
   for (i=0; i<height; i++)
   {
      int j;
      png_bytep row_pointer = spritebits + (firstbit>>3);
      spritebits += width;
       
      if (maskbits && (bpp>4) )
      {
         unsigned char *maskp = maskbits + (firstbit>>3);
         maskbits += width;
       
         for(j=0; j<realwidth; j++)
         {
            if (row_pointer[j] == 1)
               row_pointer[j] = 0;
              
            if (maskp[j] == 0)
               row_pointer[j]=1;
         }
      }
       
      for(j=0; j<reps; j++)
      {
         png_write_row(png_ptr, row_pointer);
      }
   }
 
   png_write_end(png_ptr, info_ptr);
   png_destroy_write_struct(&png_ptr, &info_ptr); 
   return realwidth;   
} 

void read_sprite_object(FILE *infile, FILE *outfile, objectheader object)
{
   transmatrix matrix;
   spritectrlblock sprite;
   unsigned char *buffer;
   int length=object.length;
   char transform[256]="";
   
   if (object.type == OBJECT_TRANSSPRITE)
   {
      // read matrix
      length -= sizeof(transmatrix);      
      fread(&matrix, sizeof(transmatrix), 1, infile);
      printf("  Transformed Sprite Object:\n");
      strcpy(transform, " transform=\"");
      dumpmatrix(matrix);
      /*
      if (matrix.a > 0 || matrix.d > 0)
      {
         sprintf(transform, "%s scale(%f %f)", transform, (double) matrix.a/
         65536, (double) matrix.d/65536);
      }*/
   }
   else
   {
      printf("  Sprite Object\n");
   }

   buffer = malloc(length);
   fread(&sprite, sizeof(spritectrlblock), 1, infile);
   
   if(object.type == OBJECT_TRANSSPRITE)
   {
      // We should be able to directly translate to an SVG matrix here.
      // Need to mess with the units though.
      sprintf(transform, "%s matrix(%f %f %f %f %f %f)", transform, converttransunits(matrix.a), converttransunits(matrix.b), converttransunits(matrix.c), converttransunits(matrix.d), topt(matrix.e), topt(master.y - matrix.f)-sprite.height);
      strcat(transform, "\"");
   }
   printf("  Sprite Control Block:\n");
   // Draw only saves one sprite in each chunk


   printf("   Name: %s\n", sprite.name);
   printf("   Width: %d\n", sprite.width);
   printf("   Height: %d\n", sprite.height);
   printf("   First bit: %d\n", sprite.firstbit);
   printf("   Last bit: %d\n", sprite.lastbit);
   printf("   Mode: %d\n", sprite.mode);
   printf("   Image: %x\n", sprite.image);
   printf("   Mask: %x\n", sprite.mask);
   printf("   Transformation: %s\n", transform);
   printf("  Data:\n");
   length-=sizeof(spritectrlblock);
   
   fread(buffer, 1, length, infile);
   sprite.width=read_sprite(sprite, buffer, length);
   fprintf(outfile, "<image xlink:href=\"%s.png\" height=\"%d\" width=\"%d\" %s/>\n",
      sprite.name, sprite.height, sprite.width, transform);
   //hexdump(buffer, length);
}

void read_objects(FILE *infile, FILE *outfile, int length)
{
   int curptr;
   objectheader object;
   int saveptr;

   saveptr=ftell(infile);
   do
   {
      curptr=ftell(infile);
      fread(&object, sizeof(objectheader), 1, infile);
      if (feof(infile)) break;
      if (object.type < 14)
      {
         printf(" Object type: %d %s\n", object.type, objectnames[object.type]);
         switch(object.type)
         {
            case OBJECT_FONTTABLE:
               read_font_table_object(infile, object, curptr);
               break;
            case OBJECT_TEXT:
               read_text_object(infile, outfile, object);
               break;
            case OBJECT_PATH:
               read_path_object(infile, outfile, object);
               break;
            case OBJECT_GROUP:
               read_group_object(infile, outfile, object);
               break;
            case OBJECT_SPRITE:
            case OBJECT_TRANSSPRITE:
               read_sprite_object(infile, outfile, object);
               break;
         }
      } else printf("Unknown object type %x, skipping\n", object.type);      
      if (!feof(infile))
         fseek(infile, curptr + object.length, SEEK_SET);
   } while ((length == -1 || ftell(infile)<(saveptr+length)) && !feof(infile));
}

int main(int argc, char **argv)
{
   char infilename[256],outfilename[256];
   drawheader header;
   FILE *infile, *outfile;
   unsigned char *chrptr;
   int i;
  
   strcpy(infilename,argv[1]);
   strcpy(outfilename,argv[2]); 
   infile=fopen(infilename, "rb");

   fread(&header, sizeof(drawheader), 1, infile);
   chrptr=strchr(header.creator,' ');
   if (chrptr != NULL)
   {
      *chrptr='\0';
   }
   
   if (header.magic != 0x77617244)
   {
      printf("Wrong magic is this a Draw file?\n");
      exit(1);
   }
   
   outfile=fopen(outfilename, "w");
   printf("File header for %s\n", infilename);
   printf(" Magic: %x\n", header.magic);
   printf(" Major version: %d\n", header.major);
   printf(" Minor version: %d\n", header.minor);
   printf(" Creator: %s\n", header.creator);
   printf(" Bounding Box: %gpt,%gpt x %gpt,%gpt\n", \
            topt(header.low.x), \
            topt(header.low.y), \
            topt(header.high.x), \
            topt(header.high.y));
   fprintf(outfile,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<svg xmlns=\"http://www.w3.org/2000/svg\" xml:space=\"preserve\" width=\"%g\" height=\"%g\" viewBox=\"0 0 %g %g\" xmlns:xlink=\"http://www.w3.org/1999/xlink\">\n",\
   topt(header.high.x)+1, topt(header.high.y)+1, \
   topt(header.high.x)+1, topt(header.high.y)+1);
   master.x=header.high.x+1;
   master.y=header.high.y;
   
   // Now read through each object
   read_objects(infile, outfile, -1);

   fprintf(outfile,"</svg>");
   return 0;
}
