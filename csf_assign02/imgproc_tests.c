// imgproc_tests.c — tests for MS1–MS3 using tctest harness.
// This file conditionally compiles/runs a smaller suite for MS2
// (the ASM build), controlled by -DASM_BUILD passed only when
// building the ASM test binary.
//
// C build (no ASM_BUILD): run full suite (MS1/MS3).
// ASM build (ASM_BUILD):   run only complement + transpose +
//                          (optional) helpers you ported in ASM.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "tctest.h"
#include "image.h"
#include "imgproc.h"

typedef struct {
  struct Image in_small;
  struct Image out_small;
  struct Image in_rect;
  struct Image out_rect;
} TestObjs;

static void fill_solid(struct Image *img, uint32_t px) {
  int n = img->width * img->height;
  for (int i = 0; i < n; i++) {
    img->data[i] = px;
  }
}

static TestObjs *setup(void) {
  TestObjs *objs = (TestObjs *) malloc(sizeof(TestObjs));
  assert(objs);

  img_init(&objs->in_small,  2, 2);
  img_init(&objs->out_small, 2, 2);

  img_init(&objs->in_rect,  6, 4);
  img_init(&objs->out_rect, 6, 4);

  objs->in_small.data[compute_index(&objs->in_small, 0, 0)] = make_pixel(0x10, 0x20, 0x30, 0x40);
  objs->in_small.data[compute_index(&objs->in_small, 0, 1)] = make_pixel(0x11, 0x21, 0x31, 0x41);
  objs->in_small.data[compute_index(&objs->in_small, 1, 0)] = make_pixel(0x12, 0x22, 0x32, 0x42);
  objs->in_small.data[compute_index(&objs->in_small, 1, 1)] = make_pixel(0x13, 0x23, 0x33, 0x43);

  fill_solid(&objs->in_rect, make_pixel(0xFF, 0x00, 0x00, 0xFF));
  objs->in_rect.data[compute_index(&objs->in_rect, 2, 3)] = make_pixel(0x12, 0x34, 0x56, 0x78);
  objs->in_rect.data[compute_index(&objs->in_rect, 0, 0)] = make_pixel(0x01, 0x02, 0x03, 0x04);

  fill_solid(&objs->out_small, make_pixel(0, 0, 0, 0xFF));
  fill_solid(&objs->out_rect,  make_pixel(0, 0, 0, 0xFF));

  return objs;
}

static void cleanup(TestObjs *objs) {
  img_cleanup(&objs->in_small);
  img_cleanup(&objs->out_small);
  img_cleanup(&objs->in_rect);
  img_cleanup(&objs->out_rect);
  free(objs);
}

void test_getters_and_make_pixel(TestObjs *t) {
  (void)t;
  uint32_t p = make_pixel(0x12, 0x34, 0x56, 0x78);
  ASSERT(get_r(p) == 0x12);
  ASSERT(get_g(p) == 0x34);
  ASSERT(get_b(p) == 0x56);
  ASSERT(get_a(p) == 0x78);

  p = make_pixel(0xAB, 0xCD, 0xEF, 0x01);
  ASSERT(get_r(p) == 0xAB);
  ASSERT(get_g(p) == 0xCD);
  ASSERT(get_b(p) == 0xEF);
  ASSERT(get_a(p) == 0x01);
}

void test_compute_index_basic(TestObjs *t) {
  (void)t;
  struct Image img;
  img_init(&img, 5, 3);
  ASSERT(compute_index(&img, 0, 0) == 0);
  ASSERT(compute_index(&img, 0, 4) == 4);
  ASSERT(compute_index(&img, 1, 0) == 5);
  ASSERT(compute_index(&img, 2, 3) == 13);
  img_cleanup(&img);
}

#ifndef ASM_BUILD
void test_is_in_ellipse_basic(TestObjs *t) {
  (void)t;
  struct Image img;
  img_init(&img, 6, 4);
  ASSERT(is_in_ellipse(&img, 2, 3) == 1);
  ASSERT(is_in_ellipse(&img, 0, 3) == 1);
  ASSERT(is_in_ellipse(&img, 0, 2) == 0);
  ASSERT(is_in_ellipse(&img, 0, 0) == 0);
  img_cleanup(&img);
}
#endif

