/*
******************************************************************************
* ftmap v 1.8.3
* 
* Draw a Full Thrust game map in gif format
*
* History:
* 6-Nov-1996 Alun Thomas
* Original author and idea 
* 
* 5-Feb-1997 Tim Jones
* Software reengineered
*
* 12-Feb-1997 Tim Jones
* Reimplemented text clash algorithm to give better results and to 
* prevent long leader lines and to stop leader lines obscuring text
*
* 17-Feb-1997 Tim Jones
* Add test to atan2 to stop domain errors on certain compilers
* Do all fading under ships before placing the ships to stop proximity
* fading
*
* 21-Feb-1997 Tim Jones
* Add locus point for game object centre
* Changes course plotting so that both legs are the same length
* Add -grid option
*
* 25-Feb-1997 Tim Jones
* Add color resource file
*
* 11-Apr-1997 Alun Thomas
* New color manager to stop palette slewing
*
* 1-Sep-1997 Tim Jones
* Real Thrust support use -t option
*
* 19-Nov-1997 Tim Jones
* Oct-tree quantized palette using Gervautz-Purgathofer method
* Background image wallpaper tiling
* Few bug fixes and general tidy up
*
* 21-Sep-1998 Tim Jones
* Make certain colour resources optional
* Don't plot null label strings
*
* 11-Mar-2021 Tim Jones
* Fix foregrond coloring of anti-alias ships
* Extend max objects to 20K and classes to 500
* Tidied compiler warnings
*
******************************************************************************
*/

/*
*******************
* Declarations
*******************
*/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <limits.h>
#include "gd.h"
#include "gdfontt.h"
#include "gdfonts.h"
#include "gdfontmb.h"
#include "gdfontl.h"
#include "gdfontg.h"

/*
*******************
* Inline functions 
*******************
*/

#ifndef MAX
#define MAX(A,B) ( ((A)>(B)) ? (A) : (B) )
#endif

#ifndef MIN
#define MIN(A,B) ( ((A)>(B)) ? (B) : (A) )
#endif

/*
*******************
* Constants
*******************
*/

#define PROGRAM "ftmap"
#define VERSION "1.8.3"
#define COPYRIGHT "Copyright Alun Thomas & Tim Jones 1997-2050"

#ifdef WIN32
#define SLASH "\\"
#else
#define SLASH "/"
#endif
#ifndef M_PI
#define M_PI ((double) 3.14159265358979323846264338327950)
#endif
#define MAX_BUFFER		 1024       /* max line length in file reading */
#define MAX_CLASSES      500   		/* game_object classes */
#define MAX_OBJECTS      20000 		/* objects on map */
#define MAX_BOXES        1000  		/* clash boxes */
#define MAX_LEADER       32    		/* leader line length */
#define MAX_SHIFTS       1000  		/* attempts to place text block */
#define MIN_RADIUS       0.0001 	/* size below which radius size is 0 */
#define ANNO_ANGLE       1    		/* search step rotation in degrees */
#define LEADER_WEIGHTING 0.3  		/* weighting factor for leader line clashing */ 
#define LINE_DILATION    4    		/* clash box dilation in pixels */
#define TEXT_DILATION    2    		/* clash box dilation in pixels */
#define EDGE_DILATION    50   		/* clash box dilation in pixels */
#define LABEL_FADE       25   		/* 25% fade under label */ 
#define GAME_OBJECT_FADE 25   		/* 50% fade under ship */ 
#define TITLE_FADE       50   		/* 50% fade under title */
#define LEGEND_FADE      50   		/* 50% fade under legend */
#define NOT_DEFINED      999  		/* GIF color not defined */
#define COL_BLK_SIZE     500  		/* Color block size */  
#define GIF_PALETTE_SIZE 256  		/* GIF image palette size */ 
#define RESERVED_COLORS  12   		/* Reserved Colors */
#define IMAGE_PALETTE_SIZE GIF_PALETTE_SIZE - RESERVED_COLORS
#define TRUE             1    		/* boolean true */
#define FALSE            0    		/* boolean false */

/*
*******************
* Types
*******************
*/

typedef struct point {
    int x;
    int y;
} Point;

typedef struct line {
    Point start;
    Point end;
} Line;

typedef struct box {
    int min_x;
    int min_y;
    int max_x;
    int max_y;
} Box;

typedef struct gameobject {
    char *name;
    double x,
           y;
    double speed;
    int heading,
        facing;  
    int delta_heading;          /* +ve=Stbd -ve=Port */
    gdImagePtr image[12];
    int radius;
    int cen_x,
        cen_y;
} GameObject;

typedef struct colorentry {
 int r,g,b;            /* RGB values of required color */
} ColorEntry;

typedef struct palette {
    ColorEntry color[GIF_PALETTE_SIZE]; /* color palette 0-255*/
    int size;
} Palette;

typedef struct node {
    int bIsLeaf;               // TRUE if node has no children
    int nPixelCount;           // Number of pixels represented by this leaf
    int nRedSum;               // Sum of red components
    int nGreenSum;             // Sum of green components
    int nBlueSum;              // Sum of blue components
    struct node* pChild[8];    // Pointers to child nodes
    struct node* pNext;        // Pointer to next reducible node
} Node;

/*
*******************
* Public Global
*******************
*/

gdImagePtr im_out     =NULL;
gdImagePtr background =NULL;

GameObject class[MAX_CLASSES];
GameObject game_objects[MAX_OBJECTS];

double min_x        =0.0;
double min_y        =0.0;
double max_x        =0.0;
double max_y        =0.0;
double pix_per_unit =0.0;

/* line styles
 */
int lgrid[2];
int lgrid_size   =2;
int lleader[2];
int lleader_size =2;
int lcourse[4];
int lcourse_size =4;

int index_class[MAX_CLASSES];
int num_indexed_classes=0;

/* program flags
 */
int verbose     =0;
int debug       =0;
int color       =1;
int tracking    =0;
int legend      =0;
int grid        =0;
int resample    =0;
int real_thrust =0;
int wallpaper   =0;

/* color indexes default impossible value
 */
ColorEntry foreground_rgb;


int foreground_color  =NOT_DEFINED;
int background_color  =NOT_DEFINED;
int title_text_color  =NOT_DEFINED;
int label_text_color  =NOT_DEFINED;
int axes_color        =NOT_DEFINED;
int axes_text_color   =NOT_DEFINED;
int axes_grid_color   =NOT_DEFINED;
int leader_color      =NOT_DEFINED;
int legend_color      =NOT_DEFINED;
int legend_text_color =NOT_DEFINED;
int locus_color       =NOT_DEFINED;
int course_color      =NOT_DEFINED;

int num_classes      =0;
int num_game_objects =0;

char *image_dir         =NULL;
char *title             =NULL;
char *gif_filename      =NULL;
char *resource_filename =NULL;

int out_x =0;
int out_y =0;  

FILE *in_file=NULL;
 
/*
 ****************** 
 * Private Global
 ******************
 */

static int num_boxes =0;
static Box clash_box[MAX_BOXES];
static Palette palette;

/* Color tree
 */
static Node* pTree   	=NULL;
static int nLeafCount 	=0;
static Node* pReducibleNodes[9]= {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};

/*
*******************
* Functions
*******************
*/


/* createNode
------------------------------------------------------------------------------
Create a node in the color quantization octree

*/
static Node* createNode (int nLevel, int nColorBits, int* pLeafCount, Node** pReducibleNodes)
{
    Node* pNode;
	
/*     printf("createNode\n"); */

    if ((pNode = (Node*) malloc(sizeof (Node))) == NULL)
        return NULL;
    memset(pNode,'\0',sizeof (Node));

    pNode->bIsLeaf = (nLevel == nColorBits) ? TRUE : FALSE;
    if (pNode->bIsLeaf)
        (*pLeafCount)++;
    else { // Add the node to the reducible list for this level
        pNode->pNext = pReducibleNodes[nLevel];
        pReducibleNodes[nLevel] = pNode;
    }
    return pNode;
}


/* addColor
------------------------------------------------------------------------------
Add a color to the color quantization octree

*/
static void addColor (Node** ppNode, int r, int g, int b, int nColorBits,
    int nLevel, int* pLeafCount, Node** pReducibleNodes)
{
    int nIndex, shift;
    static int mask[8] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };

    /* printf("addColor\n"); */

    // If the node doesn't exist, create it
    if (*ppNode == NULL)
        *ppNode = createNode (nLevel, nColorBits, pLeafCount, pReducibleNodes);

    // Update color information if it's a leaf node
    if ((*ppNode)->bIsLeaf) {
        (*ppNode)->nPixelCount++;
        (*ppNode)->nRedSum    += r;
        (*ppNode)->nGreenSum  += g;
        (*ppNode)->nBlueSum   += b;
    }

    // Recurse a level deeper if the node is not a leaf
    else {
        shift = 7 - nLevel;
        nIndex = (((r & mask[nLevel]) >> shift) << 2) |
                 (((g & mask[nLevel]) >> shift) << 1) |
                  ((b & mask[nLevel]) >> shift);
        addColor (&((*ppNode)->pChild[nIndex]), r, g, b, nColorBits,
            nLevel + 1, pLeafCount, pReducibleNodes);
    }
}

