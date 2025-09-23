/*
 * animation.c
 *
 *  Created on: Sep 19, 2025
 *      Author: libor
 */

#include "animation.h"
#include "cube_mux.h"

#include "anim_test.h"
#include "anim_edges.h"
#include "anim_scanner.h"
#include "anim_corner_wave.h"
#include "anim_sine_wave.h"
#include "anim_ping_pong.h"
#include "anim_rain.h"
#include "anim_firework.h"
#include "anim_matrix.h"
#include "anim_shrink_cube.h"
#include "anim_heartbeat.h"
#include "anim_snake.h"
#include "anim_screensaver.h"
#include "anim_text.h"
#include "anim_explosion.h"
#include "anim_bees.h"

static uint8_t animation_index;

static graph_animation_t anim;
static animation_desc_t desc[] = {
    { "Happy birthday", &anim_text          },
    { "Scanner",        &anim_scanner       },
    { "Edges",          &anim_edges         },
    { "Cube",           &anim_shrink_cube   },
    { "Corner wave",    &anim_corner_wave   },
    { "Sine wave",      &anim_sine_wave     },
    { "Ping pong",      &anim_ping_pong     },
    { "Snake",          &anim_snake         },
    { "Rain",           &anim_rain          },
    { "Fireworks",      &anim_firework      },
    { "Matrix",         &anim_matrix        },
    { "Heartbeat",      &anim_heartbeat     },
    { "Screensaver",    &anim_screensaver   },
    { "Explosion",      &anim_explosion     },
    { "Bees",           &anim_bees          },
};


void animation_init()
{
  animation_index = 0;
  animation_reset();
}

void animation_reset()
{
  anim.seqIndex = 0;
  anim.i1 = 0;
  anim.i2 = 0;
  anim.i3 = 0;
  anim.v.r = 0;
  anim.v.g = 0;
  anim.v.b = 0;
  anim.reset = 1;
}

void animation_show()
{
  if (TICK_EXPIRED(anim.timer))
  {
    desc[animation_index].func_generator(&anim);
    CubeMux_ScreenToVoxels(scrBuf);
  }
}

char* animation_get_name()
{
  return desc[animation_index].name;
}

void animation_select_next()
{
  animation_index++;
  if (animation_index >= sizeof(desc) / sizeof(animation_desc_t))
  {
    animation_index = 0;
  }
  animation_reset();
}

void animation_select_previous()
{
  if (animation_index > 0)
  {
    animation_index--;
  }
  else
  {
    animation_index = sizeof(desc) / sizeof(animation_desc_t) - 1;
  }
  animation_reset();
}
