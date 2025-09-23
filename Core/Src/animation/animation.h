/*
 * animation.h
 *
 *  Created on: Sep 19, 2025
 *      Author: libor
 */

#ifndef SRC_ANIMATION_ANIMATION_H_
#define SRC_ANIMATION_ANIMATION_H_

#include "graphics.h"
#include "string.h"


typedef struct
{
  uint32_t timer;
  Voxel_t v;
  uint32_t seqIndex;
  uint32_t i1, i2, i3;
  uint8_t reset;

} graph_animation_t;

typedef struct
{
  char name[32];
  void (*func_generator)(graph_animation_t *);
} animation_desc_t;



#define VOXEL_COL_BLACK(v)   do { (v).r = 0;   (v).g = 0;   (v).b = 0;   } while(0)
#define VOXEL_COL_WHITE(v)   do { (v).r = 255; (v).g = 255; (v).b = 255; } while(0)
#define VOXEL_COL_RED(v)     do { (v).r = 255; (v).g = 0;   (v).b = 0;   } while(0)
#define VOXEL_COL_GREEN(v)   do { (v).r = 0;   (v).g = 255; (v).b = 0;   } while(0)
#define VOXEL_COL_BLUE(v)    do { (v).r = 0;   (v).g = 0;   (v).b = 255; } while(0)
#define VOXEL_COL_YELLOW(v)  do { (v).r = 255; (v).g = 255; (v).b = 0;   } while(0)
#define VOXEL_COL_CYAN(v)    do { (v).r = 0;   (v).g = 255; (v).b = 255; } while(0)
#define VOXEL_COL_MAGENTA(v) do { (v).r = 255; (v).g = 0;   (v).b = 255; } while(0)




void animation_init();

void animation_reset();

void animation_show();

void animation_select_next();

char* animation_get_name();

void animation_select_previous();


#endif /* SRC_ANIMATION_ANIMATION_H_ */