/* reduceTree 
------------------------------------------------------------------------------
Reduce the colors in the color quantization octree

*/
static void reduceTree (int nColorBits, int* pLeafCount, Node** pReducibleNodes)
{
    int i;
    Node* pNode;
    int nRedSum, nGreenSum, nBlueSum, nChildren;

    /* printf("reduceTree\n"); */

    // Find the deepest level containing at least one reducible node
    for (i=nColorBits - 1; (i>0) && (pReducibleNodes[i] == NULL); i--);

    // Reduce the node most recently added to the list at level i
    pNode = pReducibleNodes[i];
    pReducibleNodes[i] = pNode->pNext;

    nRedSum = nGreenSum = nBlueSum = nChildren = 0;
    for (i=0; i<8; i++) {
        if (pNode->pChild[i] != NULL) {
            nRedSum += pNode->pChild[i]->nRedSum;
            nGreenSum += pNode->pChild[i]->nGreenSum;
            nBlueSum += pNode->pChild[i]->nBlueSum;
            pNode->nPixelCount += pNode->pChild[i]->nPixelCount;
            free(pNode->pChild[i]);
            pNode->pChild[i] = NULL;
            nChildren++;
        }
    }

    pNode->bIsLeaf = TRUE;
    pNode->nRedSum = nRedSum;
    pNode->nGreenSum = nGreenSum;
    pNode->nBlueSum = nBlueSum;
    *pLeafCount -= (nChildren - 1);
}

/* deleteTree
------------------------------------------------------------------------------
Delete the color quantization octree


static void deleteTree (Node** ppNode)
{
    
    int i;
    printf("deleteTree\n");

    for (i=0; i<8; i++) {
        if ((*ppNode)->pChild[i] != NULL)
            deleteTree (&((*ppNode)->pChild[i]));
    }
    free(*ppNode);
    *ppNode = NULL;
}
*/

/* getPaletteColors 
------------------------------------------------------------------------------
Retrieve colors from the octree into a color palette

*/
static void getPaletteColors (Node* pTree, Palette *pPalEntries, int* pIndex)
{
    int i;
    /* printf("getPaletteColors\n"); */

    if (pTree->bIsLeaf) {
        pPalEntries->color[*pIndex].r =
            (int) ((pTree->nRedSum) / (pTree->nPixelCount));
        pPalEntries->color[*pIndex].g =
            (int) ((pTree->nGreenSum) / (pTree->nPixelCount));
        pPalEntries->color[*pIndex].b =
            (int) ((pTree->nBlueSum) / (pTree->nPixelCount));
            if (debug) {
                printf("Palette %d r-%d g-%d b-%d\n",*pIndex,pPalEntries->color[*pIndex].r,pPalEntries->color[*pIndex].g,pPalEntries->color[*pIndex].b);
            }
        (*pIndex)++;
    }
    else {
        for (i=0; i<8; i++) {
            if (pTree->pChild[i] != NULL)
                getPaletteColors (pTree->pChild[i], pPalEntries, pIndex);
        }
    }
}

/*
------------------------------------------------------------------------------
Load color map from image

*/
void ColorMgr_getImageColorMap( gdImagePtr im , int nMaxColors)
{
    int nColorBits=8;
    int im_color;
    int r,g,b;
    int x,y;
 
    /* printf("ColorMgr_getImageColorMap\n"); */

    /* Scan image pixels adding to color quantization tree and reducing
     * tree if exceeding the palette limits
     */
    for ( x=0 ; x < im->sx ; x++ ) {
        for ( y=0 ; y < im->sy ; y++ ) { 
            im_color = gdImageGetPixel(im,x,y);
            b = gdImageBlue(im, im_color);
            g = gdImageGreen(im, im_color);
            r = gdImageRed(im, im_color);
            addColor (&pTree, r, g, b, nColorBits, 0, &nLeafCount, pReducibleNodes);
            while (nLeafCount > nMaxColors)
                    reduceTree (nColorBits, &nLeafCount, pReducibleNodes);
        }
    }
}


/* ColorMgr_allocateImageColors
------------------------------------------------------------------------------
Allocate quantized colors in image

*/
void ColorMgr_allocateImageColors( gdImagePtr im )
{
    int nIndex = 0;
    int c=0;
    
    getPaletteColors(pTree, &palette, &nIndex);
    if (debug) {
        printf("Number colors in oct-tree %d\n",nIndex); 
    }
    for (c=0; c < nIndex; c++) {
        if ( gdImageColorAllocate(im, palette.color[c].r, palette.color[c].g, palette.color[c].b) == -1 ) {
                printf("**** Warning image color palette exhausted\n");
                break; /* for */
            }
        }
    if (verbose) {
        printf( "%d colors allocated\n", c);
    }
}

/* Rint
------------------------------------------------------------------------------
Version of math.h function rint() that works on this HP-UX config

*/
static double Rint( double in )
{
    if ( in < 0.0 ) {
        return (double)( (int)( in - 0.5 ) );
    }else{
        return (double)( (int)( in + 0.5 ) );
    }
}




/* distance
------------------------------------------------------------------------------
Calculate integer distance between two points

*/
int distance(Point p1, Point p2)
{
    double dx,dy;

    dx = abs ((double)(p1.x - p2.x));
    dy = abs ((double)(p1.y - p2.y));
    return (int) sqrt((dx*dx)+(dy*dy));
}


/* Line_isOrthogonal 
------------------------------------------------------------------------------
Is line orthogonal?

Return:
 true  - orthogonal
 false - not orthogonal

*/
int Line_isOrthogonal (int startx, int starty, int endx, int endy) 
{
    return (abs(startx - endx) == 0 || abs(starty - endy) == 0);
}

/* BoxMgr_numberBoxes
------------------------------------------------------------------------------
Get number of boxes

*/
int BoxMgr_numberBoxes(void)
{
   return num_boxes;   
}


/* Box_boxDouble
------------------------------------------------------------------------------
Box constructor 4 doubles

*/
Box Box_boxDouble(double x1,double y1, double x2, double y2)
{
    Box b;
    b.min_x = MIN(x1, x2);
    b.min_y = MIN(y1, y2);
    b.max_x = MAX(x1, x2);
    b.max_y = MAX(y1, y2);
    return b;
}


/*
------------------------------------------------------------------------------
Box constructor 4 ints

*/
Box Box_boxInt(int x1,int y1, int x2, int y2)
{
    Box b;
    b.min_x = MIN(x1, x2);
    b.min_y = MIN(y1, y2);
    b.max_x = MAX(x1, x2);
    b.max_y = MAX(y1, y2);
    return b;
}


/*
------------------------------------------------------------------------------
Box constructor 2 points

*/
Box Box_boxPoint(Point p1, Point p2)
{
    Box b;
    b.min_x = MIN(p1.x, p2.x);
    b.min_y = MIN(p1.y, p2.y);
    b.max_x = MAX(p1.x, p2.x);
    b.max_y = MAX(p1.y, p2.y);
    return b;
}


/*
------------------------------------------------------------------------------
Draw box

*/
void Box_draw(Box b, gdImagePtr im)
{
	gdImageLine(im, b.min_x, b.min_y, b.min_x, b.max_y, gdStyled);
	gdImageLine(im, b.min_x, b.max_y, b.max_x, b.max_y, gdStyled);
	gdImageLine(im, b.max_x, b.max_y, b.max_x, b.min_y, gdStyled);
	gdImageLine(im, b.max_x, b.min_y, b.min_x, b.min_y, gdStyled);
}


/*
------------------------------------------------------------------------------
Dilate Box by d

*/
Box Box_dilate(Box bin, int d)
{
    Box bout;
    bout.min_x = bin.min_x - d;
    bout.min_y = bin.min_y - d;
    bout.max_x = bin.max_x + d;
    bout.max_y = bin.max_y + d;
    return bout;
}


/*
------------------------------------------------------------------------------
Get corner of box closest to a given point

*/
Point Box_closestCorner(Box b, Point p)
{
	Point po;

    if (b.min_x >= p.x) {
        po.x = b.min_x;
    } else if (b.max_x <= p.x) {
        po.x = b.max_x;
    } else {
        po.x = (b.min_x + b.max_x) / 2;
    }

    if (b.min_y >= p.y) {
        po.y = b.min_y;
    } else if (b.max_y <= p.y) {
        po.y = b.max_y;
    } else {
        po.y = (b.min_y + b.max_y) / 2;
    }
	return po;
}

/*
------------------------------------------------------------------------------
Get midpoint of box closest to a given point

*/
Point Box_closestMidPoint(Box b, Point p)
{
    Point points[4];
    int min_distance = INT_MAX;
    int i=0;
    int j=0;

    /* calculate mid point in each box side and choose the nearest to the 
     * target point
     */
    points[0].x = b.min_x;
    points[0].y = b.min_y + ((b.max_y - b.min_y) / 2);
    points[1].x = b.min_x + ((b.max_x - b.min_x) / 2);
    points[1].y = b.max_y;
    points[2].x = b.max_x;
    points[2].y = b.min_y + ((b.max_y - b.min_y) / 2);
    points[3].x = b.min_x + ((b.max_x - b.min_x) / 2);
    points[3].y = b.min_y;

    for (i=0; i < 4; i++) {
        int d;

        d = distance(p,points[i]);
        if ( d < min_distance ) {
            min_distance = d;
            j = i;
        }
    }
	return points[j];
}

