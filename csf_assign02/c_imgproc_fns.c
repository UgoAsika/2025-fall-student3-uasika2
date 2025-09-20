// C implementations of image processing functions

#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include "imgproc.h"

// TODO: define your helper functions here

uint32_t get_r(uint32_t pixel){
  return (pixel >> 24) & 0xFFu;
}

uint32_t get_g ( uint32_t pixel){
  return (pixel >> 16) & 0xFFu;
}

uint32_t get_b(uint32_t pixel) {
  return (pixel >> 8) & 0xFFu;
}

uint32_t get_a(uint32_t pixel){
  return pixel & 0xFFu;
}

uint32_t make_pixel(uint32_t r, uint32_t g, uint32_t b, uint32_t a){
  return (( r & 0xFFu) << 24) | ((g & 0xFFu) << 16) | ((b & 0xFFu) << 8) | (a & 0xFFu);
}

int32_t compute_index( struct Image *img, int32_t row, int32_t col ) {
  assert(img != NULL);
  assert(row >= 0 && row < img->height);
  assert(col >= 0 && col < img->width);
  return row * img->width + col;
}

//Return 1 if row , col is in the centred ellipse for the spec , else 0
int is_in_ellipse( struct Image *img, int32_t row, int32_t col ) {
  assert(img != NULL);
  const int32_t w = img->width;
  const int32_t h = img->height;
  const int32_t a = w / 2;  //fllor(w/2)
  const int32_t b = h / 2;  //floor(w/2)

  //center pixel coordinates (row=b, col=a)
  const int32_t x = col - a; //horizontal distance
  const int32_t y = row - b; //vertical distance

  //handle degenerate cases to avoid division by zeroo
  if (a == 0 && b == 0) {
    //1 by 1 image, only the single centre pixel is inside
    return (x == 0 && y == 0) ? 1 : 0;
  }
  if (a == 0) {
    int64_t lhs_y = (int64_t)10000 * (int64_t)y * (int64_t)y;
    int64_t rhs_y = (int64_t)b * (int64_t)b * 10000;
    return (x == 0 && lhs_y <= rhs_y) ? 1 : 0;
  }
  if (b == 0) {
    int64_t lhs_x = (int64_t)10000 * (int64_t)x * (int64_t)x;
    int64_t rhs_x = (int64_t)a * (int64_t)a * 10000;
    return (y == 0 && lhs_x <= rhs_x) ? 1 : 0;
  }

  // General case
  int64_t ax2 = (int64_t)a * (int64_t)a;
  int64_t bx2 = (int64_t)b * (int64_t)b;
  int64_t term_x = ((int64_t)10000 * (int64_t)x * (int64_t)x) / ax2;
  int64_t term_y = ((int64_t)10000 * (int64_t)y * (int64_t)y) / bx2;
  return (term_x + term_y) <= 10000 ? 1 : 0;
}
//! Transform the color component values in each input pixel
//! by applying the bitwise complement operation. I.e., each bit
//! in the color component information should be inverted
//! (1 becomes 0, 0 becomes 1.) The alpha value of each pixel should
//! be left unchanged.
//!
//! @param input_img pointer to the input Image
//! @param output_img pointer to the output Image (in which the
//!                   transformed pixels should be stored)
void imgproc_complement( struct Image *input_img, struct Image *output_img ) {
  assert(input_img && output_img);
  assert(input_img->width == output_img->width);
  assert(input_img->height == output_img->height);

  int32_t w = input_img->width;
  int32_t h = input_img->height;
  int32_t n = w * h;

  for (int32_t i = 0; i < n; ++i) {
    uint32_t p = input_img->data[i];
    uint32_t color_compl = (~p) & 0xFFFFFF00u; //invert RGB, keep low 8 bits zeroed
    uint32_t alpha       =  p   & 0x000000FFu;  //preserve the alphaa
    output_img->data[i]  = color_compl | alpha;
  }
}

//! Transform the input image by swapping the row and column
//! of each source pixel when copying it to the output image.
//! E.g., a pixel at row i and column j of the input image
//! should be copied to row j and column i of the output image.
//! Note that this transformation can only be applied to square
//! images (where the width and height are identical.)
//!
//! @param input_img pointer to the input Image
//! @param output_img pointer to the output Image (in which the
//!                   transformed pixels should be stored)
//!
//! @return 1 if the transformation succeeded, or 0 if the
//!         transformation can't be applied because the image
//!         width and height are not the same
int imgproc_transpose( struct Image *input_img, struct Image *output_img ) {
  assert(input_img && output_img);
  if (input_img->width != input_img->height) {
    return 0; // cannot transpose a non square
  }
  assert(output_img->width == input_img->width);
  assert(output_img->height == input_img->height);

  int32_t n = input_img->width; //width == height
  for (int32_t i = 0; i < n; ++i) {
    for (int32_t j = 0; j < n; ++j) {
      int32_t src_idx = i * n + j;
      int32_t dst_idx = j * n + i;
      output_img->data[dst_idx] = input_img->data[src_idx];
    }
  }
  return 1;
}

