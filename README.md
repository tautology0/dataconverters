# dataconverters

Some random file converters:


# draw2svg
Converts RISC OS !Draw files to SVG. Currently it won't handle sprites, but gets most things right.

Homerton, Trinity and Corpus are converted to sans serif, serif and system. Which should be 98% correct most of the time.

It will be quite verbose when converting, at the moment there's no way to switch this off other than redirecting it elsewhere.

Recent additions support transformations and sprite. I stole the sprite code from Ian Jeffray (http://ian.jeffray.co.uk/riscos/).

To use:

   draw2svg infile outfile

# iwordreader
Converts BBC Micro Interword files to a sort of XMLy format.

Due to the way Interword works, there may be multiple \<strong\> or \<em\> tags in strange places.

To use:

   iwordreader infile 

For some reason it will only output to stdout. Needs lots of work.