/* 
------------------------------------------------------------------------------
Get the area of overlap between two boxes

Return:
 overlap area
 
*/
int Box_overlap(Box box1, Box box2)
{
    int min_x,
        min_y,
        max_x,
        max_y;

    min_x = MAX(box1.min_x, box2.min_x);
    min_y = MAX(box1.min_y, box2.min_y);
    max_x = MIN(box1.max_x, box2.max_x);
    max_y = MIN(box1.max_y, box2.max_y);

    if ((min_x >= max_x) || (min_y >= max_y)) {
        return 0;
    } else {
        return (max_x - min_x) * (max_y - min_y);
    }
}

/*
------------------------------------------------------------------------------
Add box to box manager

*/
void BoxMgr_add(Box box)
{
   clash_box[num_boxes++] = box;
}


/*
------------------------------------------------------------------------------
Draw boxes in box manager

*/
void BoxMgr_draw(gdImagePtr im)
{
   int i;

   for (i=0; i < num_boxes; i++) {
       Box_draw(clash_box[i],im);
   }
}


/*
------------------------------------------------------------------------------
Get specified box from box manager 

*/
Box BoxMgr_get(int box_number)
{
   return clash_box[box_number];   
}

/*
------------------------------------------------------------------------------
Change colors in an image, useful for coloring an image or inverting it

*/
void changeColor(gdImagePtr im, int from_color, int to_color, int swap) 
{
    int x,y,color,fr,fg,fb,tr,tg,tb,pr,pg,pb;
    
    /* swap color not on index but rgb values
     */
    fr = gdImageRed(im, from_color);
    fg = gdImageGreen(im,from_color);
    fb = gdImageBlue(im,from_color);
    tr = gdImageRed(im, to_color);
    tg = gdImageGreen(im,to_color);
    tb = gdImageBlue(im,to_color);
    for (x=0; x<=im->sx; x++) {
        for (y=0; y<=im->sy; y++) {
            color = gdImageGetPixel(im,x,y);
            pr = gdImageRed(im, color);
            pg = gdImageGreen(im,color);
            pb = gdImageBlue(im,color);

            if (pr == fr && pg == fg && pb == fb ) {
                gdImageSetPixel(im,x,y,to_color); 
			}else if (swap && pr == tr && pg == tg && pb == tb) {
                gdImageSetPixel(im,x,y,from_color);          
			}
		}
	}
}


/* changeForeground
------------------------------------------------------------------------------
Change image foreground color - assumes the foreground is white

*/
		
void changeForeground(gdImagePtr im)
{
	int r;
	int g;
	int b;
	int new_f;
	int old_f;
	
	if (verbose) printf("Changing foreground colour\n");

	/* get foreground color and get best match in image
	 */
	r = gdImageRed(im_out,foreground_color);
	g = gdImageGreen(im_out,foreground_color);
	b = gdImageBlue(im_out,foreground_color);

	new_f = gdImageColorExact(im,r,g,b);
	if (new_f == -1) {
		new_f = gdImageColorAllocate(im,r,g,b);
		if (new_f == -1) {
			new_f = gdImageColorClosest(im,r,g,b);
		}
	}
	/* get old foreground color (white)  best match in image
	 */
	old_f = gdImageColorExact(im,255,255,255);
	if (old_f == -1) {
		old_f = gdImageColorAllocate(im,255,255,255);
		if (old_f == -1) {
			old_f = gdImageColorClosest(im,255,255,255);
		}
	}
	/* change old foreground for new foreground
	 */
	changeColor(im,old_f,new_f,0);
}




/*
------------------------------------------------------------------------------
Rotate and scale a game_object gif with optional resampling, the resampling
only works on a black background.

Note if recolouring - recolour before transforming the image

*/
gdImagePtr spinImage(gdImagePtr im_in, double angle, double scale, int resample)
{
    /* Input and output images 
     */
    gdImagePtr im_out;

    /* image sizes 
     */
    int in_x,
        in_y,
        out_l;

    /* image centre co-ords 
     */
    int in_xcen,
        in_ycen;
    int out_xcen,
        out_ycen;

    /* Color indexes 
     */
    int incol,
        outcol = -1;
    int out_trans;

    /* color components of input pixel 
     */
    int blue;
    int red;
    int green;

    /* variables for rotation stuff 
     */
    int xp,
        yp,
        xsup,
        ysup;
    int sample_count;
    double xout,
           yout;
    double r,
           angle_p,
           angle_q;
    
    int xq,
        yq;

    in_x = im_in->sx;
    in_y = im_in->sy;
    in_xcen = in_x / 2;
    in_ycen = in_y / 2;

    /* calculate size of output images 
     */
    out_l = (int) (0.9999 + scale * sqrt((double) (in_x * in_x + in_y * in_y)));
    out_xcen = out_l / 2;
    out_ycen = out_l / 2;

    /* Create output image 
     */
    im_out = gdImageCreate(out_l, out_l);

    /* First color allocated is background. 
     */
    if (color) {
        /* black
         */
        out_trans = gdImageColorAllocate(im_out, 0, 0, 0);
    }else{
        /* white
         */
        out_trans = gdImageColorAllocate(im_out, 255, 255, 255);
    }

    /* Set transparent color. 
     */
    gdImageColorTransparent(im_out, out_trans);
	

    /* produce rotated copy of input image in output image with optional 
     * resampling
     */
    for (xp = 0; xp < out_l; xp++) {
        for (yp = 0; yp < out_l; yp++) {
            sample_count = 0;
            red = 0;
            green = 0;
            blue = 0;

            if (resample ) {
                /* sample upto 25 probes around the equivalent point in the 
                 * original image taking the new images rotation and scale 
				 * into account
                 *               (xp+0.4, yp+0.4)
                 *           +---+
                 *           | * |        * = (xp,yp)
                 *           +---+
                 * (xp-0.4, yp-0.4)
                 * 
                 * if the rotation angle is 0 and the scaling factor 1 then 
                 * these points should all fall on a single input pixel. For 
                 * an output pixel at (x,y) these points should be in the area:
                 *
                 * These co-ordinates are scaled and rotated to give points in 
                 * the input image. The RGB values of the input pixels nearest 
                 * to these points are then averaged to give the color of the 
                 * output pixel.
                 * 
                 * pixel co-ordinates are integers, so you can consider the 
                 * actual co-ordinates to lie at the centre of a square 1 unit 
                 * across, inside which float co-ords round onto the pixel.
                 */
                for (xsup = 0; xsup < 5; xsup++) {
                    for (ysup = 0; ysup < 5; ysup++) {

                        xout = (double) (xp + (xsup * (double) 0.2) - 0.4);
                        yout = (double) (yp + (ysup * (double) 0.2) - 0.4);
                        r = sqrt((double) ((xout - out_xcen) * (xout - out_xcen)
                                           + (yout - out_ycen) * 
                                           (yout - out_ycen))) / scale;
						if ( r > MIN_RADIUS ) { 
							angle_p = atan2(yout - (double)out_ycen, 
											xout - (double)out_xcen);
						}else{
							angle_p = 0.0;
						}
                        angle_q = angle_p - angle;
                        xq = Rint(r * cos(angle_q) + (double)in_xcen);
                        yq = Rint(r * sin(angle_q) + (double)in_ycen);

                        if (gdImageBoundsSafe(im_in, xq, yq)) {
                            sample_count++;
                            incol = gdImageGetPixel(im_in, xq, yq);
                            red += gdImageRed(im_in, incol);
                            green += gdImageGreen(im_in, incol);
                            blue += gdImageBlue(im_in, incol);
                        }
                    }
                }

                /* if samples taken in original image take the mean color and 
                 * get the closest match in the new image
                 */
                if (sample_count > 0) {
                    red /= sample_count;
                    green /= sample_count;
                    blue /= sample_count;
                }else{
                    outcol = out_trans;
                }

            }else{
                xout = (double) xp;
                yout = (double) yp;
                r = sqrt((double) ((xout - out_xcen) * (xout - out_xcen)
                                   + (yout - out_ycen) * 
                                   (yout - out_ycen))) / scale;
				if ( r > MIN_RADIUS ) {
					angle_p = atan2(yout - (double)out_ycen, 
									xout - (double)out_xcen);
				}else{
					angle_p = 0.0;
				}
                angle_q = angle_p - angle;
                xq = Rint(r * cos(angle_q) + (double)in_xcen);
                yq = Rint(r * sin(angle_q) + (double)in_ycen);
                if (gdImageBoundsSafe(im_in, xq, yq)) {
                    incol = gdImageGetPixel(im_in, xq, yq);
                    red = gdImageRed(im_in, incol);
                    green = gdImageGreen(im_in, incol);
                    blue = gdImageBlue(im_in, incol);                   
                }else{
                    outcol = out_trans;
                }
            }

            /* allocate best color in output image
             */
            if (outcol == -1) {
                outcol = gdImageColorExact(im_out, red, green, blue);
                if (outcol == -1) {
                    outcol = gdImageColorAllocate(im_out, red, green, blue);
                    if (outcol == -1) {
                        outcol = gdImageColorClosest(im_out, red, green, 
                                                     blue);
                    }
                }
            }
            gdImageSetPixel(im_out, xp, yp, outcol);
            outcol = -1;
        }
    }
    return im_out;
}


