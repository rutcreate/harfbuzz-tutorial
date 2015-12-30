#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <hb.h>
#include <hb-ft.h>
#include <cairo.h>
#include <cairo-ft.h>
#include <ctype.h>

#define FONT_SIZE 36
#define MARGIN (FONT_SIZE * .5)

size_t trimwhitespace(char *out, size_t len, const char *str)
{
  if(len == 0)
    return 0;

  const char *end;
  size_t out_size;

  // Trim leading space
  while(isspace(*str)) str++;

  if(*str == 0)  // All spaces?
  {
    *out = 0;
    return 1;
  }

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace(*end)) end--;
  end++;

  // Set output size to minimum of trimmed string length and buffer size minus 1
  out_size = (end - str) < len-1 ? (end - str) : len-1;

  // Copy trimmed string and add null terminator
  memcpy(out, str, out_size);
  out[out_size] = 0;

  return out_size;
}

char *repl_str(const char *str, const char *old, const char *new) {

  /* Adjust each of the below values to suit your needs. */

  /* Increment positions cache size initially by this number. */
  size_t cache_sz_inc = 16;
  /* Thereafter, each time capacity needs to be increased,
   * multiply the increment by this factor. */
  const size_t cache_sz_inc_factor = 3;
  /* But never increment capacity by more than this number. */
  const size_t cache_sz_inc_max = 1048576;

  char *pret, *ret = NULL;
  const char *pstr2, *pstr = str;
  size_t i, count = 0;
  ptrdiff_t *pos_cache = NULL;
  size_t cache_sz = 0;
  size_t cpylen, orglen, retlen, newlen, oldlen = strlen(old);

  /* Find all matches and cache their positions. */
  while ((pstr2 = strstr(pstr, old)) != NULL) {
    count++;

    /* Increase the cache size when necessary. */
    if (cache_sz < count) {
      cache_sz += cache_sz_inc;
      pos_cache = realloc(pos_cache, sizeof(*pos_cache) * cache_sz);
      if (pos_cache == NULL) {
        goto end_repl_str;
      }
      cache_sz_inc *= cache_sz_inc_factor;
      if (cache_sz_inc > cache_sz_inc_max) {
        cache_sz_inc = cache_sz_inc_max;
      }
    }

    pos_cache[count-1] = pstr2 - str;
    pstr = pstr2 + oldlen;
  }

  orglen = pstr - str + strlen(pstr);

  /* Allocate memory for the post-replacement string. */
  if (count > 0) {
    newlen = strlen(new);
    retlen = orglen + (newlen - oldlen) * count;
  } else  retlen = orglen;
  ret = malloc(retlen + 1);
  if (ret == NULL) {
    goto end_repl_str;
  }

  if (count == 0) {
    /* If no matches, then just duplicate the string. */
    strcpy(ret, str);
  } else {
    /* Otherwise, duplicate the string whilst performing
     * the replacements using the position cache. */
    pret = ret;
    memcpy(pret, str, pos_cache[0]);
    pret += pos_cache[0];
    for (i = 0; i < count; i++) {
      memcpy(pret, new, newlen);
      pret += newlen;
      pstr = str + pos_cache[i] + oldlen;
      cpylen = (i == count-1 ? orglen : pos_cache[i+1]) - pos_cache[i] - oldlen;
      memcpy(pret, pstr, cpylen);
      pret += cpylen;
    }
    ret[retlen] = '\0';
  }

end_repl_str:
  /* Free the cache and return the post-replacement string,
   * which will be NULL in the event of an error. */
  free(pos_cache);
  return ret;
}

