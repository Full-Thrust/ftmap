******************************************************************************

ftmap v 1.8.3
=============

CONTENTS
    ... Usage
    ... Program flags
    ... Format of data file
    ... Fomat of game object images
    ... Formatting data file from game report
    ... Map making procedure
    ... Known Problems
    ... Authors
    ... Distribution
        ... New Features
        ... Unix
        ... Win32

==============================================================================

Usage: ftmap -a -b -d -f output.gif -g -i imagedir -l -r resource_file -v 

ftmap reads a fomatted file from the standard input and produces a gif map
of the data, according to the parameters contained within the file. ftmap
does text clash management so that the labels on the map are positions for
the best readability. It is design for use with the game Full Thrust by gzg.com

Example:

ftmap -b -i image -f example.gif < example.ft

Produces a bitonal map (white background, black foreground) in the file 
example.gif using the data in the file example.ft

Program flags
-------------

-a apply anti-alisasing, this doesn't work very well with color or bitonal
   game object images, it works best for conventional white images on a black 
   background

-b bitonal mode, uses a white background and inverts the game object bitmaps to
   be black. Any background image or color resource file is ignored. This mode 
   is used for producing maps that can be printed or email as the bitonal maps
   are small

-d debug mode currently draws boxes for the text clash resolution system and
   various ugly messages

-f followed by name of the resuting gif map file 

-g add a reference grid to the map axes

-i followed by name of directory to search for the game object images, this 
   allows several sets of images to be used and manageded

-l draw a legend of the game objects, this can take up a lot of room and is
   not subject to rigorous clash detection with existing text

-r followed by name of the resource file containing color definitions for the 
   main elements of an ftmap, *ignored* if -b specified

-v verbose mode it tells you whats its doing on

-w wallpaper tile the background otherwise stretch it into the final bitmap size


Format of data file
--------------------
Ok, now for the format of ftmap's data file, Generally blank lines are not 
tolerated and comments etc. are not supported.

The first section defines the map parameters

[Header Section]

<1> Title
<2> Background gif filename or '-' to use a plain background
<3> course trails flag - 1 on, 0 off
<4> min x co-ord (in game units)
<5> min y co-ord (in game units)
<6> max x co-ord (in game units)
<7> max y co-ord (in game units)
<8> number of pixels per game unit

Example:
NOTE Numbers are reference to the fields described and should not be included 
in actual file

<1> FT Example Map
<2> -
<3> 1
<4> -15.0
<5> 0.0
<6> 85.0
<7> 70.0
<8> 10.0

This section has a fixed length format and thus requires no delimiter for the
next section following. There follows a section defining the available game 
object classes and the images to use

[Game Object Image Section]

<1> The game object CLASS NAME
<2> The file name of the gif used for the class (pointing in direction 12)
<3> Scale factor for displaying the gif 
<4> Legend flag ( 1==list class in legend, 0==don't list class in legend)

This is repeated for each class, with a line containing a single * indicating
the end of the section. There follows a section describing each ship or other
game object (fighter, missile, asteroid).

Example:
NOTE Numbers are reference to the fields described and should not be included 
in actual file

<1> Hyperion
<2> ea_bc.gif
<3> 1.0
<4> 1
*

[Game Object Section]

<1> Name (can contain white space)
<2> CLASS NAME exact match to <1> in [Game Object Image Section])
<3> x co-ord
<4> y co-ord
<5> heading (1-12)
<6> facing (1-12)
<7> velocity
<8> change of heading delta (S=+ve, P=-ve e.g. p3 = -3, S2 = 2)

This is repeated for each class, with a line containing a single * indicating
the end of the section.

Example:
NOTE Numbers are reference to the fields described and should not be included 
in actual file

<1> NERGAL
<2> Hyperion
<3> 14.2
<4> 28.3
<5> 8
<6> 8
<7> 9
<8> 0
*


Data File Example
------------------
FT Map Example
-
1
-15.0
0.0
85.0
70.0
10.0
Omega
ea_cv.gif
1.0
1
Hyperion
ea_bc.gif
1.0
1
*
JASON
Omega
15.7
37.3
12
12
5
1
NERGAL
Hyperion
14.2
28.3
8
8
9
0
*


Fomat of game object images
---------------------------

The game object images are stored in individual gif files. One image per file. 
The default orientation of the file should be with the game object facing the 
12 o'clock position

Example:

           +-----------+  
           |     ^     |
           |    / \    |
           |    | |    |
           |   /   \   |
           |   |   |   |
           |   \   /   |
           |    #_#    |
           +-----------+