/*
------------------------------------------------------------------------------
Get the area of overlap between game object and area defined by position and 
height and width

Return:
 overlap area

*/
int overlap(GameObject game_object, int x, int y, int w, int h)
{
    int min_x,
        min_y,
        max_x,
        max_y;

    min_x = MAX(game_object.cen_x - game_object.radius, x);
    min_y = MAX(game_object.cen_y - game_object.radius, y);
    max_x = MIN(game_object.cen_x + game_object.radius, x + w);
    max_y = MIN(game_object.cen_y + game_object.radius, y + h);

    if ((min_x >= max_x) || (min_y >= max_y)) {
        return 0;
    } else {
        return (max_x - min_x) * (max_y - min_y);
    }
}


/*
------------------------------------------------------------------------------
Draw a circle in specified color around game_object

*/
void circleGameObject(gdImagePtr image, GameObject game_object, int color)
{
    gdImageArc(image, game_object.cen_x, game_object.cen_y, 
               2 * game_object.radius, 2 * game_object.radius,
               0, 360, color);
}


/*
------------------------------------------------------------------------------
Read command arguments

*/
void getArgs(int argc, char* argv[])
{
    int c=0;

    while (--argc > 0 && (*++argv)[0] == '-') { /* walks args */
        while ( (c = *++argv[0]) ) {                /* walks arg string */
            switch (toupper(c)) {
                case 'A': {
                    resample = 1;
                    break;
                }               
                case 'B': {
                    color=0;
                    break;
                }               
                case 'D': {
                    debug = 1;
                    break;
                }	
                case 'F': {
                    gif_filename = strdup((++argv)[0]);
                    argc--;
                    break;
                }
				case 'G': {
                    grid = 1;
                    break;
                }
                case 'I': {
                    image_dir = strdup((++argv)[0]);
                    argc--;
                    break;
                }
                case 'L': {
                    legend = 1;
                    break;
                }
				case 'R': {
                    resource_filename = strdup((++argv)[0]);
					argc--;
                    break;
                }
                case 'T': {
                    real_thrust = 1;
                    break;
                }
                case 'V': {
                    verbose = 1;
                    break;
                }
                case 'W': {
                    wallpaper = 1;
                    break;
                }
                default: {
                    fprintf(stderr,"ftmap: illegal option %c\n",c);
                    argc=0;
                    break;
                }
            }
            break;
        }
    }
    if (argc) {
        fprintf(stderr,"usage: ftmap -a -b -d "
                "-f filename.gif -g -i image_dir -l -r resource.ini -t -v -w\n");
        exit(1);
    }
}



/*
------------------------------------------------------------------------------
Draw map reference system, this is a pair of cartesan axes always along the
bottom x & y axis of the image

*/
void drawMapAxes(gdImagePtr im, int axes_text_color, int axes_color,
                 double pix_per_unit, int min_x, int min_y)
{
    int a=0;
	int len=0;
    int max_len = 0;
    char text[10];

	gdImageSetStyle(im,lgrid,lgrid_size);

    /* Y-Axis Line
     */
    gdImageLine(im, 0, 0, 0, im->sy - 1,axes_color);

    /* X-Axis Line 
     */
    gdImageLine(im,0,im->sy-1,im->sx-1,im->sy-1,axes_color);

    /* Mark up X-Axis 
     */
    for (a = Rint(min_x);
         Rint((double)(a - min_x)* pix_per_unit) < im->sx-1; a++) {
        int x1;

        x1 = Rint((double)(a - min_x) * pix_per_unit);
        switch (abs (a % 10)) {

            case 0:
               len = sprintf(text, "%d", a);
               if (x1 == 0) {
                  ;
               }else{ 
                   gdImageLine(im,x1,im->sy-1,x1,im->sy-11,axes_color);
                   gdImageString(im,gdFontSmall,
                                 x1 - (len / 2) * ((gdFont *) gdFontSmall)->w,
                                 im->sy - 11 - ((gdFont *) gdFontSmall)->h,
                                 text, axes_text_color);
               }
			   /* draw optional grid 
				*/
               if (grid) {
				  gdImageLine(im,x1,
							  im->sy - 11 - ((gdFont *) gdFontSmall)->h,
							  x1,0,gdStyled); 
				  BoxMgr_add
					  (Box_dilate
					   (Box_boxInt
						(x1,im->sy - 11 - ((gdFont *) gdFontSmall)->h,x1,0)
						,LINE_DILATION));
			  } 
			   break;
            
            case 5:
            gdImageLine(im,x1,im->sy-1,x1,im->sy-6,axes_color);
            break;
            
            default:
            gdImageLine(im,x1,im->sy-1,x1,im->sy-3,axes_color);
            break;
        }
    }
    /* Add clash box for X axis for height of text on axis (bottom) and for 
	 * edge of map (top)
     *
     *
     *           +----------------------+
     *      top  |                      | 
     *           +-y-+--------------+---+ 
     *           | | |              |   |
     *     left  | | |              |   | right
     *           | | |              |   |
     *           | | |              |   |
	 *           +-|-+--------------+---+ 
     *           | -----------------x   | bottom
     *           +----------------------+
     *
	 *
	 * top then bottom 
	 */
    BoxMgr_add(Box_boxInt(-EDGE_DILATION,0,im->sx+EDGE_DILATION,
						  -EDGE_DILATION));
    BoxMgr_add(Box_boxInt(-EDGE_DILATION,im->sy+EDGE_DILATION,
						  im->sx+EDGE_DILATION,
                          im->sy - 11 - ((gdFont *)gdFontSmall)->h));

    /* Mark up Y-Axis get the widest annotation width 
     */
    for (a = Rint(min_y); 
         Rint((double)(a - min_y) * pix_per_unit) < im->sy-1; a++) {
        int y1;

        y1 = Rint((double)im->sy - (double)(a - min_y)* pix_per_unit);
        switch (abs (a % 10)) {

            case 0:
            len = sprintf(text, "%d", a);

			/* save widest text width 
			 */
			if (len > max_len) {
				max_len = len;
			}

            /* special don't annotate where the axes cross
             */
            if ( y1 == im->sy  ) {
                ;
            }else{
            
                gdImageLine(im,0,y1,10,y1,axes_color);
                gdImageString(im,gdFontSmall,10+((gdFont *) gdFontSmall)->w,
                              y1 - ((gdFont *) gdFontSmall)->h / 2,
                              text,axes_text_color);
            }

			/* draw optional grid 
			 */
			if (grid) {
			   gdImageLine(im,
						   10 + (((gdFont*)gdFontSmall)->w * (len+1)) 
						   ,y1,im->sx,y1,gdStyled);
			   BoxMgr_add
				   (Box_dilate
					(Box_boxInt
					 (10 + (((gdFont*)gdFontSmall)->w * (len+1)) ,y1,im->sx,y1)
					 ,LINE_DILATION));
		   }
            break;
            
            case 5: 
            gdImageLine(im,0,y1,5,y1,axes_color);
            break;
            
            default:
            gdImageLine(im,0,y1,2,y1,axes_color);
            break;
        }
    }
	/* Add clash box for Y axis for axis widest text on axis (left) and for 
	 * edge of map (right) (see diagram above)
	 *
	 * left then right
	 */
    BoxMgr_add(Box_boxInt(-EDGE_DILATION,0,
						  10 + (((gdFont *)gdFontSmall)->w * ++max_len),
                          im->sy - 1));    
    BoxMgr_add(Box_boxInt(im->sx,0,im->sx+EDGE_DILATION,im->sy - 1)); 
}



/*
------------------------------------------------------------------------------
Draw vector trail for game_objects course in Full or real thrust style

In Full Thrust style the course is drawn along the game objects track using the
delta heading value.

In Real thrust style the course is drawn along the current heading at
the current velocity showing where the game object will be if its course
is not modified

*/
void plotCourse(gdImagePtr im, GameObject game_object, int color)
{
    double m2;
    double m1;
    double rad_heading;

    double mid_x,
           mid_y,
           start_x,
           start_y,
           end_x,
           end_y;
    int h1,
        h2;
    int pix_mid_x,
        pix_mid_y,
        pix_start_x,
        pix_start_y,
        pix_end_x,
        pix_end_y;

    int mid_turn;

    /* Set tracking linestyle 
     */
    gdImageSetStyle(im,lcourse,lcourse_size);

    if (real_thrust) {
        /* Real Thrust course plot
         */
        rad_heading = (game_object.heading / 180.0) * M_PI;
        end_x = game_object.x + (game_object.speed * sin(rad_heading));
        end_y = game_object.y + (game_object.speed * cos(rad_heading));
        pix_end_x = (int) Rint((end_x - min_x) * pix_per_unit);
        pix_end_y = im->sy - 1 - (int) Rint((end_y - min_y) * pix_per_unit);;

        /* Draw line. Add course extent to the text box manager if orthogonal, if not don't bother as 
         * angled lines eat up too much map white space
         */
        gdImageLine(im, game_object.cen_x, game_object.cen_y, pix_end_x, pix_end_y,
                    gdStyled);
        if (Line_isOrthogonal(game_object.cen_x, game_object.cen_y, pix_end_x, pix_end_y)) {

            /* as course is orthogonal dilate the box otherwise it has a zero
             * dimension 
             */
            BoxMgr_add(Box_dilate(Box_boxInt(game_object.cen_x, game_object.cen_y, 
                                             pix_end_x, pix_end_y),LINE_DILATION));
        }
    }
    else {
        /* Full Thrust course plot
         */
        mid_turn = game_object.delta_heading - 
            (int) ((double) game_object.delta_heading / 2.0);
        h2 = game_object.heading % 12;
        h1 = (game_object.heading - mid_turn) % 12;
        m2 = game_object.speed / 2.0;
        m1 = m2; 
 
        mid_x = game_object.x - (m2 * sin(M_PI * (double) h2 / 6.0));
        mid_y = game_object.y - (m2 * cos(M_PI * (double) h2 / 6.0));
        start_x = mid_x - (m1 * sin(M_PI * (double) h1 / 6.0));
        start_y = mid_y - (m1 * cos(M_PI * (double) h1 / 6.0));

        pix_mid_x = (int) Rint((mid_x - min_x) * pix_per_unit);
        pix_mid_y = im->sy - 1 - (int) Rint((mid_y - min_y) * pix_per_unit);
        pix_start_x = (int) Rint((start_x - min_x) * pix_per_unit);
        pix_start_y = im->sy - 1 - (int) Rint((start_y - min_y) * pix_per_unit);

        /* Draw line. Add course extent to the text box 
         * manager if orthogonal, if not don't bother as angled lines eat up too 
         * much map white space
         */
        
        gdImageLine(im, game_object.cen_x, game_object.cen_y, pix_mid_x, pix_mid_y,
                    gdStyled);
        if (Line_isOrthogonal(game_object.cen_x,game_object.cen_y,pix_mid_x,
                              pix_mid_y)) {

            /* as course is orthogonal dilate the box otherwise it has a zero
             * dimension 
             */
            BoxMgr_add(Box_dilate(Box_boxInt(game_object.cen_x,game_object.cen_y,
                                             pix_mid_x,pix_mid_y),LINE_DILATION));
        }

        /* add course extent to the text box manager if orthogonal, if not don't 
         * bother as angled lines eat up too much map white space
         */
        gdImageLine(im, pix_mid_x, pix_mid_y, pix_start_x, pix_start_y, gdStyled);
        if (Line_isOrthogonal(pix_mid_x,pix_mid_y,pix_start_x,pix_start_y)) {

            /* as course is orthogonal dilate the box otherwise it has a zero
             * dimension 
             */
            BoxMgr_add(Box_dilate(Box_boxInt(pix_mid_x,pix_mid_y,pix_start_x,
                                             pix_start_y),LINE_DILATION));
        }
    }
}



