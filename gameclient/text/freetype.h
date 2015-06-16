typedef signed long FT_Fixed;
typedef unsigned short FT_UShort;
typedef signed int FT_Int;
typedef signed long FT_Long;
typedef signed long FT_Pos;

typedef struct
{
    FT_Pos  x;
    FT_Pos  y;

}FT_Vector;

typedef struct
{
    FT_Fixed  xx, xy;
    FT_Fixed  yx, yy;
}FT_Matrix;

typedef struct
{
    short       n_contours;      /* number of contours in glyph        */
    short       n_points;        /* number of points in the glyph      */

    FT_Vector*  points;          /* the outline's points               */
    char* tags;            /* the points flags                   */
    short* contours;        /* the contour end points             */

    int flags;           /* outline masks                      */
}FT_Outline;

typedef struct  FT_Bitmap_
  {
    int             rows;
    int             width;
    int             pitch;
    unsigned char*  buffer;
    short           num_grays;
    char            pixel_mode;
    char            palette_mode;
    void*           palette;

  } FT_Bitmap;

   typedef struct  FT_BBox_
  {
    FT_Pos  xMin, yMin;
    FT_Pos  xMax, yMax;

  } FT_BBox;

  typedef struct  FT_Span_
  {
    short           x;
    unsigned short  len;
    unsigned char   coverage;

  } FT_Span;

  typedef void
  (*FT_SpanFunc)( int             y,
                  int             count,
                  const FT_Span*  spans,
                  void*           user );

    #define FT_Raster_Span_Func  FT_SpanFunc



    typedef int
  (*FT_Outline_MoveToFunc)( const FT_Vector*  to,
                            void*             user );

#define FT_Outline_MoveTo_Func  FT_Outline_MoveToFunc

  typedef int
  (*FT_Outline_LineToFunc)( const FT_Vector*  to,
                            void*             user );

#define FT_Outline_LineTo_Func  FT_Outline_LineToFunc

typedef int
  (*FT_Outline_ConicToFunc)( const FT_Vector*  control,
                             const FT_Vector*  to,
                             void*             user );

#define FT_Outline_ConicTo_Func  FT_Outline_ConicToFunc

typedef int
  (*FT_Outline_CubicToFunc)( const FT_Vector*  control1,
                             const FT_Vector*  control2,
                             const FT_Vector*  to,
                             void*             user );

#define FT_Outline_CubicTo_Func  FT_Outline_CubicToFunc

typedef struct  FT_Outline_Funcs_
  {
    FT_Outline_MoveToFunc   move_to;
    FT_Outline_LineToFunc   line_to;
    FT_Outline_ConicToFunc  conic_to;
    FT_Outline_CubicToFunc  cubic_to;

    int                     shift;
    FT_Pos                  delta;

  } FT_Outline_Funcs;

  typedef struct  FT_Raster_Params_
  {
    const FT_Bitmap*        target;
    const void*             source;
    int                     flags;
    FT_SpanFunc             gray_spans;
    FT_SpanFunc             black_spans;  /* doesn't work! */
    void*                   user;
    FT_BBox                 clip_box;

  } FT_Raster_Params;

  typedef struct FT_RasterRec_*  FT_Raster;

#define FT_IMAGE_TAG( value, _x1, _x2, _x3, _x4 )  \
          value = ( ( (unsigned long)_x1 << 24 ) | \
                    ( (unsigned long)_x2 << 16 ) | \
                    ( (unsigned long)_x3 << 8  ) | \
                      (unsigned long)_x4         )

  typedef enum  FT_Glyph_Format_
  {
    FT_IMAGE_TAG( FT_GLYPH_FORMAT_NONE, 0, 0, 0, 0 ),

    FT_IMAGE_TAG( FT_GLYPH_FORMAT_COMPOSITE, 'c', 'o', 'm', 'p' ),
    FT_IMAGE_TAG( FT_GLYPH_FORMAT_BITMAP,    'b', 'i', 't', 's' ),
    FT_IMAGE_TAG( FT_GLYPH_FORMAT_OUTLINE,   'o', 'u', 't', 'l' ),
    FT_IMAGE_TAG( FT_GLYPH_FORMAT_PLOTTER,   'p', 'l', 'o', 't' )

  } FT_Glyph_Format;

  typedef int
  (*FT_Raster_NewFunc)( void*       memory,
                        FT_Raster*  raster );

#define FT_Raster_New_Func  FT_Raster_NewFunc

  typedef void
  (*FT_Raster_DoneFunc)( FT_Raster  raster );

#define FT_Raster_Done_Func  FT_Raster_DoneFunc

  typedef void
  (*FT_Raster_ResetFunc)( FT_Raster       raster,
                          unsigned char*  pool_base,
                          unsigned long   pool_size );

#define FT_Raster_Reset_Func  FT_Raster_ResetFunc

  typedef int
  (*FT_Raster_SetModeFunc)( FT_Raster      raster,
                            unsigned long  mode,
                            void*          args );

#define FT_Raster_Set_Mode_Func  FT_Raster_SetModeFunc

  typedef int
  (*FT_Raster_RenderFunc)( FT_Raster                raster,
                           const FT_Raster_Params*  params );

#define FT_Raster_Render_Func  FT_Raster_RenderFunc

 typedef struct  FT_Raster_Funcs_
  {
    FT_Glyph_Format        glyph_format;
    FT_Raster_NewFunc      raster_new;
    FT_Raster_ResetFunc    raster_reset;
    FT_Raster_SetModeFunc  raster_set_mode;
    FT_Raster_RenderFunc   raster_render;
    FT_Raster_DoneFunc     raster_done;

  } FT_Raster_Funcs;

  typedef enum  FT_Pixel_Mode_
  {
    FT_PIXEL_MODE_NONE = 0,
    FT_PIXEL_MODE_MONO,
    FT_PIXEL_MODE_GRAY,
    FT_PIXEL_MODE_GRAY2,
    FT_PIXEL_MODE_GRAY4,
    FT_PIXEL_MODE_LCD,
    FT_PIXEL_MODE_LCD_V,

    FT_PIXEL_MODE_MAX      /* do not remove */

  } FT_Pixel_Mode;

  #define FT_CURVE_TAG( flag )  ( flag & 3 )

  #define FT_CURVE_TAG_ON            1
#define FT_CURVE_TAG_CONIC         0
#define FT_CURVE_TAG_CUBIC         2

#define FT_RASTER_FLAG_DEFAULT  0x0
#define FT_RASTER_FLAG_AA       0x1
#define FT_RASTER_FLAG_DIRECT   0x2
#define FT_RASTER_FLAG_CLIP     0x4

#define FT_OUTLINE_NONE             0x0
#define FT_OUTLINE_OWNER            0x1
#define FT_OUTLINE_EVEN_ODD_FILL    0x2
#define FT_OUTLINE_REVERSE_FILL     0x4
#define FT_OUTLINE_IGNORE_DROPOUTS  0x8
#define FT_OUTLINE_SMART_DROPOUTS   0x10
#define FT_OUTLINE_INCLUDE_STUBS    0x20

#define FT_OUTLINE_HIGH_PRECISION   0x100
#define FT_OUTLINE_SINGLE_PASS      0x200


#define FT2_BUILD_LIBRARY
#define FT_ABS( a )     ( (a) < 0 ? -(a) : (a) )

#define FT_UNUSED(x)

extern const FT_Raster_Funcs ft_grays_raster;