//! Transform the input image by copying only those pixels that are
//! within an ellipse centered within the bounds of the image.
//! Pixels not in the ellipse should be left unmodified, which will
//! make them opaque black.
//!
//! Let w represent the width of the image and h represent the
//! height of the image. Let a=floor(w/2) and b=floor(h/2).
//! Consider the pixel at row b and column a is being at the
//! center of the image. When considering whether a specific pixel
//! is in the ellipse, x is the horizontal distance to the center
//! of the image and y is the vertical distance to the center of
//! the image. The pixel at the coordinates described by x and y
//! is in the ellipse if the following inequality is true:
//!
//!   floor( (10000*x*x) / (a*a) ) + floor( (10000*y*y) / (b*b) ) <= 10000
//!
//! @param input_img pointer to the input Image
//! @param output_img pointer to the output Image (in which the
//!                   transformed pixels should be stored)
void imgproc_ellipse( struct Image *input_img, struct Image *output_img ) {
  assert(input_img && output_img);
  assert(input_img->width == output_img->width);
  assert(input_img->height == output_img->height);

  int32_t h = input_img->height;
  int32_t w = input_img->width;

  for (int32_t row = 0; row < h; ++row) {
    for (int32_t col = 0; col < w; ++col) {
      if (is_in_ellipse(input_img, row, col)) {
        int32_t idx = compute_index(input_img, row, col);
        output_img->data[idx] = input_img->data[idx];
      }
      //else leave it as initialised opaque black from img init
    }
  }
}

//! Transform the input image using an "emboss" effect. The pixels
//! of the source image are transformed as follows.
//!
//! The top row and left column of pixels are transformed so that their
//! red, green, and blue color component values are all set to 128,
//! and their alpha values are not modified.
//!
//! For all other pixels, we consider the pixel's color component
//! values r, g, and b, and also the pixel's upper-left neighbor's
//! color component values nr, ng, and nb. In comparing the color
//! component values of the pixel and its upper-left neighbor,
//! we consider the differences (nr-r), (ng-g), and (nb-b).
//! Whichever of these differences has the largest absolute value
//! we refer to as diff. (Note that in the case that more than one
//! difference has the same absolute value, the red difference has
//! priority over green and blue, and the green difference has priority
//! over blue.)
//!
//! From the value diff, compute the value gray as 128 + diff.
//! However, gray should be clamped so that it is in the range
//! 0..255. I.e., if it's negative, it should become 0, and if
//! it is greater than 255, it should become 255.
//!
//! For all pixels not in the top or left row, the pixel's red, green,
//! and blue color component values should be set to gray, and the
//! alpha value should be left unmodified.
//!
//! @param input_img pointer to the input Image
//! @param output_img pointer to the output Image (in which the
//!                   transformed pixels should be stored)
void imgproc_emboss( struct Image *input_img, struct Image *output_img ) {
  assert(input_img && output_img);
  assert(input_img->width == output_img->width);
  assert(input_img->height == output_img->height);

  int32_t h = input_img->height;
  int32_t w = input_img->width;

  for (int32_t row = 0; row < h; ++row) {
    for (int32_t col = 0; col < w; ++col) {
      int32_t idx = compute_index(input_img, row, col);
      uint32_t p = input_img->data[idx];
      uint32_t a = get_a(p);

      if (row == 0 || col == 0) {
        //keep alpha
        uint32_t gray = 128u;
        output_img->data[idx] = make_pixel(gray, gray, gray, a);
      } else {
        // Compare with upperleft neighbour
        int32_t nidx = compute_index(input_img, row - 1, col - 1);
        uint32_t np = input_img->data[nidx];

        int32_t r  = (int32_t)get_r(p);
        int32_t g  = (int32_t)get_g(p);
        int32_t b  = (int32_t)get_b(p);
        int32_t nr = (int32_t)get_r(np);
        int32_t ng = (int32_t)get_g(np);
        int32_t nb = (int32_t)get_b(np);

        int32_t dr = nr - r;
        int32_t dg = ng - g;
        int32_t db = nb - b;

        int32_t adr = dr >= 0 ? dr : -dr;
        int32_t adg = dg >= 0 ? dg : -dg;
        int32_t adb = db >= 0 ? db : -db;

        int32_t diff;
        if (adr >= adg && adr >= adb) {
          diff = dr;                 // red wins ties
        } else if (adg >= adb) {
          diff = dg;                 // green wins tie with blue
        } else {
          diff = db;                 // else blue
        }

        int32_t gray_i = 128 + diff;
        if (gray_i < 0) gray_i = 0;
        if (gray_i > 255) gray_i = 255;

        uint32_t gray = (uint32_t)gray_i;
        output_img->data[idx] = make_pixel(gray, gray, gray, a);
      }
    }
  }
}