/*
------------------------------------------------------------------------------
Copy an image into the map image

*/
void copyBackgroundImage(gdImagePtr map, int x, int y, int w, int h, gdImagePtr back)
{
    double aspect_ratio;
    int src_w,
        src_h,
        dy,
        dx;
    if (debug) {
        printf("Background transparency %d\n",gdImageGetTransparent(back));
    }
    /* Turn off any transparency or you'll get nasty holes in the background image
     */
    gdImageColorTransparent(back,-1);
    if (back == NULL) {
        return;
    }
    /* Wallpaper or stretch the background image
     */
    if (wallpaper) {
        /* Wallpaper tiled
         */
        src_w = (int) 1.0 + (w / gdImageSX(back));
        src_h = (int) 1.0 + (h / gdImageSY(back));
        for (dy = 0; (dy < src_h); dy++) {
            for (dx = 0; (dx < src_w); dx++) {
                gdImageCopy(map, back, dx * back->sx, dy * back->sy, 0, 0, back->sx, back->sy);
             }
        }
    }else{ 
        /* Stretch to fit 
         */
        aspect_ratio = (double) w / (double) h;
        src_w = Rint(aspect_ratio * (back->sy));
        src_h = Rint((back->sx) / aspect_ratio);

        if (src_w > (back->sx)) {
            src_w = back->sx;
        } else if (src_h > (back->sy)) {
            src_h = back->sy;
        }
        gdImageCopyResized(map, back, x, y, 0, 0, w, h, src_w, src_h);
    }
}


/*
------------------------------------------------------------------------------
Blend pixel to percentage of target rgb values

*/
void mergePixelColor(gdImagePtr im, int x, int y,
                             int r, int g, int b,
                             int pct_original)
{
    int in_color,
        out_color;
    int out_r,
        out_g,
        out_b;

    in_color = gdImageGetPixel(im, x, y);

    out_r = (gdImageRed(im, in_color) * pct_original
             + r * (100 - pct_original)) / 100;

    out_g = (gdImageGreen(im, in_color) * pct_original
             + g * (100 - pct_original)) / 100;

    out_b = (gdImageBlue(im, in_color) * pct_original
             + b * (100 - pct_original)) / 100;

    out_color = gdImageColorExact(im, out_r, out_g, out_b);
    if (out_color == -1) {
        out_color = gdImageColorAllocate(im, out_r, out_g, out_b);
        if (out_color == -1) {
            out_color = gdImageColorClosest(im, out_r, out_g, out_b);
        }
    }
    gdImageSetPixel(im, x, y, out_color);
}


/*
------------------------------------------------------------------------------
Fade out pixels under box

*/
void fadeBox(gdImagePtr im, Box box, int pct_fade)
{
    int x,
        y;

    for (x = box.min_x; x <= box.max_x; x++) {
        for (y = box.min_y; y <= box.max_y; y++) {
            mergePixelColor(im, x, y, 0, 0, 0, (100 - pct_fade));
        }
    }
}


/*
------------------------------------------------------------------------------
Fade out pixels under circle, uses a scanline and the circle radius

*/
void fadeCircle(gdImagePtr im, int xc, int yc, int r, int pct_fade)
{
    int x,
        y;

    for (x = (xc - r); x <= (xc + r); x++) {
        for (y = (yc - r); y <= (yc + r); y++) {
            if (sqrt((x - xc) * (x - xc) + (y - yc) * (y - yc)) <= r) {
                mergePixelColor(im, x, y, 0, 0, 0, (100 - pct_fade));
            }
        }
    }
}

/* 
------------------------------------------------------------------------------
Make string upper case, modifies the contents but not the base pointer

*/
void strupper(char *s1) 
{
	char *s2;

	s2 = s1;
	while (*s2) {
		*s2 = toupper(*s2);
		s2++;
	}
}

/* 
------------------------------------------------------------------------------
Process Color section in resource file 

if allocate then allocate the colors in the out image palette
 
*/
void colorSection(FILE *r_file, int allocate) 
{
	char line[256];
	while (fgets(line, 255, r_file)) {
		char* s;
		char* d;

		/* strip comments from line 
		 */		  
		strupper(line);
		s = strstr(line,";");
		if (s) {
			*s = '\0';
		}
		
		/* end of section - pop the line back and continue section processing
		 */
		if (strstr(line,"[")) {
			fputs(line,r_file);
			return;
		}
		
		/* extract expected values this is three integers for r g b after an 
         * '=' character 
		 */
		if (strlen(line) > 0 ) {
			d = strstr(line,"=");
			if (d) {
				int r,g,b;
				sscanf(++d,"%d%d%d",&r,&g,&b);

				if (strstr(line,"BACKGROUNDCOLOR")) {
					if ( allocate ) background_color = gdImageColorAllocate(im_out, r, g, b);
				}
				if (strstr(line,"FOREGROUNDCOLOR")) {
					if ( allocate ) foreground_color = gdImageColorAllocate(im_out, r, g, b);
					foreground_rgb.r = r;
					foreground_rgb.g = g;
					foreground_rgb.b = b;
				}
				if (strstr(line,"TITLETEXTCOLOR")) {
					if ( allocate ) title_text_color = gdImageColorAllocate(im_out, r, g, b);
				}
				if (strstr(line,"LABELTEXTCOLOR")) {
					if ( allocate ) label_text_color = gdImageColorAllocate(im_out, r, g, b);
				}
				if (strstr(line,"AXESCOLOR")) {
					if ( allocate ) axes_color =  gdImageColorAllocate(im_out, r, g, b);
				}
				if (strstr(line,"AXESTEXTCOLOR")) {
					if ( allocate ) axes_text_color = gdImageColorAllocate(im_out, r, g, b);
				}
				if (strstr(line,"LEGENDCOLOR")) {
					if ( allocate ) legend_color = gdImageColorAllocate(im_out, r, g, b);
				}
				if (strstr(line,"LEGENDTEXTCOLOR")) {
					if ( allocate ) legend_text_color = gdImageColorAllocate(im_out, r, g, b);
				}
				if (strstr(line,"LEADERCOLOR")) {
					if ( allocate ) leader_color = gdImageColorAllocate(im_out, r, g, b);
				}
				if (strstr(line,"COURSECOLOR")) {
					if ( allocate ) course_color = gdImageColorAllocate(im_out, r, g, b);
				}
				if (strstr(line,"GRIDCOLOR")) {
					if ( allocate ) axes_grid_color = gdImageColorAllocate(im_out, r, g, b);
				}
				if (strstr(line,"LOCUSCOLOR")) {
					if ( allocate ) locus_color = gdImageColorAllocate(im_out, r, g, b);
				}
			}else{
				fprintf(stderr,"**** Warning bad resource file entry \n%s\n",line);
			}
		}
	}
}



/* 
------------------------------------------------------------------------------
Read resource file information
 
*/
void readResource(int allocate) {
	 char line[256];
	 FILE * r_file;

	 if (verbose) printf("Reading resource file %s\n",resource_filename);
	 r_file = fopen(resource_filename, "r");

	 /* read line until EOL || EOF
	  */
     while (fgets(line, 255, r_file)) {
		 char* s;

		 /* strip comments from line 
		  */
		 strupper(line);
         s = strstr(line,";");
         if (s) {
			 *s = '\0';
		 }

		 /* find section header 
		  */
		 if (strstr(line,"[")) {
			 if (strstr(line,"]")) {

				 /* process sections
				  */
				 if (strstr(line,"COLOR")) {
					 colorSection(r_file, allocate);
				 }
			 }else{
				 fprintf(stderr,"**** Error bad resource section %s\n",line);
			 }
		 }
	 }
	 fclose(r_file);
}