Black is always mapped to transparent in the game object files. 

The convention is for the image background to be black and the image foreground
to be white, *breaking the convention will produce the wrong results*.

Color images are supported where the image background is black and the
foreground various colors. For producing bitonal maps (white background black
foreground) the conventional images are inverted by the program automatically.

The images can be any size but typically not too big (33x33). 
Large images take a very long time to process anythin over 120 px will be slow. 
The images are scaled according to the scaling factor in the data file
The images can be of any game object a ship, fighters, missiles, asteroids etc.

Resource File
-------------

The -r file for ftmap allows the use of a separate resource file which
controls the appearance of the ftmap. The resource file is made up of sections
denoted by '[' ']' i.e. [Color] and within a section a resource name is
allocated a value. The section and resource names are case insensitive but
must be spelt correctly. A resource name has a following '=' and then its values
separated by white space. 

ftmap only supports a Color section at this time. The resource file can contain
comments beginning with ';' on a separate line or the end of a line. The example
shows the current color resources and what they apply to. The -r option colors 
are currently ignored if you use the -b flag.

Example:

;******************************************************************************
; ftmap color resource file
;
; The section identifier [Color] must be present & spelled correctly
;
; Resource names are case insensitive & must be spelled correctly
; The '=' must follow the resource names.
;
; Color values are given as red blue and green values between 0 - 255
; separated by white space
;
; History:
; 25-Feb-1997 Tim Jones
; Created
;******************************************************************************
;
[Color]
;------------------------------------------------------------------------------
;                    r    g    b
;                       0 - 255        comments
;------------------------------------------------------------------------------
backgroundColor =    0    30   0     ; background MUST always be specified first
foregroundColor =    0    255  0     ; replaces white in the image bitmaps
titleTextColor =     0    255  0     ; map title
axesColor =          0    200  0     ; x & y axes
axesgridColor =      0    100  0     ; axes grid
axesTextColor =      0    220  100   ; axes text
labelTextColor =     0    200  96    ; label text
leaderColor =        0    150  96    ; leader line from label
courseColor =        0    255  100   ; game object course tracks
locusColor =         200  255  100   ; accurate centre of game object
legendColor =        0    200  0     ; legend box
legendTextColor =    0    200  100   ; legend text
;******************************************************************************

Important:

backgroundColor is a special resource as it MUST be the first color in the 
file so that the GIF image has the correct transparency. The foregroundColor is
also a special resource as this changes the foreground color (white) of 
conventional images to the specified color. The results with non conventional
images will be undefined. 

Certain values should can be undefined by commenting out with ';'

; foregroundColor - don't use this setting if color sprites or sprites not white on black
; leaderColor - don't set this if you don't want lines draw from the label to the object
; locusColor - don't set this if you don't want dot drawn at the exact location of the game object

Formatting data file from game report
-------------------------------------

The ftmap data file format is optimised for computer reading but is difficult
for humans to read. Typically a game will produce reports which can be parsed
and formatted into the correct data file format by tools such as awk or perl. as
a last resort you can type in the data yourself.

Header File
~~~~~~~~~~~
Typically the game header section in the data file doesn't change and is stored
in a separate file that the text processing filter includes into the final data
file. These lines have the HEADER: so that the text filter can recognise them.

Example:

HEADER: FT Map Example
HEADER: background.gif
HEADER: 1
HEADER: -15.0
HEADER: 0.0
HEADER: 85.0
HEADER: 70.0
HEADER: 10.0
HEADER: Omega
HEADER: ea_cv.gif
HEADER: 1.0
HEADER: 1
HEADER: Hyperion
HEADER: ea_bc.gif
HEADER: 1.0
HEADER: 1
HEADER: *


There is an example awk program supplied example.awk that parses a game report 
example.rpt into the ftmap data file format example.ft.


Map making procedure
--------------------

Typical procedure for producing a map is thus.

1)  Edit the header file to change the title etc.

2)  Edit the report file to conform to the text filter program, this may 
    require fiddling with information so that is all looks the same. As most
    people seem to do this manually there are often formatting mistakes. Its a 
    trade off between a simple text parser and a complicated one that can cope 
    with various idiosyncrasies in the report format. This is the bit *you* have
    to figure out

3)  Run the text processor to produce the data file this merges the turn header
    with the data in the report to produce the data file ftmap requires.

    Example:

    awk -f example.awk example_header example.rpt > test.ft

