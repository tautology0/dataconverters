Key Database File Format
============

Filer Information
-----------------

| Offset | Description |
| ------ | ----------- |
| $0 | 14  characters: Name of database (padded) |
| ?14 | LSB of address of above database | 
| ?15 | ESB of address of above database |
| ?16 | MSB of address of above database |

This is repeated for all files.

To derive the address of the database the formula is:

```?14+(256*?15)+(65536*?16)```

Header Information
------------------

| Offset | Description |
| ------ | ----------- |
| ?0 | LSB of start of data |
| ?1 | ESB of start of data |  
| ?2 | MSB of start of data |
| ?3 | Dummy |
| ?4 | Number of fields |
| ?5 | LSB of number of cards |
| ?6 | MSB of number of cards | 
| ?7 | LSB of maximum number of cards |
| ?8 | MSB of maximum number of cards |
| ?9-?13 | Dummies |

Start of data is given by:

```?14+(256*?15)+(65536*?16)```

Blocks of 31 bytes of field data
--------------------------------

| Offset | Description |
| ------ | ----------- |
| $0 | 14 characters: Name of field (no terminating CR) |
| ?14 | Dummy |
| ?15 | Dummy |
| ?16 | LSB of token pointer or first field number for formula |
| ?17 | ESB of token pointer or second field number for formula |
| ?18 | MSB of token pointer or formula operation |
| ?19 | Number of tokens for this field |
| ?20 | Dummy |
| ?21 | Dummy |
| ?22 | Dummy |
| ?23 | Dummy |
| ?24 | Length of field (in chars) |
| ?25 | Dummy |
| ?26 | Type of field |
| ?27 | Dummy |
| ?28 | Dummy |
| ?29 | Dummy |
| ?30 | Dummy |

Types of fields
---------------

Number          Type of field

1               Byte numeric
2               Date
3               Integer numeric
4               String
5               Logical
6               Map coordinate
7               Token
8               Real numeric
9               Dependant (formula)
10              String
11              Freetext

Tokens are stored at the token pointer in the file, in numeric order.
Each token is a non CR terminated string, 14 characters in length.
Formulas are composed in the following way:

                field(n) op field(m)    

Where n and m are the numbers of two numeric strings.
      op is the operation, in the file it is stored as a number,
          the numbers are:

Op number       Operation

1               Add      (+)
2               Subtract (-)
3               Multiply (*)
4               Divide   (/)

Data storage
------------

The data is stored in varing formats, the length of the actual data stored 
is taken from the length of the field.

Type of data    Stored as

Byte            1 byte of data. (i.e. a value 0-255)
Date            A string consisting of:
                4 bytes of year
                2 bytes of month
                2 bytes of day
                i.e. 15th June 1974 is stored as:
                        1974615
Integer         Stored as ASCII characters, but the data is stored
                backwards.
                i.e. 6564 is stored as:
                        @A              
String          A non CR terminated string, padded out with SPC characters
                to fill up to length of field.
Logical         1 byte of data to signify:
                        True  = 255
                        False = 0
Map Coord       Stored as ASCII characters, with the following values:
                the easting value is the first byte,
                the northing the second.
                i.e. a coordinate of 33 east 54 north is stored as
                        ![00]6[00]
Token           See below section.
Real            The real number is multiplied by 100 and stored as ASCII
                characters. Like the Integer, the data is written backwards.
                i.e. 64.65 is stored as:
                        A@
Dependant       No usuable data is stored, the formula can is taken from the
                field information, and the data can then be worked out.
Freetext        A 500 byte block of data, padded out with SPC characters
                To fill up 500 bytes. CR characters can be stored in this
                block.

Tokens
------

Tokens  are  stored,  in the data block by a 16 bit number, stored in LBF
format. i.e. LSB, MSB.

Tokens are stored as follows:
Firstly, 1 is added to the token number, then this number is stored as an
indices of 2. This means, that any number of tokens (up to the maximum of
17) can be stored for one data entry.
The best way to illustrate this is as an example:

e.g.    The token required is token 1, 
=> the data stored is 2^(1+1) i.e. 4
=> the resulting data is [04][00]

e.g.2   The token to be stored is token 6,
=> the data stored is 2^(6+1) i.e. 128
=> the resulting data is [7F][00]

Multiple token are stored as the addition of all the calculatons:

e.g.    The tokens required are 4 and 6,
=> the data stored is (2^(4+1))+(2^(6+1)) i.e. 32 + 128 = 160
=> the resulting data is [A0][00]

e.g.2   The tokens required are 3, 7 and 10,
=> the data stored is (2^(3+1))+(2^(7+1))+(2^(10+1)) i.e. 2320
=> the resulting data is [10][09]

A suggested routine to get out all the tokens is:

I%=&20000:tok$=""
REPEAT
REPEAT I%=I%/2
U. I%=0 OR (data DIV I%>=1) OR data=0
tok$+=toke$(field,INT(0.5+FNtoknum(I%))):data-=I%
U. I%=0 OR data=0

Also needed to be added, is the following Function:

DEFFNtoknum(num)=((LOG(num)/LOG(2))+1)

Where, data  is the data taken from the file,
       field is the current field number.
       toke$(A,B) is an array containing all of the tokens.
               A is the field number and
               B is the token number.
At the end of this, the tokens will be held in tok$.
