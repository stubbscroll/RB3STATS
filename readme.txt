rock band 3 forum banner stats by stubbscroll

given a userid (number, not name), read scorehero webpage and create 3 png
banner files with stats for pro keys, pro guitar and pro bass.

to compile, check make.bat which is for mingw in windows. it should be easy
to figure out how to build it in linux. two executables are built:
- rb3stats.exe which grabs scores from scorehero and stores them in stats.txt
- genimage.exe which reads stats.txt and generates banner image

to generate banners, run gen.bat (change userid first). in linux, make a
shell script that does the same stuff. this results in 3 files:
probass.png, progtr.png, prokeys.png.

the program uses icons from rb3probass.bmp. rb3progtr.bmp, rb3prokeys.bmp.

requirements:
- c compiler
- libcurl (web stuff)
- sdl 1.2 (image stuff)
- imagemagick (for converting bmp to png)

FUTURE WORK
- make banner less tall by changing the logo, for instance to RB3+icon on
  the same line
- get input from some who's good at graphics and heed their advice

i don't have plans of adding more instruments. it shouldn't be too hard if
you're determined enough anyway.
