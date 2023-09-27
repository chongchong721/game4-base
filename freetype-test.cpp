#include <ft2build.h>
#include FT_FREETYPE_H
#include <math.h>
#include <hb.h>
#include <hb-ft.h>
#include <iostream>

// Now we can get free_type glyph from a text. Only need to draw those glyphs using opengl


//This file exists to check that programs that use freetype / harfbuzz link properly in this base code.
//You probably shouldn't be looking here to learn to use either library.

#define FONT_SIZE 36
#define MARGIN (FONT_SIZE * .5)




/***********************************************************************
*************/

#define WIDTH   640
#define HEIGHT  480


/* origin is the upper left corner */
unsigned char image[HEIGHT][WIDTH];


/* Replace this function with something useful. */

void
draw_bitmap( FT_Bitmap*  bitmap,
             FT_Int      x,
             FT_Int      y)
{
  FT_Int  i, j, p, q;
  FT_Int  x_max = x + bitmap->width;
  FT_Int  y_max = y + bitmap->rows;


  /* for simplicity, we assume that `bitmap->pixel_mode' */
  /* is `FT_PIXEL_MODE_GRAY' (i.e., not a bitmap font)   */

  for ( i = x, p = 0; i < x_max; i++, p++ )
  {
    for ( j = y, q = 0; j < y_max; j++, q++ )
    {
      if ( i < 0      || j < 0       ||
           i >= WIDTH || j >= HEIGHT )
        continue;

      image[j][i] |= bitmap->buffer[q * bitmap->width + p];
    }
  }
}


void
show_image( void )
{
  int  i, j;


  for ( i = 0; i < HEIGHT; i++ )
  {
    for ( j = 0; j < WIDTH; j++ )
      putchar( image[i][j] == 0 ? ' '
                                : image[i][j] < 128 ? '+'
                                                    : '*' );
    putchar( '\n' );
  }
}

/***********************************************************************/






int main(int argc, char **argv) {
	const char *fontfile;
	const char *text;
	fontfile = argv[1];
	text = argv[2];

	FT_Library library;
	FT_Face ft_face;
	FT_Error ft_error;
	FT_GlyphSlot  slot;

	FT_Matrix     matrix;                 /* transformation matrix */
  	FT_Vector     pen;  

	double        angle = ( 25.0 / 360 ) * 3.14159 * 2;

	int           target_height = HEIGHT;
	unsigned int  i;


	FT_Init_FreeType( &library );
	ft_error = FT_New_Face( library, fontfile, 0, &ft_face );/* create face object */
  	/* error handling omitted */

	if(ft_error){
		printf("error\n");
	}

  	/* use 50pt at 100dpi */
  	ft_error = FT_Set_Char_Size( ft_face, 50 * 64, 0,
                            100, 0 );    

	hb_font_t *hb_font;
	hb_font = hb_ft_font_create(ft_face,NULL);

	hb_buffer_t *buf = hb_buffer_create();
	hb_buffer_add_utf8(buf,text,-1,0,-1);
	hb_buffer_guess_segment_properties(buf);

	hb_shape(hb_font,buf,NULL,0);

	unsigned int glyph_count;
	//unsigned int len = hb_buffer_get_length (buf);
  	hb_glyph_info_t *info = hb_buffer_get_glyph_infos (buf, &glyph_count);
  	//hb_glyph_position_t *pos = hb_buffer_get_glyph_positions (buf, &glyph_count);





	slot = ft_face->glyph;

	/* set up matrix */
	matrix.xx = (FT_Fixed)( cos( angle ) * 0x10000L );
	matrix.xy = (FT_Fixed)(-sin( angle ) * 0x10000L );
	matrix.yx = (FT_Fixed)( sin( angle ) * 0x10000L );
	matrix.yy = (FT_Fixed)( cos( angle ) * 0x10000L );

	/* the pen position in 26.6 cartesian space coordinates; */
	/* start at (300,200) relative to the upper left corner  */
	pen.x = 300 * 64;
	pen.y = ( target_height - 200 ) * 64;

	for ( i = 0; i < glyph_count; i++ )
	{
		/* set transformation */
		FT_Set_Transform( ft_face, &matrix, &pen );

		//https://github.com/tangrams/harfbuzz-example/blob/master/src/freetypelib.cpp
		// Create FT_glyph
		auto glyphindex = info[i].codepoint;
			FT_Load_Glyph(
				ft_face,
				glyphindex,
				FT_LOAD_RENDER  // FT_LOAD_DEFAULT
		);



		/* now, draw to our target surface (convert position) */
		draw_bitmap( &slot->bitmap,
					slot->bitmap_left,
					target_height - slot->bitmap_top );

		/* increment pen position */
		pen.x += slot->advance.x;
		pen.y += slot->advance.y;
	}

	show_image();

	FT_Done_Face    ( ft_face );
	FT_Done_FreeType( library );






	std::cout << "It worked?" << std::endl;
}