/*
------------------------------------------------------------------------------
Load game object images

*/
void loadGameImages ()
{
	FILE* out_file;
    char temp_name[MAX_BUFFER - 1];
	
	/* TODO get the foreground color set to white as a default */
	foreground_rgb.r = 255;
    foreground_rgb.g = 255;
    foreground_rgb.b = 255;
	
	if (resource_filename && color) {
		readResource(0);
	}
	if (verbose) printf("Foreground color is %d %d %d\n", foreground_rgb.r,foreground_rgb.g, foreground_rgb.b);
	

    /* Read and pre-rotate to 12 possible heading positions the game object 
     * gifs. If creating a bitonal map make negative images from the 
     * conventional, white on black. Add the image colors to the color manager
     * so that the main image palette will be the best fit for the images used
     */
    num_classes = 0;
    if (verbose) printf("Reading game object images\n");
    do {
        char game_object_filename[MAX_BUFFER];
        gdImagePtr temp_image=NULL;
        double gif_scale=0.0;
        int heading=0;    
        double angle=0.0;
        int temp_x=0;

        scanf("%s\n",temp_name);
        if (temp_name[0] != '*') {
            class[num_classes].name = (char *) strdup(temp_name);
            scanf("%s\n", temp_name);
            if (image_dir) {
                sprintf(game_object_filename,"%s%s%s",image_dir,SLASH,
                        temp_name);
            } else {
                strcpy(game_object_filename,temp_name);
            }

            in_file = fopen(game_object_filename, "rb");
            if (!in_file) {
                fprintf(stderr,"**** Error unable to load source image %s\n**** Aborting\n",
                        game_object_filename);
                temp_image = 0;
                exit(0);
            } else {
                temp_image = gdImageCreateFromGif(in_file);
                fclose(in_file);
            }
            if (verbose) printf("\t%s %s",game_object_filename,
                   temp_image == NULL ?"not read":"read");

            /* invert bitmap if bitonal 
             */
			int b,w,f; 
            if (!color) {
                b = gdImageColorExact(temp_image,0,0,0);
                if (b == -1) {
                    b = gdImageColorAllocate(temp_image,0,0,0);
                    if (b == -1) {
                        b = gdImageColorClosest(temp_image,0,0,0);
                    }
                }
                w = gdImageColorExact(temp_image,255,255,255);
                if (w == -1) {
                    w = gdImageColorAllocate(temp_image,255,255,255);
                    if (w == -1) {
                        w = gdImageColorClosest(temp_image,255,255,255);
                    }
                }
                changeColor(temp_image,b,w,1);
            }
			
			/* TODO
			 if color and a foreground re-color in effect then change color on the loaded sprite before rotation
			 Note we don't have the foreground color rgb yet - need to refactor that so it's read before 
			*/
			
			/* if colors and the foreground isn't white swap it with white with the just loaded sprite */
			if (color && !(foreground_rgb.r == 255 && foreground_rgb.g == 255 && foreground_rgb.b == 255 )) {
				f = gdImageColorAllocate(temp_image,foreground_rgb.r,foreground_rgb.g,foreground_rgb.b);
                    if (f == -1) {
                        f = gdImageColorClosest(temp_image,foreground_rgb.r,foreground_rgb.g,foreground_rgb.b);
                    }
				w = gdImageColorExact(temp_image,255,255,255);
                if (w == -1) {
                    w = gdImageColorAllocate(temp_image,255,255,255);
                    if (w == -1) {
                        w = gdImageColorClosest(temp_image,255,255,255);
                    }
                }
				changeColor(temp_image,f,w,1);
			}
			
			
			
			
            /* add image palette to color manager
             */
            ColorMgr_getImageColorMap(temp_image, IMAGE_PALETTE_SIZE);

            scanf("%lf\n", &gif_scale);
            for (heading = 0; heading < 12; heading++) {
                angle = ((double) (M_PI / 6.0)) * heading;
                class[num_classes].image[heading] = 
                    spinImage(temp_image, angle, gif_scale, resample);
					if (debug) {
						sprintf(temp_name,"%s%d.gif",class[num_classes].name, heading);
						out_file = fopen(temp_name, "wb");
						gdImageGif(class[num_classes].image[heading], out_file);
						fclose(out_file);
					}
                    if (verbose) printf(".");
            }
            if (verbose) printf("\n");
            gdImageDestroy(temp_image);
            class[num_classes].radius = 
                gdImageSX(class[num_classes].image[0]) / 2;

            scanf("%d\n", &temp_x);
            if (temp_x) {
                index_class[num_indexed_classes] = num_classes;
                num_indexed_classes++;
            }
            num_classes++;
        }
    } while (temp_name[0] != '*');
}


/*
------------------------------------------------------------------------------
Load game object data

*/
void loadGameObjects()
{   
    char temp_name[256];
	float temp;

    /* Get game object data from data file 
     */
    num_game_objects = 0;
    if (verbose) printf("reading game objects\n");
    do {
        GameObject this_game_object;
        int class_num=0;
        int heading=0;

        fgets(temp_name,80,stdin);
        temp_name[strlen(temp_name)-1] = '\0';
        if (temp_name[0] != '*') {
            this_game_object.name = strdup(temp_name);

            scanf("%s\n", temp_name);
            for (class_num = 0; class_num < num_classes; class_num++) {
                if (strcmp(class[class_num].name, temp_name) == 0) {
                    for (heading = 0; heading < 12; heading++) {
                        this_game_object.image[heading] = 
                            class[class_num].image[heading];
                    }
                    this_game_object.radius = class[class_num].radius;
                    break;
                }
            }

            scanf("%f\n", &temp);
            this_game_object.x = (double)temp;
            scanf("%f\n", &temp);
            this_game_object.y = (double)temp;
            this_game_object.cen_x = 
                (int) (0.5 + (this_game_object.x - min_x) * pix_per_unit);
            this_game_object.cen_y = 
                out_y - (int)(0.5 + (this_game_object.y -min_y) * pix_per_unit);
            scanf("%d\n", &(this_game_object.heading));
            scanf("%d\n", &(this_game_object.facing));
            scanf("%f\n", &temp);
            this_game_object.speed = (double)temp;
            scanf("%d\n", &(this_game_object.delta_heading));
         
            /* Store game object and load its color map into the color manager
             */                 
            game_objects[num_game_objects++] = this_game_object;
        }
    } while (temp_name[0] != '*');
}


/*
------------------------------------------------------------------------------
Fade game object background in gif image

*/
void fadeGameObjects() 
{
    int game_object_num;

    /* Fade Game Object background 
     */
    if (verbose) printf("fading background for game objects\n");
    for (game_object_num = 0; game_object_num < num_game_objects; 
         game_object_num++) {
        GameObject this_game_object;
		
        this_game_object = game_objects[game_object_num];

        /* if a background image then fade out the background under the 
         * image so it isn't swamped by the background do this before
         * adding the images or they get faded too
         */
		fadeCircle(im_out, this_game_object.cen_x, this_game_object.cen_y, 
				   this_game_object.radius, GAME_OBJECT_FADE);
	}
}	



/*
------------------------------------------------------------------------------
Draw game objects in new gif image

*/
void drawGameObjects() 
{    
    int temp_x=0;
    int temp_y=0;
    int game_object_num;

    /* Plot Game_Object Images 
     */
    if (verbose) printf("Drawing game objects\n");
    for (game_object_num = 0; game_object_num < num_game_objects; 
         game_object_num++) {
        GameObject this_game_object;

        this_game_object = game_objects[game_object_num];
        temp_x = this_game_object.cen_x - this_game_object.radius;
        temp_y = this_game_object.cen_y - this_game_object.radius;

		/* change foreground color - should be moved to where images are processed for facings  as it craps up AA */
		 
		if (color && foreground_color != NOT_DEFINED) {
			changeForeground
				(this_game_object.image[this_game_object.facing % 12]);
		}

        gdImageCopy(im_out, 
                    this_game_object.image[this_game_object.facing % 12],
                    temp_x, temp_y, 0, 0, this_game_object.radius * 2, 
                    this_game_object.radius * 2);

        /* Plot game object course after image so ship locus is clear
         * as course terminates there
         */
        if (tracking) {
            plotCourse(im_out, this_game_object, course_color);
        }        

        /* Add game object bounding box to text box manager
         */
        BoxMgr_add(Box_boxInt(temp_x,temp_y,
                              temp_x + 2 * this_game_object.radius,
                              temp_y + 2 * this_game_object.radius));
    }
}


