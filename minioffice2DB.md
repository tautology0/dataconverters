# Mini Office II Database filetype

## Header Information
| Offset | Description |
| ------ | ----------- |
| ?0,?1  | 2 bytes: # of cards - (?1*256+?0) |

## Sets of 18 bytes of field data
| Offset | Size | Description |
| ------ | ----------- | ----|
| $0 | 11 characters (10 + CR) | for field name |
| $11 | 7 characters ( 6 + CR) | for numeric field |
|     | or ("@@@@@@" + CR) | for string field |

## Lengths
All string  fields (i.e. no units) have 60 characters
All numeric fields (i.e. units) have 10 characters

## Data
After header and fields all data is stored in CR terminated strings.