4)  Produce the map using ftmap and the data file. if it core dumps its probably
    a problem with the data file, so check that first.

    ftmap -b -i image -f example.gif < test.ft 

5)  View the image and check for funnies, repeat from 2) to eliminate errors.


Known Problems
--------------

1) If the pixel per game unit value is large (>15) images produce can break up 
the ship images etc. AFAIK this is a rounding error problem in the image 
library and the workaround is to reduce the value (<=10).

2) FIXED in 1.8.3 Anti aliasing algortihtm is a little flaky and doesn't work with color or
bitonal maps very well.  Its designed to and works with the conventional image 
format (black background white foreground). 

3) Unrobust to problems in the data file format, will core dump.

4) Legend often covers up map data, don't use it in these cases or reduce the
content with the flag in the data file

5) The -i qualifier *must* be used or it fails to find the image 
files

Authors
-------
ftmap is the original IP of Alun Thomas, it was conceived by him and is all his work. I
Tim Jones have extended his work by adding various options and extending 
the program to add smart color compression and re-engineering the code to be more maintainable, robust and efficient 
and creating the distribution and doscumentation.


Distribution
------------
This program is FREEWARE and can be distributed as long as you don't claim its
yours or try to make money out of it (unlikely). As the source code is included 
(unix only) you can modify and improve the program if you wish and feed it back
to the user community via the FTGZG-L mailing list. Its now on Github too.

Release Notes
--------------

v 1.8.3
~~~~~~~

Fixed the re-color option using foregroundColor, this now repaints the sprites before 
they are rotated giving much better anti-aliasing results.

Increased max object to 20K and max object classes to 500.

Created simple unit tests in unit-test.sh

Removed all gcc compiler warnings.

Removed leader line from overlap tests if not used

v 1.8.2 
~~~~~~~

Certain resources in the ftmap init file no longer
have default values if they are undefined these are:

	foregroundColor
	leaderColor
	locusColor
	
They can be commented out in the init file by using the
comment character ';' semi-colon or omitted all together.

If a text label has a null value then the label will not
be plotted. A null value is a line with nothing on it but
a carriage return.

v 1.7
~~~~~

-g option to draw a grid at 10x10 game unit intervals

-r option to allow full color control via a color resource file

locus point drawn at the exact position of the game object on the map
for more accurate measuring or firing arcs and distance

game object tracks are more even in length when turning

Unix
~~~~
The distribution is in files

ftmap.tar - base pack

README.txt        - documentation
ftmap.c           - source code
ftmap.ini         - resource file
gd1.2.tar         - gd image library
Makefile          - makefile example
example.awk       - awk program example
example.hdr       - turn header example
example.rpt       - example turn report 
example.ft        - ftmap example data file uses example.tar images 
ex_img.tar        - example images 

Procedure:

I'd create a structure like
 
    $HOME/ftmap
    $HOME/ftmap/image

1) unpack ftmap.tar in $HOME/ftmap

2) unpack gd1.2.tar in $HOME/ftmap. it will create $HOME/ftmap/gd1.2
   ,go there. Modify $HOME/ftmap/gd1.2/Makefile there for you platform and 
   make libgd.a there using 'make'.

3) go to $HOME/ftmap and modify the ftmap Makefile for you platform compiler 
   and to point the libgd.a library from step 2) and where to install the 
   program; if different; then 

    make ftmap

4) move ex_img.tar to $HOME/ftmap/image and 
   unpack ex_img.tar there, these are the example images

5) From a console window 

    cd $HOME/ftmap
    ftmap -i image -f example.gif < example.ft

6) This should produce example.gif which you can view with your favourite paint 
   program (xv) or browser.

 
Win32
~~~~~
ftmap.zip - base pack

README.txt        - documentation
ftmap.exe         - the ftmap program
ftmap.ini         - resource file
example.awk       - awk program example
example.hdr       - turn header example
example.rpt       - example turn report 
example.ft        - ftmap example data file uses example.tar images 
ex_img.zip        - example images 

Procedure:

The Win32 distribution doesn't need building just un-pack the files and 
your ready to go, I'd create a structure like

    C:\ftmap
    C:\ftmap\image

1) unpack ftmap.zip in C:\ftmap 

2) move ex_img.zip to C:\ftmap\image and unpack  
   ex_img.zip there, these are the example images

3) From a console window (DOS box) 

    cd C:\ftmap
    ftmap -i image -f example.gif <example.ft

4) This should produce example.gif which you can view with your favourite paint 
   program or browser.

 
******************************************************************************
Last updated 2-Dec-1997 Tim Jones
******************************************************************************