/*
------------------------------------------------------------------------------
Read data file header information
 
*/
void readHeader() {
    char temp_name[MAX_BUFFER - 1];
    char background_filename[MAX_BUFFER];
	float temp;

    /* Get Map Title 
     */
    fgets(temp_name, 80, stdin);
    title = strdup(temp_name);
    if (!isprint(title[strlen(title) - 1])) {
        title[strlen(title) - 1] = '\0';
    }
    

    /* Get Background image (if any) and color 
     */
    scanf("%s\n", temp_name);
    if (color) {
        if (temp_name[0] != '-') {
            if (image_dir) {
                sprintf(background_filename,"%s%s%s",image_dir,SLASH,temp_name);
            } else {
                strcpy(background_filename,temp_name);
            }
            in_file = fopen(background_filename, "rb");
            if (!in_file) {
                fprintf(stderr, "**** Error unable to load background image %s\n",
                        background_filename );
                background = NULL;
            } else {
                /* Create background image and add its color map to the oct-tree
                 */
                background = gdImageCreateFromGif(in_file);
                fclose(in_file);
                ColorMgr_getImageColorMap(background, IMAGE_PALETTE_SIZE);
            }
        } else {
            background = NULL;
        }
    }else{
        background = NULL;
    }
    
    /* Get tracking flag 
     */
    scanf("%d\n", &tracking);
    tracking = tracking ? 1 : 0;
    
    /* Get Map Corner Co-Ords 
     */
    scanf("%f\n", &temp);
	min_x = (double)temp;
    scanf("%f\n", &temp);
    min_y = (double)temp;
    scanf("%f\n", &temp);
    max_x  = (double)temp;
    scanf("%f\n", &temp);
    max_y = (double)temp;
    

    /* Get pixels/unit ratio 
     */
    scanf("%f\n", &temp);
    pix_per_unit = (double)temp;
    out_x = (int) (0.5 + pix_per_unit * (max_x - min_x));
    out_y = (int) (0.5 + pix_per_unit * (max_y - min_y));

    /* Announce parameters
     */
    if (verbose) {
        printf("title                %s\n",title);
        printf("map extent           %f.1/%f.1 %f.1/%f.1\n",min_x, min_y, max_x, max_y);
        printf("image resolution     %d x %d\n", out_x, out_y);
        printf("map                  %s\n",color       ? "color" : "bitonal" );
        printf("background           %s\n",background  ? background_filename : "plain" );
        if (background) {
        printf("tiling               %s\n",wallpaper   ? "on" : "off" );
        }
        printf("resampling           %s\n",resample    ? "on" : "off" );    
        printf("object tracks        %s\n",tracking    ? "on" : "off" );
        if (tracking) {
        printf("real thrust plotting %s\n",real_thrust ? "on" : "off" );
        }
        printf("grid                 %s\n",grid        ? "on" : "off" );
        printf("legend               %s\n",legend      ? "on" : "off" );
        printf("\n");
    }
}


/* setStyles()
------------------------------------------------------------------------------
Set up complex linestyles
 
*/
void setStyles()
{
    /* Linestyle definitions & default style set according to color/bitonal
     */
    lcourse[0] = course_color;      
    lcourse[1] = gdTransparent;
    lcourse[2] = gdTransparent;
    lcourse[3] = gdTransparent;
    lgrid[0] = axes_grid_color;           
    lgrid[1] = color ? axes_grid_color : gdTransparent;
    lleader[0] = leader_color;        
    lleader[1] = leader_color;
}


/* getAnnoPosition()
------------------------------------------------------------------------------
Find best position for annotation

Use a radial approach with a 360d rotation leader line increasing in radius 
from the game object radius to the maximum leader length. If a good position 
is found then use it other wise exhaust search and use the best position 
found. 

Take the effect of the leader line from the candidate positions into account 
but it is weighted to be less important than the text clashing

                                                   
                                    r              
                               +----------+--------+ angle +d degrees
                         centre           | text   |      |
                                          +--------+      V
*/
Point getAnnoPosition
    (
    GameObject this_game_object, 
	int text_width, 
	int text_height
    )
{
	Box text_box;
	Box leader_box;
	Point position;
	Point centre;
	Point text_to;
	Point text_from;
    int overlap = 0;
	int min_overlap = INT_MAX;
	int r = 0;
	int d = 0;
	int box_num=0;

	centre.x = this_game_object.cen_x;
	centre.y = this_game_object.cen_y;

	/* grow the radius 1 pixel to the limit
	 */
	for (r=this_game_object.radius; r < (this_game_object.radius + MAX_LEADER); 
		 r++) {

		/* degree increments for this radius the +ve x-axis is 0 degrees
         * -ve y-axis 90 degrees
		 */
		for (d=0; d < 360; d+=ANNO_ANGLE) {
			double angle;

			/* calculate the end of the leader line position and the position
             * of the text as a box at this position. 
			 */
			angle = ((double)d / 180.0) * M_PI;
			text_from.x = Rint ((double)r * cos(angle) + (double)centre.x);
			text_from.y = Rint ((double)r * sin(angle) + (double)centre.y);

			if ( d >= 0 && d <= 90) {
				/* first quadrant hang text box from bottom left
				 */
				text_to.x = text_from.x + text_width; 
				text_to.y = text_from.y + text_height;

            }else if ( d > 90 && d <= 180) {
				/* second quadrant hang text box from top left
				 */	
				text_to.x = text_from.x - text_width; 
				text_to.y = text_from.y + text_height;

			}else if ( d > 180 && d <= 270) {
				/* third quadrant hang text box from top right
				 */
				text_to.x = text_from.x - text_width;
				text_to.y = text_from.y - text_height;

			}else{
				/* fourth quadrant hang text box from bottom right
				 */
				text_to.x = text_from.x + text_width; 
				text_to.y = text_from.y - text_height;
			}
			text_box = Box_boxPoint(text_from,text_to);
			/*** if (debug) Box_draw(text_box,im_out); ***/
			leader_box = Box_boxPoint(centre, 
									  Box_closestMidPoint(text_box, centre)); 
			/* get amount of clash with existing boxes and keep the best so 
			 * far take the leader line into account in determining the clash
             * amount but weight it differently than for text. 
			 * if there is no clash then use the current point now
			 */
			overlap = 0;
			for (box_num = 0; box_num < BoxMgr_numberBoxes(); box_num++) {
				overlap += Box_overlap(BoxMgr_get(box_num),text_box);
				overlap += (Box_overlap(BoxMgr_get(box_num),leader_box) * 
							LEADER_WEIGHTING);
                }

			/* if the best position so far, calculate the best text position 
			 * as the top left of the text box as this is postion gd uses to 
			 * place text images
			 */
			if (overlap == 0) {
				/* this box doesn't clash so use it now
				 */
				position.x = text_box.min_x;
				position.y = text_box.min_y;
				goto FOUND_IT;
			}else{
				/* if this is the best box yet then store the position
				 */
				if (overlap < min_overlap) {
					min_overlap = overlap;
					position.x = text_box.min_x;
					position.y = text_box.min_y;
				}
			}
		}
	}
	FOUND_IT:
	return position;
}


/* annotateGameObjects()
------------------------------------------------------------------------------
Annotate the game objects with their names/id performing clash resolution
 
*/
void annotateGameObjects()
{
    Box text_box;
	GameObject this_game_object;
	Point anno_position;
	Point leader_end;
	Point centre;
    int game_object_num;
    int text_width=0;
    int text_height=0;

    if (verbose) printf("annotating game objects\n");
    for (game_object_num = 0; game_object_num < num_game_objects; game_object_num++) {

		this_game_object = game_objects[game_object_num];
		centre.x = this_game_object.cen_x;
		centre.y = this_game_object.cen_y; 
		
		if (strlen(this_game_object.name) > 0) {		
			text_width = strlen(this_game_object.name) * 
				((gdFont *) gdFontSmall)->w;
			text_height = ((gdFont *) gdFontSmall)->h;
			anno_position = getAnnoPosition(this_game_object, text_width, 
											text_height);

			/* Plot name and add to clash box manager, dilate the text box stored
			 * so that a small border exists around text blocks, use the original
			 * text box for the leader line calculation
			 */
			text_box = Box_boxInt(anno_position.x,anno_position.y, 
								  anno_position.x + text_width,
								  anno_position.y + text_height);
			if (background) {
				fadeBox(im_out, text_box, LABEL_FADE);
			}
			gdImageString(im_out, gdFontSmall, anno_position.x, anno_position.y,
						  this_game_object.name, label_text_color);
			text_box = Box_dilate(text_box, TEXT_DILATION);
			BoxMgr_add(text_box); 

			/* Draw leader line and add to clash box manager
			 */
			
			if (leader_color != NOT_DEFINED) {
				leader_end = Box_closestMidPoint(text_box, centre);

				gdImageSetStyle(im_out,lleader,lleader_size);
				gdImageLine(im_out, leader_end.x, leader_end.y, centre.x, centre.y,
							gdStyled);
				if (Line_isOrthogonal(leader_end.x, leader_end.y, centre.x, centre.y)){
            
					/* line is orthogonal so dilate the clash box otherwise it has a 
					 * zero dimension box
					 */
					BoxMgr_add(Box_dilate(Box_boxPoint(leader_end, centre),
										  LINE_DILATION));
				}else{
					/* line not orthogonal use as is for clash box
					 */
					BoxMgr_add(Box_boxPoint(leader_end,centre));
				}
			}
		}

		/* Add locus spot at the center of the image
         */	
		if (locus_color != NOT_DEFINED) {
			gdImageSetPixel(im_out,centre.x, centre.y, locus_color);
		}

    } /* end for */
}