int main(int argc, char **argv)
{
  if (argc < 9)
  {
    fprintf (stderr, "usage: text2image [ttf font file] [font size] [line spacing] [color without #] [alignment left,middle,right] [max width] [output file] text\n");
    exit (1);
  }

  int buffer_len = 2048;
  if (argc > 9) {
    buffer_len = atoi(argv[9]);
  }
  // printf("buffer len: %s, %d\n", argv[6], argc);

  const char *fontfile;
  int font_size;
  double margin;
  char *output_file;
  char *color;
  char *alignment;
  char converted[buffer_len];
  char input1[buffer_len];
  char input2[buffer_len];
  char input3[buffer_len];
  int line_spacing;
  int max_width;
  int start_text_index = 5;

  fontfile = argv[1];
  font_size = atoi(argv[2]);
  margin = font_size * 0.5;
  line_spacing = atoi(argv[3]);
  color = argv[4];

  alignment = argv[5];
  max_width = atoi(argv[6]);
  output_file = argv[7];
  strcpy(converted, repl_str(argv[8], "<quote>", "\""));
  strcpy(converted, repl_str(converted, "<squote>", "'"));
  
  strcpy(input1, converted);
  strcpy(input2, converted);
  strcpy(input3, converted);

  const char s[2] = "\n";
  const char spaces[3] = " \t";
  char *token;

  /* Initialize FreeType and create FreeType font face. */
  FT_Library ft_library;
  FT_Face ft_face;
  FT_Error ft_error;

  if ((ft_error = FT_Init_FreeType (&ft_library)))
    abort();
  if ((ft_error = FT_New_Face (ft_library, fontfile, 0, &ft_face)))
    abort();
  if ((ft_error = FT_Set_Char_Size (ft_face, font_size*64, font_size*64, 0, 0)))
    abort();

  /* Create hb-ft font. */
  hb_font_t *hb_font;
  hb_font = hb_ft_font_create (ft_face, NULL);

  /* ==========================================================================================
   * Count num lines.
   * ========================================================================================== */
  double canvas_width = 0;
  double canvas_height = 2 * margin;
  int num_lines = 0;

  char new_input[4096];
  double current_width = font_size;
  if (max_width > 0)
  {
    // Find width for space.
    // Create hb-ftbuffer and populate.
    hb_buffer_t *hb_buffer_space;
    hb_buffer_space = hb_buffer_create ();
    hb_buffer_add_utf8 (hb_buffer_space, " ", -1, 0, -1);
    hb_buffer_guess_segment_properties (hb_buffer_space);

    // Shape it!
    hb_shape (hb_font, hb_buffer_space, NULL, 0);

    // Get glyph information and positions out of the buffer.
    // unsigned int len = hb_buffer_get_length (hb_buffer_space);
    // hb_glyph_info_t *info_space = hb_buffer_get_glyph_infos (hb_buffer_space, NULL);
    hb_glyph_position_t *pos_space = hb_buffer_get_glyph_positions (hb_buffer_space, NULL);
    double space_width = pos_space[0].x_advance / 64.;

    token = strtok(input1, spaces);
    while (token != NULL)
    {
      char t[4096];
      strcpy(t, token);
      strcat(t, " ");

      // Create hb-ftbuffer and populate.
      hb_buffer_t *hb_buffer;
      hb_buffer = hb_buffer_create ();
      hb_buffer_add_utf8 (hb_buffer, t, -1, 0, -1);
      hb_buffer_guess_segment_properties (hb_buffer);

      // Shape it!
      hb_shape (hb_font, hb_buffer, NULL, 0);

      // Get glyph information and positions out of the buffer.
      unsigned int len = hb_buffer_get_length (hb_buffer);
      hb_glyph_info_t *info = hb_buffer_get_glyph_infos (hb_buffer, NULL);
      hb_glyph_position_t *pos = hb_buffer_get_glyph_positions (hb_buffer, NULL);

      // Calculate.
      double width = 0;
      for (unsigned int i = 0; i < len; i++)
      {
        width += pos[i].x_advance / 64.;
      }

      if (current_width + width > max_width)
      {
        // Reset.
        if (current_width + width - space_width > max_width)
        {
          current_width = font_size + width;
          strcat(new_input, "\n");
          strcat(new_input, t);
        }
        else
        {
          strcat(new_input, token);
          current_width += width;
        }
      }
      else
      {
        strcat(new_input, t);
        current_width += width;
      }

      token = strtok(NULL, spaces);
    }

    strcpy(input1, new_input);
    strcpy(input2, new_input);
    strcpy(input3, new_input);
  }

  token = strtok(input1, s);
  while (token != NULL)
  {
    num_lines++;
    token = strtok(NULL, s);
  }

  /* ==========================================================================================
   * Calculate canvas size.
   * ========================================================================================== */
  int count = 0;
  int num_lines_support = 100;
  double lines_width[num_lines_support];

  token = strtok(input2, s);
  while (token != NULL)
  {
    // Create hb-ftbuffer and populate.
    hb_buffer_t *hb_buffer;
    hb_buffer = hb_buffer_create ();
    hb_buffer_add_utf8 (hb_buffer, token, -1, 0, -1);
    hb_buffer_guess_segment_properties (hb_buffer);

    // Shape it!
    hb_shape (hb_font, hb_buffer, NULL, 0);

    // Get glyph information and positions out of the buffer.
    unsigned int len = hb_buffer_get_length (hb_buffer);
    hb_glyph_info_t *info = hb_buffer_get_glyph_infos (hb_buffer, NULL);
    hb_glyph_position_t *pos = hb_buffer_get_glyph_positions (hb_buffer, NULL);

    // Calculate.
    double width = 2 * margin;
    double height = 2 * margin;
    for (unsigned int i = 0; i < len; i++)
    {
      width  += pos[i].x_advance / 64.;
      height -= pos[i].y_advance / 64.;
    }
    if (HB_DIRECTION_IS_HORIZONTAL (hb_buffer_get_direction(hb_buffer)))
      height += font_size;
    else
      width  += font_size;

    lines_width[count] = width;

    if (width > canvas_width)
    {
      canvas_width = width;
    }
    
    canvas_height += font_size;

    if (count < num_lines - 1)
    {
      canvas_height += line_spacing;
    }

    count++;
    token = strtok(NULL, s);
  }

  double lines_padding[num_lines_support];
  for (int i = 0; i < num_lines_support; i++)
  {
    if (lines_width[i] == 0) break;

    if (strcmp(alignment, "center") == 0)
    {
      lines_padding[i] = (canvas_width - lines_width[i]) * 0.5;
    }
    else if (strcmp(alignment, "right") == 0)
    {
      lines_padding[i] = canvas_width - lines_width[i];
    }
    else
    {
      lines_padding[i] = 0;
    }
  }

  /* Set up cairo surface. */
  cairo_surface_t *cairo_surface;
  cairo_surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                ceil(canvas_width),
                ceil(canvas_height));
  cairo_t *cr;
  cr = cairo_create (cairo_surface);
  cairo_set_source_rgba (cr, 1., 1., 1., 0.);
  cairo_paint (cr);

  char *color_chunks;
  long int color_r, color_g, color_b, color_a;
  color_r = strtol(color, &color_chunks, 16);
  color_g = strtol(color_chunks, &color_chunks, 16);
  color_b = strtol(color_chunks, &color_chunks, 16);
  color_a = strtol(color_chunks, NULL, 16);

  double r = ((double)color_r)/255;
  double g = ((double)color_g)/255;
  double b = ((double)color_b)/255;
  double a = ((double)color_a)/255;

  cairo_set_source_rgba (cr, r, g, b ,a);
  cairo_translate (cr, margin, margin);

  /* Set up cairo font face. */
  cairo_font_face_t *cairo_face;
  cairo_face = cairo_ft_font_face_create_for_ft_face (ft_face, 0);
  cairo_set_font_face (cr, cairo_face);
  cairo_set_font_size (cr, font_size);

  cairo_font_extents_t font_extents;
  cairo_font_extents (cr, &font_extents);
  double baseline = (font_size - font_extents.height) * .5 + font_extents.ascent;
  cairo_translate (cr, 0, baseline);

  count = 0;
  token = strtok(input3, s);
  while( token != NULL ) 
  {
    /* Create hb-ftbuffer and populate. */
    hb_buffer_t *hb_buffer;
    hb_buffer = hb_buffer_create ();
    hb_buffer_add_utf8 (hb_buffer, token, -1, 0, -1);
    hb_buffer_guess_segment_properties (hb_buffer);

    /* Shape it! */
    hb_shape (hb_font, hb_buffer, NULL, 0);

    /* Get glyph information and positions out of the buffer. */
    unsigned int len = hb_buffer_get_length (hb_buffer);
    hb_glyph_info_t *info = hb_buffer_get_glyph_infos (hb_buffer, NULL);
    hb_glyph_position_t *pos = hb_buffer_get_glyph_positions (hb_buffer, NULL);

    /* Draw glyph on cairo surface. */
    cairo_glyph_t *cairo_glyphs = cairo_glyph_allocate (len);
    double current_x = lines_padding[count];
    double current_y = count * -(font_size + line_spacing);
    for (unsigned int i = 0; i < len; i++)
    {
      cairo_glyphs[i].index = info[i].codepoint;
      cairo_glyphs[i].x = current_x + pos[i].x_offset / 64.;
      cairo_glyphs[i].y = -(current_y + pos[i].y_offset / 64.);
      current_x += pos[i].x_advance / 64.;
      current_y += pos[i].y_advance / 64.;
    }
    cairo_show_glyphs (cr, cairo_glyphs, len);
    cairo_glyph_free (cairo_glyphs);

    hb_buffer_destroy (hb_buffer);

    count++;
    token = strtok(NULL, s);
  }

  // ============================================================================

  cairo_surface_write_to_png (cairo_surface, output_file);

  cairo_font_face_destroy (cairo_face);
  cairo_destroy (cr);
  cairo_surface_destroy (cairo_surface);

  hb_font_destroy (hb_font);

  FT_Done_Face (ft_face);
  FT_Done_FreeType (ft_library);

  return 0;
}