void test_complement_basic(TestObjs *t) {
  fill_solid(&t->out_small, make_pixel(0, 0, 0, 0xFF));

  imgproc_complement(&t->in_small, &t->out_small);

  for (int r = 0; r < t->in_small.height; r++) {
    for (int c = 0; c < t->in_small.width; c++) {
      uint32_t pin  = t->in_small.data[compute_index(&t->in_small, r, c)];
      uint32_t pout = t->out_small.data[compute_index(&t->out_small, r, c)];
      uint32_t expected = ((~pin) & 0xFFFFFF00u) | (pin & 0xFFu);
      ASSERT(pout == expected);
    }
  }
}

void test_transpose_basic(TestObjs *t) {
  int ok = imgproc_transpose(&t->in_small, &t->out_small);
  ASSERT(ok == 1);

  ASSERT(t->out_small.data[compute_index(&t->out_small, 0, 0)] ==
         t->in_small.data[compute_index(&t->in_small, 0, 0)]);
  ASSERT(t->out_small.data[compute_index(&t->out_small, 1, 1)] ==
         t->in_small.data[compute_index(&t->in_small, 1, 1)]);
  ASSERT(t->out_small.data[compute_index(&t->out_small, 1, 0)] ==
         t->in_small.data[compute_index(&t->in_small, 0, 1)]);
  ASSERT(t->out_small.data[compute_index(&t->out_small, 0, 1)] ==
         t->in_small.data[compute_index(&t->in_small, 1, 0)]);

  ok = imgproc_transpose(&t->in_rect, &t->out_rect);
  ASSERT(ok == 0);
}

#ifndef ASM_BUILD
void test_ellipse_basic(TestObjs *t) {
  fill_solid(&t->out_rect, make_pixel(0, 0, 0, 0xFF));

  imgproc_ellipse(&t->in_rect, &t->out_rect);

  int a = t->in_rect.width / 2;
  int b = t->in_rect.height / 2;

  ASSERT(t->out_rect.data[compute_index(&t->out_rect, b, a)] ==
         t->in_rect.data[compute_index(&t->in_rect, b, a)]);
  ASSERT(t->out_rect.data[compute_index(&t->out_rect, 0, 0)] ==
         make_pixel(0, 0, 0, 0xFF));
  ASSERT(t->out_rect.data[compute_index(&t->out_rect, 0, 3)] ==
         t->in_rect.data[compute_index(&t->in_rect, 0, 3)]);
}

static int clamp255(int v) { return v < 0 ? 0 : (v > 255 ? 255 : v); }

void test_emboss_basic(TestObjs *t) {
  struct Image in, out;
  img_init(&in, 2, 2);
  img_init(&out, 2, 2);

  in.data[compute_index(&in,0,0)] = make_pixel(10,  20,  30,  200);
  in.data[compute_index(&in,0,1)] = make_pixel(40,  50,  60,  210);
  in.data[compute_index(&in,1,0)] = make_pixel(70,  80,  90,  220);
  in.data[compute_index(&in,1,1)] = make_pixel(100, 110, 120, 230);

  imgproc_emboss(&in, &out);

  ASSERT(out.data[compute_index(&out,0,0)] == make_pixel(128,128,128, 200));
  ASSERT(out.data[compute_index(&out,0,1)] == make_pixel(128,128,128, 210));
  ASSERT(out.data[compute_index(&out,1,0)] == make_pixel(128,128,128, 220));

  int r=100,g=110,b=120, a=230;
  int nr=10, ng=20, nb=30;
  int dr = nr - r, dg = ng - g, db = nb - b;
  int adr = dr>=0?dr:-dr, adg = dg>=0?dg:-dg, adb = db>=0?db:-db;
  int diff;
  if (adr>=adg && adr>=adb) diff = dr;
  else if (adg>=adb)        diff = dg;
  else                      diff = db;
  int gray = clamp255(128 + diff);
  ASSERT(out.data[compute_index(&out,1,1)] == make_pixel(gray,gray,gray, a));

  img_cleanup(&in);
  img_cleanup(&out);

  (void)t;
}
#endif

int main(int argc, char **argv) {
  if (argc > 1) {
    tctest_testname_to_execute = argv[1];
  }

  printf("Running imgproc tests (%s build)\n",
#ifdef ASM_BUILD
         "ASM"
#else
         "C"
#endif
  );

#ifndef ASM_BUILD
  TEST( test_complement_basic );
  TEST( test_transpose_basic );
  TEST( test_ellipse_basic );
  TEST( test_emboss_basic );
  TEST( test_getters_and_make_pixel );
  TEST( test_compute_index_basic );
  TEST( test_is_in_ellipse_basic );
#else
  TEST( test_complement_basic );
  TEST( test_transpose_basic );
  TEST( test_getters_and_make_pixel );
  TEST( test_compute_index_basic );
#endif

  return 0;
}