/*
------------------------------------------------------------------------------
Draw map title 
 
*/
void drawTitle()
{
    int temp_x=0;
    int temp_y=0;
    int title_width=0;
    int title_height=0;
    int min_overlaps;
    int overlaps;
    int box_num=0;
    int title_x=0;
    int title_y=0;
    Box temp_box = { min_x = 0, min_y = 0, max_x = 0, max_y = 0 };
    Box title_box = { min_x = 0, min_y = 0, max_x = 0, max_y = 0 };

    /* Find best place to write the title at the top of the map 
     */
    if (verbose) printf("Drawing title\n");
    title_height = ((gdFont *) gdFontGiant)->h;
    title_width = ((gdFont *) gdFontGiant)->w * strlen(title);
    min_overlaps = INT_MAX;
    title_x = 0;
    title_y = 0;
    for (temp_y = title_height / 2; 
         temp_y < (im_out->sy - title_height * 1.5); 
         temp_y += title_height / 2) {
        for (temp_x = ((gdFont *) gdFontGiant)->w; 
             (temp_x + title_width + ((gdFont *) gdFontGiant)->w) < im_out->sx; 
             temp_x++) {

            overlaps = 0;
            temp_box = Box_boxInt(temp_x,temp_y,temp_x + title_width,
                                  temp_y + ((gdFont *) gdFontGiant)->h);
            for (box_num = 0; box_num < BoxMgr_numberBoxes(); box_num++) {
                overlaps += Box_overlap(BoxMgr_get(box_num),temp_box);
            }
            if (overlaps < min_overlaps) {
                min_overlaps = overlaps;
                title_x = temp_x;
                title_y = temp_y;
                title_box = temp_box;
            }
        }
    }
    
    /* Having found a good place for the title, write it 
     */
    if (background) {
        fadeBox(im_out, title_box, TITLE_FADE);
    }
    gdImageString(im_out, gdFontGiant, title_x, title_y, title, 
				  title_text_color);
    BoxMgr_add(Box_dilate(title_box,TEXT_DILATION));    
}


/*
------------------------------------------------------------------------------
Draw map legend

*/
void drawLegend ()
{
    int index_num;
    int max_text_w;
    int max_image_r;
    int total_h;
    int legend_x;
    int legend_y;
    int ypos;
    Box temp_box;

    /* Estimate size of legend 
     */
    for (index_num = 0, max_text_w = 0, max_image_r = 0, total_h = 0;
         index_num < num_indexed_classes;
         index_num++) {
        if (strlen(class[index_class[index_num]].name) > max_text_w) {
            max_text_w = strlen(class[index_class[index_num]].name);
        }
        if (class[index_class[index_num]].radius > max_image_r) {
            max_image_r = class[index_class[index_num]].radius;
        }
        total_h += MAX(class[index_class[index_num]].radius * 2, 
                       ((gdFont *) gdFontSmall)->h);
    }
    max_text_w++;
    
    /* Find a good place to put the legend 
     * for the moment, assume the top right 
     */
    legend_x = im_out->sx - ((max_image_r * 2) + max_text_w * 
                             ((gdFont *) gdFontSmall)->w) - 15;
    legend_y = 30;
    
    /* Draw Legend 
     */
    if (verbose) printf("drawing legend\n");
    if (background) {
        temp_box = Box_boxInt(legend_x, legend_y,
                              legend_x + ((max_image_r * 2) + max_text_w * 
                                          ((gdFont *) gdFontSmall)->w),
                              legend_y + total_h);
        fadeBox(im_out,temp_box,LEGEND_FADE);
    }
    gdImageRectangle(im_out, legend_x, legend_y,
                     legend_x + (max_image_r * 2) + max_text_w * 
                     ((gdFont *) gdFontSmall)->w,
                     legend_y + total_h, legend_color);
    for (index_num = 0, ypos = legend_y; index_num < num_indexed_classes;
         index_num++) {

		/* change foreground color 
		 */
		if (color && foreground_color != NOT_DEFINED) {
			changeForeground
				(class[index_class[index_num]].image[3]);
		}
        gdImageCopy(im_out, class[index_class[index_num]].image[3],
                    legend_x + max_text_w * 
                    ((gdFont *) gdFontSmall)->w + max_image_r - 
                    class[index_class[index_num]].radius, ypos, 0, 0,
                    class[index_class[index_num]].radius * 2, 
                    class[index_class[index_num]].radius * 2);
        gdImageString(im_out, gdFontSmall, legend_x + (max_text_w - 
                       strlen(class[index_class[index_num]].name))
                      * ((gdFont *) gdFontSmall)->w,
                      ypos + class[index_class[index_num]].radius - 
                      ((gdFont *) gdFontSmall)->h / 2,
                      class[index_class[index_num]].name, legend_text_color);
        ypos += MAX(class[index_class[index_num]].radius * 2, 
                    ((gdFont *) gdFontSmall)->h);
    }
}


/*
------------------------------------------------------------------------------
Create empty map image and default palette

*/
void createImage()
{
    /* Create map image first color allocated is the background color so
     * allocate according to color or bitonal. Allocate other map colors
     * according to color or bitonal map
     */
    im_out = gdImageCreate(out_x, out_y);

    /* Read the color data from the resource file and allocate them in the image
	 * palette
	 */
    if (resource_filename && color) {
		readResource(1);
	}

	/* set default colors if not already defined
	 */
    if (color) {
        if (background_color == NOT_DEFINED) { 
			background_color = gdImageColorAllocate(im_out, 0, 0, 0);
		}
		if (title_text_color == NOT_DEFINED) { 
			title_text_color = gdImageColorAllocate(im_out, 96, 255, 96);
		}
		if (label_text_color == NOT_DEFINED) { 
			label_text_color = gdImageColorAllocate(im_out, 96, 255, 96);
		}
		/* if (leader_color == NOT_DEFINED) { 		
			leader_color = label_text_color;
		}
		if (locus_color == NOT_DEFINED) {         
			locus_color = label_text_color;
		}
		*/
		if (course_color == NOT_DEFINED) {         
			course_color = gdImageColorAllocate(im_out, 96, 96, 255);
		}
		if (axes_color == NOT_DEFINED) {         
			axes_color = gdImageColorAllocate(im_out, 64, 196, 64);
		}		
		if (axes_grid_color == NOT_DEFINED) {         
			axes_grid_color = gdImageColorAllocate(im_out, 33, 100, 33);
		}
		if (axes_text_color == NOT_DEFINED) {         
			axes_text_color = axes_color;
		}
		if (legend_color == NOT_DEFINED) {         
			legend_color = axes_color; 
		}
        if (legend_text_color == NOT_DEFINED) {   
            legend_text_color = axes_color;
		}
    }else{
        /* for a bitonal map the resource colors are ignored else its not
		 * bitonal
		 */
		background_color = gdImageColorAllocate(im_out, 255, 255, 255);
		foreground_color = gdImageColorAllocate(im_out, 0, 0, 0);
		title_text_color = foreground_color;
		label_text_color = foreground_color;
		/* leader_color     = foreground_color;		
		locus_color      = background_color;
		*/
		course_color     = foreground_color;
		axes_color       = foreground_color;		
		axes_grid_color  = foreground_color;
		axes_text_color  = foreground_color;
		legend_color     = foreground_color;
		legend_text_color= foreground_color;
    }
    /* reserve most frequent colors in image palette and delete the color manager
     */
    ColorMgr_allocateImageColors(im_out);
}


/*
------------------------------------------------------------------------------
Write Image file

*/
void writeImage() 
{
    FILE *out_file=NULL;  
    char outfname[256];

    /* DEBUG draw clash detection boxes
     */
    if (debug) BoxMgr_draw(im_out);

    /* Write map to interlaced gif image file and deallocate the image
     */
    if (verbose) printf("writing image file\n");
    gdImageInterlace(im_out, 1);
    if (gif_filename) {
        strcpy(outfname,gif_filename);
    }else{
        strcpy(outfname,"ftmap.gif");
    }
    out_file = fopen(outfname, "wb");
    gdImageGif(im_out, out_file);
    fclose(out_file);
    gdImageDestroy(im_out);
}


/*
******************************************************************************
ftmap main program

******************************************************************************
*/
int main(int argc, char* argv[])
{
    /* Get arguments
     */
    getArgs(argc,argv);

    /* Announce
     */
    fprintf(stdout,"%s v%s\n",PROGRAM, VERSION);
    fprintf(stdout,"%s\n\n",COPYRIGHT);

    /* Read the header information from the data file
     */
    readHeader();
	
    /* Load the game object images
     */
    loadGameImages();
    
    /* Load the game object data
     */
    loadGameObjects();
	
	/* Create map image from resources file colors
     */
    createImage();

    /* Set up complex linestyles
     */
    setStyles();

   /* Copy background image to map image
    */
    if (background) {
        copyBackgroundImage(im_out, 0, 0, out_x, out_y, background);
    }

    /* Draw map reference axes
     */
    drawMapAxes(im_out, axes_text_color, axes_color, pix_per_unit, min_x, 
				min_y);    

	/* If a background fade it under the game objects so they are easier to 
	 * see against it. Do all the fading first otherwise the game object images
     * will be unintentionally modified. Then draw the game object images and 
	 * tracking lines and annotate the game objects so that the text doesn't 
     * overlap
     */
	if (background) {
		fadeGameObjects();
	}
    drawGameObjects();
    annotateGameObjects();  

    /* Draw the map title and optional legend doing minimal clash detection
     */
    drawTitle();
    if (legend) {
        drawLegend();
    }  

    /* Write map image
     */
    writeImage();
}

/******************************************************************************/










