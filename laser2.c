/*

  laser2.c

  Copyright 2017 Matthew T. Pandina. All rights reserved.

  This file is part of Laser 2.

  Laser 2 is free software: you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Laser 2 is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Laser 2.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <stdlib.h>
#include <string.h>
#include <avr/pgmspace.h>
#include <uzebox.h>

#include "data/tileset.inc"
#include "data/sprites.inc"
#include "data/titlescreen.inc"
#include "data/patches.inc"
#include "data/midisong.h"

typedef struct {
  uint16_t held;
  uint16_t prev;
  uint16_t pressed;
  uint16_t released;
} __attribute__ ((packed)) BUTTON_INFO;

#define TILE_BACKGROUND  0
#define TILE_GREEN 1
#define TILE_YELLOW 2
#define TILE_BLUE 3
#define TILE_RED 4

#define TILE_TITLE_LASER 1

// Defines for the pieces. Rotations are treated as different pieces. Unknown rotations end in _U
#define P_BLANK 0
#define P_LASER_T 1
#define P_LASER_R 2
#define P_LASER_B 3
#define P_LASER_L 4
#define P_LASER_U 5
#define P_MIRROR_TARGET_OPT_BR 6
#define P_MIRROR_TARGET_OPT_BL 7
#define P_MIRROR_TARGET_OPT_TL 8
#define P_MIRROR_TARGET_OPT_TR 9
#define P_MIRROR_TARGET_OPT_U 10
#define P_MIRROR_TARGET_REQ_BR 11
#define P_MIRROR_TARGET_REQ_BL 12
#define P_MIRROR_TARGET_REQ_TL 13
#define P_MIRROR_TARGET_REQ_TR 14
#define P_MIRROR_TARGET_REQ_U 15
#define P_SPLIT_TRBL 16
#define P_SPLIT_TLBR 17
#define P_SPLIT_U 18
#define P_DBL_MIRROR_TRBL 19
#define P_DBL_MIRROR_TLBR 20
#define P_DBL_MIRROR_U 21
#define P_CHECKPOINT_TCBC 22
#define P_CHECKPOINT_LCRC 23
#define P_CHECKPOINT_U 24
#define P_CELL_BLOCKER 25

// The configuration of the playing board
uint8_t board[5][5] = {
  {  0,  0,  0,  0,  0 },
  {  0,  0,  0,  0,  0 },
  {  0,  0,  0,  0,  0 },
  {  0,  0,  0,  0,  0 },
  {  0,  0,  0,  0,  0 },
};

/* Each square may have a laser beam going in and/or out in any direction
      IN   OUT
   0b 0000 0000
       \\\\ \\\\__ top
        \\\\ \\\__ bottom
         \\\\ \\__ left
          \\\\ \__ right
           \\\\
            \\\\__ top
             \\\__ bottom
              \\__ left
               \__ right
*/
#define D_OUT_T 1
#define D_OUT_B 2
#define D_OUT_L 4
#define D_OUT_R 8

#define D_IN_T 16
#define D_IN_B 32
#define D_IN_L 64
#define D_IN_R 128

// The bitmap of where the laser is, and which direction(s) it is travelling
uint8_t laser[5][5] = {
  {  0,  0,  0,  0,  0 },
  {  0,  0,  0,  0,  0 },
  {  0,  0,  0,  0,  0 },
  {  0,  0,  0,  0,  0 },
  {  0,  0,  0,  0,  0 },
};

// The pieces in your "hand" (that need to be placed on the board)
uint8_t hand[5] = { 0, 0, 0, 0, 0 };

const uint8_t levelData[] PROGMEM = {
  // LEVEL 1
  // Puzzle
  0, 0, 0, 0, 0,
  0, P_LASER_B, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, P_MIRROR_TARGET_REQ_BR, 0,
  0, 0, 0, 0, 0,
  // Solution
  0, 0, 0, 0, 0,
  0, P_LASER_B, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, P_DBL_MIRROR_TLBR, 0, P_MIRROR_TARGET_REQ_BR, 0,
  0, 0, 0, 0, 0,
  // Hand
  P_DBL_MIRROR_U, 0, 0, 0, 0,
  // Targets
  1,
  
  // LEVEL 2
  // Puzzle
  0, 0, 0, 0, P_MIRROR_TARGET_REQ_TR,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  P_MIRROR_TARGET_OPT_BR, 0, 0, 0, 0,
  P_LASER_U, 0, 0, 0, 0,
  // Solution
  0, 0, 0, 0, P_MIRROR_TARGET_REQ_TR,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  P_MIRROR_TARGET_OPT_BR, 0, 0, 0, P_MIRROR_TARGET_OPT_TL,
  P_LASER_T, 0, 0, 0, 0,
  // Hand
  P_MIRROR_TARGET_OPT_U, 0, 0, 0, 0,
  // Targets
  1,

  // LEVEL 3
  // Puzzle
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  P_MIRROR_TARGET_OPT_U, 0, 0, 0, P_MIRROR_TARGET_OPT_BL,
  P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_REQ_U, 0, 0, 0,
  // Solution
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  P_MIRROR_TARGET_OPT_BR, 0, 0, 0, P_MIRROR_TARGET_OPT_BL,
  P_MIRROR_TARGET_OPT_TR, P_MIRROR_TARGET_REQ_BR, 0, 0, P_LASER_T,
  // Hand
  P_LASER_U, 0, 0, 0, 0,
  // Targets
  1,

  // LEVEL 4
  // Puzzle
  0, P_LASER_B, 0, 0, 0,
  P_CHECKPOINT_LCRC, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  P_MIRROR_TARGET_OPT_U, 0, 0, 0, 0,
  // Solution
  P_MIRROR_TARGET_OPT_TR, P_LASER_B, 0, 0, 0,
  P_CHECKPOINT_LCRC, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  P_MIRROR_TARGET_OPT_TR, P_MIRROR_TARGET_OPT_TL, 0, 0, 0,
  // Hand
  P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_OPT_U, 0, 0, 0,
  // Targets
  1,

  // LEVEL 5
  // Puzzle
  0, P_MIRROR_TARGET_REQ_U, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, P_MIRROR_TARGET_REQ_U,
  0, P_CELL_BLOCKER, 0, 0, 0,
  // Solution
  0, P_MIRROR_TARGET_REQ_TR, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  P_LASER_R, P_SPLIT_TRBL, 0, 0, P_MIRROR_TARGET_REQ_BR,
  0, P_CELL_BLOCKER, 0, 0, 0,
  // Hand
  P_LASER_U, P_SPLIT_U, 0, 0, 0,
  // Targets
  2,

  // LEVEL 6
  // Puzzle
  0, P_MIRROR_TARGET_REQ_U, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, P_CHECKPOINT_TCBC, 0,
  0, 0, 0, 0, 0,
  // Solution
  0, P_MIRROR_TARGET_REQ_TR, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, P_DBL_MIRROR_TLBR, 0, P_CHECKPOINT_TCBC, P_LASER_L,
  0, 0, 0, 0, 0,
  // Hand
  P_LASER_U, P_DBL_MIRROR_U, 0, 0, 0,
  // Targets
  1,

  // LEVEL 7
  // Puzzle
  0, P_MIRROR_TARGET_REQ_TR, 0, 0, 0,
  P_LASER_R, 0, 0, 0, P_MIRROR_TARGET_OPT_BL,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, P_MIRROR_TARGET_OPT_TR, 0, 0, P_MIRROR_TARGET_OPT_TL,
  // Solution
  0, P_MIRROR_TARGET_REQ_TR, 0, 0, 0,
  P_LASER_R, P_DBL_MIRROR_TLBR, 0, 0, P_MIRROR_TARGET_OPT_BL,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, P_MIRROR_TARGET_OPT_TR, 0, 0, P_MIRROR_TARGET_OPT_TL,
  // Hand
  P_DBL_MIRROR_U, 0, 0, 0, 0,
  // Targets
  1,

  // LEVEL 8
  // Puzzle
  P_MIRROR_TARGET_REQ_U, 0, P_CHECKPOINT_TCBC, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, P_LASER_U, 0, P_CELL_BLOCKER,
  // Solution
  P_MIRROR_TARGET_REQ_TL, 0, P_CHECKPOINT_TCBC, P_MIRROR_TARGET_OPT_BL, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, P_LASER_R, P_MIRROR_TARGET_OPT_TL, P_CELL_BLOCKER,
  // Hand
  P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_OPT_U, 0, 0, 0,
  // Targets
  1,

  // LEVEL 9
  // Puzzle
  P_MIRROR_TARGET_REQ_U, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  P_CELL_BLOCKER, 0, P_DBL_MIRROR_U, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, P_LASER_U,
  // Solution
  P_MIRROR_TARGET_REQ_TL, 0, P_MIRROR_TARGET_OPT_BL, 0, 0,
  0, 0, 0, 0, 0,
  P_CELL_BLOCKER, 0, P_DBL_MIRROR_TLBR, 0, P_MIRROR_TARGET_OPT_BL,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, P_LASER_T,
  // Hand
  P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_OPT_U, 0, 0, 0,
  // Targets
  1,

  // LEVEL 10
  // Puzzle
  0, 0, 0, 0, P_MIRROR_TARGET_OPT_U,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  P_MIRROR_TARGET_REQ_U, P_LASER_U, 0, 0, 0,
  // Solution
  P_MIRROR_TARGET_OPT_BR, 0, 0, 0, P_MIRROR_TARGET_OPT_BL,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  P_MIRROR_TARGET_REQ_BL, P_LASER_R, 0, 0, P_MIRROR_TARGET_OPT_TL,
  // Hand
  P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_OPT_U, 0, 0, 0,
  // Targets
  1,

  // LEVEL 11
  // Puzzle
  0, 0, 0, P_MIRROR_TARGET_OPT_U, 0,
  0, 0, 0, 0, 0,
  P_MIRROR_TARGET_OPT_U, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  P_LASER_U, 0, 0, P_MIRROR_TARGET_OPT_U, 0,
  // Solution
  0, 0, 0, P_MIRROR_TARGET_OPT_TR, 0,
  0, 0, 0, 0, 0,
  P_MIRROR_TARGET_OPT_TL, 0, 0, P_SPLIT_TLBR, 0,
  0, 0, 0, 0, 0,
  P_LASER_R, 0, 0, P_MIRROR_TARGET_OPT_TL, 0,
  // Hand
  P_SPLIT_U, 0, 0, 0, 0,
  // Targets
  2,

  // LEVEL 12
  // Puzzle
  0, 0, 0, 0, P_MIRROR_TARGET_REQ_U,
  0, 0, P_CHECKPOINT_U, 0, 0,
  0, 0, P_DBL_MIRROR_U, 0, 0,
  0, 0, 0, 0, 0,
  P_LASER_U, 0, 0, 0, 0,
  // Solution
  0, 0, P_MIRROR_TARGET_OPT_BR, 0, P_MIRROR_TARGET_REQ_BR,
  0, 0, P_CHECKPOINT_LCRC, 0, 0,
  P_MIRROR_TARGET_OPT_BR, 0, P_DBL_MIRROR_TRBL, 0, 0,
  0, 0, 0, 0, 0,
  P_LASER_T, 0, 0, 0, 0,
  // Hand
  P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_OPT_U, 0, 0, 0,
  // Targets
  1,

  // LEVEL 13
  // Puzzle
  0, 0, P_LASER_U, 0, P_DBL_MIRROR_TLBR,
  0, 0, 0, 0, 0,
  P_MIRROR_TARGET_REQ_TL, 0, 0, 0, 0,
  P_MIRROR_TARGET_REQ_U, 0, 0, 0, P_MIRROR_TARGET_OPT_U,
  0, 0, 0, 0, 0,
  // Solution
  0, 0, P_LASER_R, 0, P_DBL_MIRROR_TLBR,
  0, 0, 0, 0, 0,
  P_MIRROR_TARGET_REQ_TL, 0, 0, 0, P_SPLIT_TRBL,
  P_MIRROR_TARGET_REQ_TL, 0, 0, 0, P_MIRROR_TARGET_OPT_TL,
  0, 0, 0, 0, 0,
  // Hand
  P_SPLIT_U, 0, 0, 0, 0,
  // Targets
  2,

  // LEVEL 14
  // Puzzle
  0, 0, 0, 0, 0,
  0, P_MIRROR_TARGET_OPT_U, 0, P_MIRROR_TARGET_OPT_BL, 0,
  0, 0, P_MIRROR_TARGET_OPT_U, P_DBL_MIRROR_TLBR, 0,
  0, P_MIRROR_TARGET_OPT_U, 0, P_MIRROR_TARGET_OPT_U, 0,
  0, 0, 0, 0, 0,
  // Solution
  0, 0, 0, 0, 0,
  0, P_MIRROR_TARGET_OPT_BR, 0, P_MIRROR_TARGET_OPT_BL, 0,
  0, 0, P_MIRROR_TARGET_OPT_TL, P_DBL_MIRROR_TLBR, P_LASER_L,
  0, P_MIRROR_TARGET_OPT_TR, 0, P_MIRROR_TARGET_OPT_TL, 0,
  0, 0, 0, 0, 0,
  // Hand
  P_LASER_U, 0, 0, 0, 0,
  // Targets
  1,

  // LEVEL 15
  // Puzzle
  0, 0, 0, 0, 0,
  0, P_MIRROR_TARGET_OPT_U, 0, P_MIRROR_TARGET_OPT_U, 0,
  0, 0, P_MIRROR_TARGET_OPT_TR, P_DBL_MIRROR_TLBR, 0,
  0, P_MIRROR_TARGET_OPT_U, 0, P_MIRROR_TARGET_OPT_U, 0,
  0, 0, 0, 0, 0,
  // Solution
  0, 0, P_LASER_B, 0, 0,
  0, P_MIRROR_TARGET_OPT_BR, 0, P_MIRROR_TARGET_OPT_BR, 0,
  0, 0, P_MIRROR_TARGET_OPT_TR, P_DBL_MIRROR_TLBR, 0,
  0, P_MIRROR_TARGET_OPT_TR, 0, P_MIRROR_TARGET_OPT_TL, 0,
  0, 0, 0, 0, 0,
  // Hand
  P_LASER_U, 0, 0, 0, 0,
  // Targets
  1,

  // LEVEL 16
  // Puzzle
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, P_MIRROR_TARGET_OPT_U, 0,
  0, P_LASER_B, 0, 0, 0,
  0, 0, P_MIRROR_TARGET_REQ_BL, 0, 0,
  // Solution
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  P_MIRROR_TARGET_OPT_BR, 0, P_SPLIT_TLBR, P_MIRROR_TARGET_OPT_BR, 0,
  0, P_LASER_B, 0, 0, 0,
  P_MIRROR_TARGET_OPT_TR, P_MIRROR_TARGET_OPT_TL, P_MIRROR_TARGET_REQ_BL, 0, 0,
  // Hand
  P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_OPT_U, P_SPLIT_U, 0,
  // Targets
  2,

  // LEVEL 17
  // Puzzle
  P_LASER_U, 0, 0, 0, P_MIRROR_TARGET_REQ_U,
  0, 0, 0, P_DBL_MIRROR_TLBR, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  // Solution
  P_LASER_R, 0, 0, P_SPLIT_TLBR, P_MIRROR_TARGET_REQ_BR,
  0, 0, 0, P_DBL_MIRROR_TLBR, P_MIRROR_TARGET_REQ_BR,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  // Hand
  P_MIRROR_TARGET_REQ_U, P_SPLIT_U, 0, 0, 0,
  // Targets
  2,

  // LEVEL 18
  // Puzzle
  0, 0, 0, 0, 0,
  0, 0, P_CHECKPOINT_U, 0, 0,
  0, 0, 0, 0, 0,
  0, P_MIRROR_TARGET_OPT_U, 0, P_MIRROR_TARGET_OPT_U, 0,
  0, P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_REQ_U, P_MIRROR_TARGET_OPT_U, 0,
  // Solution
  0, 0, P_LASER_B, 0, 0,
  0, 0, P_CHECKPOINT_LCRC, 0, 0,
  0, 0, 0, 0, 0,
  0, P_MIRROR_TARGET_OPT_BR, 0, P_MIRROR_TARGET_OPT_BL, 0,
  0, P_MIRROR_TARGET_OPT_TR, P_MIRROR_TARGET_REQ_TL, P_MIRROR_TARGET_OPT_TL, 0,
  // Hand
  P_LASER_U, 0, 0, 0, 0,
  // Targets
  1,

  // LEVEL 19
  // Puzzle
  0, 0, 0, P_MIRROR_TARGET_REQ_U, 0,
  0, P_LASER_U, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, P_MIRROR_TARGET_REQ_TL, 0,
  0, 0, 0, 0, 0,
  // Solution
  0, 0, 0, P_MIRROR_TARGET_REQ_TR, 0,
  0, P_LASER_R, 0, P_SPLIT_TRBL, P_MIRROR_TARGET_OPT_BL,
  0, 0, 0, 0, 0,
  0, 0, 0, P_MIRROR_TARGET_REQ_TL, P_MIRROR_TARGET_OPT_TL,
  0, 0, 0, 0, 0,
  // Hand
  P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_OPT_U, P_SPLIT_U, 0, 0,
  // Targets
  2,

  // LEVEL 20
  // Puzzle
  0, 0, 0, P_MIRROR_TARGET_REQ_U, 0,
  P_MIRROR_TARGET_REQ_TL, 0, 0, 0, P_CELL_BLOCKER,
  0, P_CHECKPOINT_U, 0, 0, P_MIRROR_TARGET_REQ_U,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  // Solution
  0, 0, 0, P_MIRROR_TARGET_REQ_TR, 0,
  P_MIRROR_TARGET_REQ_TL, 0, 0, P_SPLIT_TLBR, P_CELL_BLOCKER,
  P_LASER_R, P_CHECKPOINT_TCBC, 0, P_SPLIT_TRBL, P_MIRROR_TARGET_REQ_BR,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  // Hand
  P_LASER_U, P_SPLIT_U, P_SPLIT_U, 0, 0,
  // Targets
  3,

  // LEVEL 21
  // Puzzle
  0, 0, P_SPLIT_TRBL, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, P_MIRROR_TARGET_REQ_U, P_MIRROR_TARGET_REQ_U, P_MIRROR_TARGET_REQ_U, 0,
  // Solution
  0, P_DBL_MIRROR_TRBL, P_SPLIT_TRBL, P_SPLIT_TRBL, P_LASER_L,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, P_MIRROR_TARGET_REQ_BL, P_MIRROR_TARGET_REQ_BL, P_MIRROR_TARGET_REQ_BL, 0,
  // Hand
  P_LASER_U, P_DBL_MIRROR_U, P_SPLIT_U, 0, 0,
  // Targets
  3,

  // LEVEL 22
  // Puzzle
  0, 0, 0, 0, 0,
  P_MIRROR_TARGET_REQ_U, 0, P_LASER_R, 0, P_MIRROR_TARGET_REQ_U,
  0, P_DBL_MIRROR_U, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, P_MIRROR_TARGET_REQ_U, 0,
  // Solution
  0, 0, 0, 0, 0,
  P_MIRROR_TARGET_REQ_TL, P_MIRROR_TARGET_OPT_BL, P_LASER_R, P_SPLIT_TLBR, P_MIRROR_TARGET_REQ_BR,
  0, P_DBL_MIRROR_TLBR, 0, P_SPLIT_TRBL, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, P_MIRROR_TARGET_REQ_BL, 0,
  // Hand
  P_MIRROR_TARGET_OPT_U, P_SPLIT_U, P_SPLIT_U, 0, 0,
  // Targets
  3,

  // LEVEL 23
  // Puzzle
  0, 0, 0, 0, P_MIRROR_TARGET_REQ_U,
  0, 0, 0, 0, 0,
  0, P_CHECKPOINT_U, 0, 0, 0,
  0, P_LASER_B, 0, 0, 0,
  0, 0, 0, 0, 0,
  // Solution
  0, 0, 0, 0, P_MIRROR_TARGET_REQ_TR,
  0, 0, 0, 0, 0,
  P_MIRROR_TARGET_OPT_TL, P_CHECKPOINT_TCBC, 0, 0, P_SPLIT_TLBR,
  0, P_LASER_B, 0, 0, 0,
  0, P_MIRROR_TARGET_OPT_TR, 0, 0, P_MIRROR_TARGET_OPT_TL,
  // Hand
  P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_OPT_U, P_SPLIT_U, 0,
  // Targets
  2,

  // LEVEL 24
  // Puzzle
  0, 0, 0, 0, P_LASER_L,
  0, 0, 0, 0, 0,
  P_MIRROR_TARGET_REQ_U, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  P_MIRROR_TARGET_OPT_U, 0, 0, P_CHECKPOINT_TCBC, 0,
  // Solution
  P_MIRROR_TARGET_OPT_BR, 0, 0, 0, P_LASER_L,
  0, 0, 0, 0, 0,
  P_MIRROR_TARGET_REQ_TR, 0, 0, 0, P_MIRROR_TARGET_OPT_BL,
  0, 0, 0, 0, 0,
  P_MIRROR_TARGET_OPT_TR, 0, 0, P_CHECKPOINT_TCBC, P_MIRROR_TARGET_OPT_TL,
  // Hand
  P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_OPT_U, 0, 0,
  // Targets
  1,

  // LEVEL 25
  // Puzzle
  P_LASER_U, 0, 0, P_CELL_BLOCKER, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, P_CHECKPOINT_U, P_SPLIT_U, 0,
  0, 0, 0, P_MIRROR_TARGET_REQ_U, 0,
  // Solution
  P_LASER_B, 0, 0, P_CELL_BLOCKER, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  P_DBL_MIRROR_TLBR, 0, P_CHECKPOINT_TCBC, P_SPLIT_TLBR, P_MIRROR_TARGET_REQ_BR,
  0, 0, 0, P_MIRROR_TARGET_REQ_BL, 0,
  // Hand
  P_MIRROR_TARGET_REQ_U, P_DBL_MIRROR_U, 0, 0, 0,
  // Targets
  2,

  // LEVEL 26
  // Puzzle
  0, 0, 0, P_CELL_BLOCKER, 0,
  P_CHECKPOINT_LCRC, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  P_MIRROR_TARGET_OPT_U, P_DBL_MIRROR_TLBR, P_LASER_U, P_MIRROR_TARGET_OPT_U, 0,
  0, 0, 0, 0, 0,
  // Solution
  P_MIRROR_TARGET_OPT_TR, 0, 0, P_CELL_BLOCKER, 0,
  P_CHECKPOINT_LCRC, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  P_MIRROR_TARGET_OPT_TR, P_DBL_MIRROR_TLBR, P_LASER_R, P_MIRROR_TARGET_OPT_BL, 0,
  0, P_MIRROR_TARGET_OPT_TR, 0, P_MIRROR_TARGET_OPT_TL, 0,
  // Hand
  P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_OPT_U, 0, 0,
  // Targets
  1,

  // LEVEL 27
  // Puzzle
  0, 0, 0, 0, 0,
  P_CHECKPOINT_LCRC, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  P_MIRROR_TARGET_OPT_U, P_DBL_MIRROR_TLBR, P_LASER_U, P_MIRROR_TARGET_OPT_U, 0,
  0, 0, 0, P_CELL_BLOCKER, 0,
  // Solution
  P_MIRROR_TARGET_OPT_BR, 0, 0, P_MIRROR_TARGET_OPT_BL, 0,
  P_CHECKPOINT_LCRC, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  P_MIRROR_TARGET_OPT_TR, P_DBL_MIRROR_TLBR, P_LASER_R, P_MIRROR_TARGET_OPT_TL, 0,
  0, P_MIRROR_TARGET_OPT_BL, 0, P_CELL_BLOCKER, 0,
  // Hand
  P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_OPT_U, 0, 0,
  // Targets
  1,

  // LEVEL 28
  // Puzzle
  0, 0, 0, 0, 0,
  0, 0, 0, P_CELL_BLOCKER, 0,
  0, P_MIRROR_TARGET_OPT_TL, P_DBL_MIRROR_TRBL, 0, P_LASER_U,
  0, 0, 0, P_MIRROR_TARGET_OPT_TL, 0,
  0, 0, 0, 0, 0,
  // Solution
  0, 0, P_MIRROR_TARGET_OPT_BR, P_MIRROR_TARGET_OPT_BL, 0,
  0, 0, P_CHECKPOINT_LCRC, P_CELL_BLOCKER, 0,
  0, P_MIRROR_TARGET_OPT_TL, P_DBL_MIRROR_TRBL, 0, P_LASER_L,
  0, 0, P_MIRROR_TARGET_OPT_TR, P_MIRROR_TARGET_OPT_TL, 0,
  0, 0, 0, 0, 0,
  // Hand
  P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_OPT_U, P_CHECKPOINT_U, 0,
  // Targets
  1,

  // LEVEL 29
  // Puzzle
  0, 0, 0, 0, 0,
  0, 0, 0, P_MIRROR_TARGET_REQ_TR, 0,
  P_LASER_U, 0, 0, P_DBL_MIRROR_TLBR, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, P_MIRROR_TARGET_REQ_U, 0,
  // Solution
  0, 0, 0, 0, 0,
  0, 0, 0, P_MIRROR_TARGET_REQ_TR, 0,
  P_LASER_R, 0, 0, P_DBL_MIRROR_TLBR, P_MIRROR_TARGET_OPT_BL,
  0, 0, 0, P_SPLIT_TLBR, P_MIRROR_TARGET_OPT_TL,
  0, 0, 0, P_MIRROR_TARGET_REQ_BL, 0,
  // Hand
  P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_OPT_U, P_SPLIT_U, 0, 0,
  // Targets
  2,

  // LEVEL 30
  // Puzzle
  0, 0, P_MIRROR_TARGET_OPT_U, 0, 0,
  0, P_MIRROR_TARGET_OPT_U, 0, P_MIRROR_TARGET_OPT_U, 0,
  0, P_CHECKPOINT_U, 0, 0, 0,
  0, P_MIRROR_TARGET_REQ_U, P_SPLIT_TRBL, P_MIRROR_TARGET_OPT_U, 0,
  0, 0, 0, 0, 0,
  // Solution
  0, 0, P_MIRROR_TARGET_OPT_TR, 0, 0,
  0, P_MIRROR_TARGET_OPT_BR, 0, P_MIRROR_TARGET_OPT_BL, 0,
  0, P_CHECKPOINT_LCRC, 0, 0, 0,
  0, P_MIRROR_TARGET_REQ_BL, P_SPLIT_TRBL, P_MIRROR_TARGET_OPT_TL, 0,
  0, 0, P_LASER_T, 0, 0,
  // Hand
  P_LASER_U, 0, 0, 0, 0,
  // Targets
  2,

  // LEVEL 31
  // Puzzle
  P_MIRROR_TARGET_OPT_U, 0, 0, P_MIRROR_TARGET_OPT_U, 0,
  0, 0, 0, 0, 0,
  P_CHECKPOINT_LCRC, 0, 0, 0, 0,
  0, 0, 0, 0, P_LASER_U,
  P_CELL_BLOCKER, 0, 0, P_MIRROR_TARGET_REQ_U, 0,
  // Solution
  P_MIRROR_TARGET_OPT_BR, 0, 0, P_MIRROR_TARGET_OPT_BL, 0,
  0, 0, 0, 0, 0,
  P_CHECKPOINT_LCRC, 0, 0, 0, 0,
  P_MIRROR_TARGET_OPT_TR, 0, 0, P_DBL_MIRROR_TLBR, P_LASER_L,
  P_CELL_BLOCKER, 0, 0, P_MIRROR_TARGET_REQ_BL, 0,
  // Hand
  P_MIRROR_TARGET_OPT_U, P_DBL_MIRROR_U, 0, 0, 0,
  // Targets
  1,

  // LEVEL 32
  // Puzzle
  0, 0, P_MIRROR_TARGET_OPT_U, 0, P_MIRROR_TARGET_OPT_U,
  0, 0, 0, 0, 0,
  0, P_LASER_U, 0, P_MIRROR_TARGET_OPT_BL, 0,
  0, 0, 0, 0, P_CHECKPOINT_LCRC,
  P_DBL_MIRROR_TLBR, 0, 0, 0, 0,
  // Solution
  0, 0, P_MIRROR_TARGET_OPT_TL, P_SPLIT_TRBL, P_MIRROR_TARGET_OPT_BL,
  0, 0, 0, 0, 0,
  P_MIRROR_TARGET_OPT_BR, P_LASER_L, 0, P_MIRROR_TARGET_OPT_BL, 0,
  0, 0, 0, 0, P_CHECKPOINT_LCRC,
  P_DBL_MIRROR_TLBR, 0, 0, 0, P_MIRROR_TARGET_OPT_TL,
  // Hand
  P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_OPT_U, P_SPLIT_U, 0, 0,
  // Targets
  2,

  // LEVEL 33
  // Puzzle
  0, 0, 0, 0, 0,
  0, 0, P_DBL_MIRROR_U, 0, P_MIRROR_TARGET_REQ_U,
  0, 0, 0, 0, 0,
  P_LASER_U, 0, P_CELL_BLOCKER, 0, 0,
  0, 0, 0, P_MIRROR_TARGET_REQ_U, 0,
  // Solution
  0, 0, 0, 0, 0,
  0, 0, P_DBL_MIRROR_TRBL, 0, P_MIRROR_TARGET_REQ_BR,
  0, 0, 0, 0, 0,
  P_LASER_B, 0, P_CELL_BLOCKER, 0, 0,
  P_MIRROR_TARGET_OPT_TR, 0, P_SPLIT_TRBL, P_MIRROR_TARGET_REQ_BR, 0,
  // Hand
  P_MIRROR_TARGET_OPT_U, P_SPLIT_U, 0, 0, 0,
  // Targets
  2,

  // LEVEL 34
  // Puzzle
  0, 0, P_LASER_B, 0, 0,
  0, 0, 0, P_MIRROR_TARGET_REQ_BR, 0,
  0, 0, 0, P_CHECKPOINT_U, 0,
  P_DBL_MIRROR_TLBR, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  // Solution
  0, 0, P_LASER_B, 0, 0,
  P_MIRROR_TARGET_OPT_BR, 0, 0, P_MIRROR_TARGET_REQ_BR, 0,
  0, 0, P_MIRROR_TARGET_OPT_TR, P_CHECKPOINT_TCBC, P_MIRROR_TARGET_OPT_BL,
  P_DBL_MIRROR_TLBR, 0, 0, 0, P_MIRROR_TARGET_OPT_TL,
  0, 0, 0, 0, 0,
  // Hand
  P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_OPT_U, 0,
  // Targets
  1,

  // LEVEL 35
  // Puzzle
  0, 0, P_MIRROR_TARGET_OPT_U, 0, 0,
  P_MIRROR_TARGET_REQ_U, P_CHECKPOINT_U, 0, P_LASER_U, 0,
  0, P_MIRROR_TARGET_OPT_U, P_SPLIT_U, 0, 0,
  P_MIRROR_TARGET_OPT_U, 0, P_MIRROR_TARGET_OPT_U, 0, 0,
  0, 0, 0, 0, 0,
  // Solution
  P_DBL_MIRROR_TRBL, 0, P_MIRROR_TARGET_OPT_BL, 0, 0,
  P_MIRROR_TARGET_REQ_TR, P_CHECKPOINT_TCBC, 0, P_LASER_L, 0,
  0, P_MIRROR_TARGET_OPT_TL, P_SPLIT_TRBL, 0, 0,
  P_MIRROR_TARGET_OPT_TR, 0, P_MIRROR_TARGET_OPT_TL, 0, 0,
  0, 0, 0, 0, 0,
  // Hand
  P_DBL_MIRROR_U, 0, 0, 0, 0,
  // Targets
  2,

  // LEVEL 36
  // Puzzle
  0, 0, P_MIRROR_TARGET_REQ_U, 0, 0,
  P_MIRROR_TARGET_OPT_U, P_CHECKPOINT_U, 0, P_LASER_U, 0,
  0, P_MIRROR_TARGET_OPT_U, P_SPLIT_U, 0, 0,
  P_MIRROR_TARGET_OPT_U, 0, P_MIRROR_TARGET_OPT_U, 0, 0,
  0, 0, 0, 0, 0,
  // Solution
  0, 0, P_MIRROR_TARGET_REQ_TR, 0, 0,
  P_MIRROR_TARGET_OPT_BR, P_CHECKPOINT_TCBC, P_DBL_MIRROR_TRBL, P_LASER_L, 0,
  0, P_MIRROR_TARGET_OPT_TL, P_SPLIT_TRBL, 0, 0,
  P_MIRROR_TARGET_OPT_TR, 0, P_MIRROR_TARGET_OPT_TL, 0, 0,
  0, 0, 0, 0, 0,
  // Hand
  P_DBL_MIRROR_U, 0, 0, 0, 0,
  // Targets
  2,

  // LEVEL 37
  // Puzzle
  0, 0, 0, P_SPLIT_TRBL, 0,
  0, 0, 0, 0, 0,
  0, P_CELL_BLOCKER, P_CHECKPOINT_U, 0, P_MIRROR_TARGET_OPT_TL,
  0, 0, 0, P_MIRROR_TARGET_OPT_BL, 0,
  0, P_DBL_MIRROR_TRBL, 0, 0, 0,
  // Solution
  0, P_MIRROR_TARGET_OPT_BR, 0, P_SPLIT_TRBL, P_MIRROR_TARGET_OPT_BL,
  0, 0, 0, 0, 0,
  P_LASER_R, P_CELL_BLOCKER, P_CHECKPOINT_TCBC, 0, P_MIRROR_TARGET_OPT_TL,
  0, 0, 0, P_MIRROR_TARGET_OPT_BL, 0,
  P_MIRROR_TARGET_OPT_TL, P_DBL_MIRROR_TRBL, 0, 0, 0,
  // Hand
  P_LASER_U, P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_OPT_U, 0,
  // Targets
  2,

  // LEVEL 38
  // Puzzle
  0, 0, P_MIRROR_TARGET_OPT_U, 0, 0,
  0, P_MIRROR_TARGET_OPT_U, 0, P_MIRROR_TARGET_OPT_U, 0,
  0, P_CHECKPOINT_U, 0, 0, 0,
  0, P_MIRROR_TARGET_REQ_U, P_SPLIT_TLBR, P_MIRROR_TARGET_OPT_U, 0,
  0, 0, 0, 0, 0,
  // Solution
  0, 0, P_MIRROR_TARGET_OPT_TR, 0, 0,
  0, P_MIRROR_TARGET_OPT_BR, 0, P_MIRROR_TARGET_OPT_BL, 0,
  0, P_CHECKPOINT_LCRC, 0, 0, 0,
  P_LASER_R, P_MIRROR_TARGET_REQ_TL, P_SPLIT_TLBR, P_MIRROR_TARGET_OPT_TL, 0,
  0, 0, 0, 0, 0,
  // Hand
  P_LASER_U, 0, 0, 0, 0,
  // Targets
  2,

  // LEVEL 39
  // Puzzle
  0, 0, 0, 0, 0,
  0, 0, P_MIRROR_TARGET_OPT_BL, 0, 0,
  P_MIRROR_TARGET_REQ_TL, 0, 0, P_CELL_BLOCKER, P_MIRROR_TARGET_OPT_U,
  0, P_DBL_MIRROR_U, 0, 0, P_MIRROR_TARGET_OPT_U,
  0, 0, 0, 0, 0,
  // Solution
  0, 0, 0, 0, 0,
  0, P_MIRROR_TARGET_OPT_BR, P_MIRROR_TARGET_OPT_BL, 0, 0,
  P_MIRROR_TARGET_REQ_TL, 0, 0, P_CELL_BLOCKER, P_MIRROR_TARGET_OPT_BL,
  0, P_DBL_MIRROR_TLBR, 0, P_CHECKPOINT_TCBC, P_MIRROR_TARGET_OPT_TL,
  0, 0, P_LASER_T, 0, 0,
  // Hand
  P_LASER_U, P_MIRROR_TARGET_OPT_U, P_CHECKPOINT_U, 0, 0,
  // Targets
  1,

  // LEVEL 40
  // Puzzle
  P_LASER_U, 0, 0, 0, 0,
  0, 0, P_CHECKPOINT_LCRC, 0, 0,
  0, P_DBL_MIRROR_TLBR, 0, 0, 0,
  0, 0, 0, 0, P_MIRROR_TARGET_REQ_BR,
  0, 0, 0, P_MIRROR_TARGET_REQ_BL, 0,
  // Solution
  P_LASER_R, P_SPLIT_TLBR, P_MIRROR_TARGET_OPT_BL, 0, 0,
  0, 0, P_CHECKPOINT_LCRC, 0, 0,
  0, P_DBL_MIRROR_TLBR, 0, P_MIRROR_TARGET_OPT_BL, 0,
  0, 0, P_MIRROR_TARGET_OPT_TR, 0, P_MIRROR_TARGET_REQ_BR,
  0, 0, 0, P_MIRROR_TARGET_REQ_BL, 0,
  // Hand
  P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_OPT_U, P_SPLIT_U, 0,
  // Targets
  2,

  // LEVEL 41
  // Puzzle
  0, 0, P_LASER_B, 0, 0,
  0, 0, 0, 0, P_MIRROR_TARGET_REQ_U,
  0, P_MIRROR_TARGET_REQ_TR, 0, 0, 0,
  0, 0, P_MIRROR_TARGET_REQ_TL, 0, 0,
  0, 0, 0, 0, 0,
  // Solution
  0, 0, P_LASER_B, 0, 0,
  0, 0, P_SPLIT_TLBR, P_SPLIT_TLBR, P_MIRROR_TARGET_REQ_BR,
  0, P_MIRROR_TARGET_REQ_TR, 0, 0, 0,
  0, P_MIRROR_TARGET_OPT_TR, P_MIRROR_TARGET_REQ_TL, P_MIRROR_TARGET_OPT_TL, 0,
  0, 0, 0, 0, 0,
  // Hand
  P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_OPT_U, P_SPLIT_U, P_SPLIT_U, 0,
  // Targets
  3,

  // LEVEL 42
  // Puzzle
  0, 0, 0, 0, P_MIRROR_TARGET_REQ_U,
  0, 0, 0, 0, 0,
  0, P_CHECKPOINT_LCRC, 0, 0, 0,
  P_MIRROR_TARGET_REQ_U, P_CELL_BLOCKER, P_SPLIT_U, 0, 0,
  0, 0, P_MIRROR_TARGET_OPT_U, 0, P_MIRROR_TARGET_REQ_U,
  // Solution
  0, P_DBL_MIRROR_TRBL, P_SPLIT_TLBR, 0, P_MIRROR_TARGET_REQ_BR,
  0, 0, 0, 0, 0,
  0, P_CHECKPOINT_LCRC, 0, 0, 0,
  P_MIRROR_TARGET_REQ_TL, P_CELL_BLOCKER, P_SPLIT_TRBL, 0, 0,
  0, P_LASER_T, P_MIRROR_TARGET_OPT_TR, 0, P_MIRROR_TARGET_REQ_BR,
  // Hand
  P_LASER_U, P_DBL_MIRROR_U, P_SPLIT_U, 0, 0,
  // Targets
  3,

  // LEVEL 43
  // Puzzle
  0, 0, 0, 0, 0,
  0, P_LASER_R, 0, P_DBL_MIRROR_TRBL, 0,
  0, 0, 0, 0, 0,
  0, P_MIRROR_TARGET_REQ_BL, 0, 0, 0,
  0, P_MIRROR_TARGET_REQ_TL, 0, 0, 0,
  // Solution
  0, 0, 0, P_MIRROR_TARGET_OPT_TR, 0,
  0, P_LASER_R, P_SPLIT_TLBR, P_DBL_MIRROR_TRBL, 0,
  0, P_MIRROR_TARGET_OPT_BR, P_SPLIT_TRBL, 0, 0,
  0, P_MIRROR_TARGET_REQ_BL, 0, 0, 0,
  0, P_MIRROR_TARGET_REQ_TL, P_MIRROR_TARGET_OPT_TL, 0, 0,
  // Hand
  P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_OPT_U, P_SPLIT_U, P_SPLIT_U,
  // Targets
  3,

  // LEVEL 44
  // Puzzle
  0, P_MIRROR_TARGET_REQ_TL, 0, P_MIRROR_TARGET_OPT_U, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, P_DBL_MIRROR_U, 0,
  0, 0, 0, P_CHECKPOINT_U, 0,
  0, P_MIRROR_TARGET_OPT_U, 0, 0, 0,
  // Solution
  0, P_MIRROR_TARGET_REQ_TL, 0, P_MIRROR_TARGET_OPT_BL, 0,
  0, 0, 0, 0, 0,
  0, P_MIRROR_TARGET_OPT_BR, 0, P_DBL_MIRROR_TRBL, P_LASER_L,
  0, 0, 0, P_CHECKPOINT_LCRC, 0,
  0, P_MIRROR_TARGET_OPT_TR, 0, P_MIRROR_TARGET_OPT_TL, 0,
  // Hand
  P_LASER_U, P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_OPT_U, 0, 0,
  // Targets
  1,

  // LEVEL 45
  // Puzzle
  0, P_MIRROR_TARGET_REQ_U, P_MIRROR_TARGET_OPT_U, 0, 0,
  0, P_CELL_BLOCKER, 0, 0, 0,
  0, 0, 0, P_MIRROR_TARGET_OPT_U, 0,
  0, P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_OPT_BR, 0, 0,
  0, 0, 0, 0, 0,
  // Solution
  0, P_MIRROR_TARGET_REQ_TL, P_MIRROR_TARGET_OPT_BL, 0, 0,
  0, P_CELL_BLOCKER, P_CHECKPOINT_LCRC, 0, 0,
  P_LASER_R, P_SPLIT_TLBR, P_SPLIT_TRBL, P_MIRROR_TARGET_OPT_BR, 0,
  0, P_MIRROR_TARGET_OPT_TR, P_MIRROR_TARGET_OPT_BR, 0, 0,
  0, 0, 0, 0, 0,
  // Hand
  P_LASER_U, P_SPLIT_U, P_SPLIT_U, P_CHECKPOINT_U, 0,
  // Targets
  3,

  // LEVEL 46
  // Puzzle
  0, P_MIRROR_TARGET_REQ_U, P_MIRROR_TARGET_OPT_U, 0, 0,
  0, P_CELL_BLOCKER, 0, 0, 0,
  0, 0, 0, P_MIRROR_TARGET_OPT_U, 0,
  0, P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_OPT_BL, 0, 0,
  0, 0, 0, 0, 0,
  // Solution
  0, P_MIRROR_TARGET_REQ_TR, P_MIRROR_TARGET_OPT_TR, 0, 0,
  0, P_CELL_BLOCKER, P_CHECKPOINT_LCRC, 0, 0,
  0, P_SPLIT_TRBL, P_SPLIT_TRBL, P_MIRROR_TARGET_OPT_BR, 0,
  0, P_MIRROR_TARGET_OPT_TR, P_MIRROR_TARGET_OPT_BL, 0, 0,
  0, 0, P_LASER_T, 0, 0,
  // Hand
  P_LASER_U, P_SPLIT_U, P_SPLIT_U, P_CHECKPOINT_U, 0,
  // Targets
  3,

  // LEVEL 47
  // Puzzle
  0, 0, 0, 0, P_MIRROR_TARGET_REQ_U,
  0, 0, 0, 0, 0,
  0, P_CHECKPOINT_TCBC, 0, 0, 0,
  P_MIRROR_TARGET_REQ_U, P_CELL_BLOCKER, P_SPLIT_U, 0, 0,
  0, 0, P_MIRROR_TARGET_OPT_U, 0, P_MIRROR_TARGET_REQ_U,
  // Solution
  0, 0, 0, 0, P_MIRROR_TARGET_REQ_TR,
  0, 0, 0, 0, 0,
  P_LASER_R, P_CHECKPOINT_TCBC, P_SPLIT_TLBR, 0, P_DBL_MIRROR_TRBL,
  P_MIRROR_TARGET_REQ_TL, P_CELL_BLOCKER, P_SPLIT_TRBL, 0, 0,
  0, 0, P_MIRROR_TARGET_OPT_TR, 0, P_MIRROR_TARGET_REQ_BR,
  // Hand
  P_LASER_U, P_DBL_MIRROR_U, P_SPLIT_U, 0, 0,
  // Targets
  3,

  // LEVEL 48
  // Puzzle
  0, 0, 0, 0, P_LASER_B,
  P_CELL_BLOCKER, P_MIRROR_TARGET_REQ_TR, 0, 0, 0,
  0, P_DBL_MIRROR_TRBL, 0, 0, 0,
  0, P_MIRROR_TARGET_REQ_BL, 0, 0, 0,
  0, 0, 0, 0, 0,
  // Solution
  P_MIRROR_TARGET_OPT_TR, 0, 0, 0, P_LASER_B,
  P_CELL_BLOCKER, P_MIRROR_TARGET_REQ_TR, 0, 0, 0,
  P_SPLIT_TRBL, P_DBL_MIRROR_TRBL, 0, 0, P_SPLIT_TRBL,
  0, P_MIRROR_TARGET_REQ_BL, 0, 0, 0,
  P_MIRROR_TARGET_OPT_TR, 0, 0, 0, P_MIRROR_TARGET_OPT_TL,
  // Hand
  P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_OPT_U, P_SPLIT_U, P_SPLIT_U,
  // Targets
  3,

  // LEVEL 49
  // Puzzle
  0, 0, 0, 0, 0,
  0, P_SPLIT_U, 0, 0, P_MIRROR_TARGET_OPT_U,
  0, 0, 0, 0, P_LASER_L,
  0, 0, P_SPLIT_U, 0, 0,
  0, 0, 0, 0, 0,
  // Solution
  0, P_MIRROR_TARGET_OPT_TR, 0, 0, 0,
  0, P_SPLIT_TRBL, 0, 0, P_MIRROR_TARGET_OPT_BR,
  0, 0, P_MIRROR_TARGET_OPT_BR, 0, P_LASER_L,
  0, P_MIRROR_TARGET_OPT_TR, P_SPLIT_TRBL, 0, 0,
  0, 0, P_MIRROR_TARGET_OPT_BL, 0, 0,
  // Hand
  P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_OPT_U, 0,
  // Targets
  3,

  // LEVEL 50
  // Puzzle
  P_MIRROR_TARGET_REQ_U, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, P_CHECKPOINT_TCBC, 0,
  0, P_SPLIT_TLBR, P_MIRROR_TARGET_REQ_U, 0, 0,
  0, 0, 0, P_CELL_BLOCKER, P_MIRROR_TARGET_REQ_U,
  // Solution
  P_MIRROR_TARGET_REQ_TR, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  P_MIRROR_TARGET_OPT_TR, P_SPLIT_TRBL, 0, P_CHECKPOINT_TCBC, P_LASER_L,
  0, P_SPLIT_TLBR, P_MIRROR_TARGET_REQ_BR, 0, 0,
  0, P_MIRROR_TARGET_OPT_TR, 0, P_CELL_BLOCKER, P_MIRROR_TARGET_REQ_BR,
  // Hand
  P_LASER_U, P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_OPT_U, P_SPLIT_U, 0,
  // Targets
  3,

  // LEVEL 51
  // Puzzle
  0, 0, P_LASER_B, 0, 0,
  0, 0, 0, P_MIRROR_TARGET_REQ_U, 0,
  0, 0, 0, 0, 0,
  0, P_MIRROR_TARGET_REQ_BL, 0, P_MIRROR_TARGET_REQ_U, 0,
  P_DBL_MIRROR_TLBR, 0, 0, 0, 0,
  // Solution
  0, 0, P_LASER_B, 0, 0,
  P_MIRROR_TARGET_OPT_BR, P_SPLIT_TLBR, 0, P_MIRROR_TARGET_REQ_BR, 0,
  0, 0, 0, 0, 0,
  0, P_MIRROR_TARGET_REQ_BL, P_SPLIT_TLBR, P_MIRROR_TARGET_REQ_BR, 0,
  P_DBL_MIRROR_TLBR, 0, P_MIRROR_TARGET_OPT_TL, 0, 0,
  // Hand
  P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_OPT_U, P_SPLIT_U, P_SPLIT_U, 0,
  // Targets
  3,

  // LEVEL 52
  // Puzzle
  0, 0, 0, 0, 0,
  0, 0, P_SPLIT_U, 0, P_MIRROR_TARGET_OPT_U,
  0, 0, 0, 0, P_LASER_L,
  0, 0, 0, 0, 0,
  0, P_SPLIT_U, 0, 0, 0,
  // Solution
  0, 0, 0, 0, 0,
  0, P_MIRROR_TARGET_OPT_BR, P_SPLIT_TLBR, 0, P_MIRROR_TARGET_OPT_BR,
  0, P_MIRROR_TARGET_OPT_TR, 0, 0, P_LASER_L,
  0, 0, 0, 0, 0,
  P_MIRROR_TARGET_OPT_TL, P_SPLIT_TLBR, P_MIRROR_TARGET_OPT_TL, 0, 0,
  // Hand
  P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_OPT_U, 0,
  // Targets
  3,

  // LEVEL 53
  // Puzzle
  0, 0, 0, P_MIRROR_TARGET_REQ_U, 0,
  0, 0, 0, 0, 0,
  P_CELL_BLOCKER, 0, P_MIRROR_TARGET_REQ_BR, 0, 0,
  0, P_CHECKPOINT_U, 0, 0, P_MIRROR_TARGET_REQ_U,
  P_DBL_MIRROR_TLBR, 0, P_LASER_U, 0, 0,
  // Solution
  P_MIRROR_TARGET_OPT_BR, P_SPLIT_TLBR, 0, P_MIRROR_TARGET_REQ_BR, 0,
  0, 0, 0, 0, 0,
  P_CELL_BLOCKER, P_MIRROR_TARGET_OPT_TR, P_MIRROR_TARGET_REQ_BR, 0, 0,
  P_SPLIT_TRBL, P_CHECKPOINT_TCBC, 0, 0, P_MIRROR_TARGET_REQ_BR,
  P_DBL_MIRROR_TLBR, 0, P_LASER_L, 0, 0,
  // Hand
  P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_OPT_U, P_SPLIT_U, P_SPLIT_U, 0,
  // Targets
  3,

  // LEVEL 54
  // Puzzle
  0, P_SPLIT_U, 0, 0, P_MIRROR_TARGET_OPT_U,
  0, 0, 0, P_DBL_MIRROR_U, 0,
  0, 0, P_MIRROR_TARGET_REQ_TR, 0, 0,
  0, P_MIRROR_TARGET_REQ_BL, 0, 0, 0,
  0, 0, 0, P_MIRROR_TARGET_OPT_U, 0,
  // Solution
  P_LASER_R, P_SPLIT_TLBR, 0, 0, P_MIRROR_TARGET_OPT_BR,
  0, P_SPLIT_TLBR, 0, P_DBL_MIRROR_TLBR, 0,
  0, 0, P_MIRROR_TARGET_REQ_TR, 0, 0,
  0, P_MIRROR_TARGET_REQ_BL, 0, 0, 0,
  0, 0, P_MIRROR_TARGET_OPT_TR, P_MIRROR_TARGET_OPT_TL, 0,
  // Hand
  P_LASER_U, P_MIRROR_TARGET_OPT_U, P_SPLIT_U, 0, 0,
  // Targets
  3,

  // LEVEL 55
  // Puzzle
  0, 0, 0, 0, 0,
  0, P_LASER_U, 0, P_MIRROR_TARGET_OPT_U, 0,
  0, 0, P_MIRROR_TARGET_OPT_U, 0, 0,
  0, P_MIRROR_TARGET_OPT_U, 0, 0, P_MIRROR_TARGET_OPT_U,
  0, 0, P_MIRROR_TARGET_OPT_U, P_DBL_MIRROR_U, 0,
  // Solution
  0, 0, 0, 0, 0,
  0, P_LASER_B, 0, P_MIRROR_TARGET_OPT_TR, 0,
  0, P_SPLIT_TLBR, P_MIRROR_TARGET_OPT_BL, P_CHECKPOINT_LCRC, 0,
  0, P_MIRROR_TARGET_OPT_TR, 0, 0, P_MIRROR_TARGET_OPT_BR,
  0, 0, P_MIRROR_TARGET_OPT_TR, P_DBL_MIRROR_TRBL, 0,
  // Hand
  P_SPLIT_U, P_CHECKPOINT_U, 0, 0, 0,
  // Targets
  2,

  // LEVEL 56
  // Puzzle
  0, P_MIRROR_TARGET_REQ_U, 0, 0, 0,
  0, 0, 0, 0, P_LASER_U,
  0, P_CHECKPOINT_U, 0, 0, 0,
  0, 0, P_MIRROR_TARGET_REQ_U, 0, 0,
  0, 0, P_MIRROR_TARGET_REQ_BL, 0, 0,
  // Solution
  0, P_MIRROR_TARGET_REQ_TR, 0, 0, 0,
  P_MIRROR_TARGET_OPT_BR, 0, P_SPLIT_TRBL, 0, P_LASER_L,
  0, P_CHECKPOINT_LCRC, 0, 0, 0,
  P_MIRROR_TARGET_OPT_TR, P_SPLIT_TRBL, P_MIRROR_TARGET_REQ_BL, 0, 0,
  0, 0, P_MIRROR_TARGET_REQ_BL, 0, 0,
  // Hand
  P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_OPT_U, P_SPLIT_U, P_SPLIT_U, 0,
  // Targets
  3,

  // LEVEL 57
  // Puzzle
  P_CELL_BLOCKER, 0, P_MIRROR_TARGET_OPT_BR, 0, 0,
  0, P_CHECKPOINT_U, 0, 0, 0,
  0, P_MIRROR_TARGET_OPT_TL, 0, P_DBL_MIRROR_U, 0,
  0, 0, 0, 0, P_MIRROR_TARGET_OPT_U,
  P_LASER_U, 0, 0, 0, 0,
  // Solution
  P_CELL_BLOCKER, P_MIRROR_TARGET_OPT_BR, P_MIRROR_TARGET_OPT_BR, 0, 0,
  0, P_CHECKPOINT_LCRC, 0, 0, 0,
  P_MIRROR_TARGET_OPT_BR, P_MIRROR_TARGET_OPT_TL, 0, P_DBL_MIRROR_TLBR, 0,
  P_SPLIT_TRBL, 0, 0, P_SPLIT_TRBL, P_MIRROR_TARGET_OPT_BR,
  P_LASER_T, 0, 0, 0, 0,
  // Hand
  P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_OPT_U, P_SPLIT_U, P_SPLIT_U, 0,
  // Targets
  3,

  // LEVEL 58
  // Puzzle
  0, 0, 0, 0, 0,
  0, 0, P_CHECKPOINT_U, 0, 0,
  P_MIRROR_TARGET_OPT_TR, 0, 0, 0, 0,
  P_MIRROR_TARGET_OPT_U, 0, P_LASER_U, 0, 0,
  P_MIRROR_TARGET_OPT_TR, 0, 0, 0, P_DBL_MIRROR_TRBL,
  // Solution
  0, 0, 0, 0, 0,
  0, P_MIRROR_TARGET_OPT_BR, P_CHECKPOINT_TCBC, 0, P_MIRROR_TARGET_OPT_BL,
  P_MIRROR_TARGET_OPT_TR, 0, 0, 0, 0,
  P_MIRROR_TARGET_OPT_TR, P_SPLIT_TLBR, P_LASER_L, 0, 0,
  P_MIRROR_TARGET_OPT_TR, 0, 0, 0, P_DBL_MIRROR_TRBL,
  // Hand
  P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_OPT_U, P_SPLIT_U, 0, 0,
  // Targets
  2,

  // LEVEL 59
  // Puzzle
  0, 0, 0, 0, 0,
  P_MIRROR_TARGET_OPT_U, 0, P_CELL_BLOCKER, P_SPLIT_U, 0,
  P_MIRROR_TARGET_REQ_U, 0, P_DBL_MIRROR_U, 0, 0,
  0, P_LASER_T, 0, P_CHECKPOINT_U, 0,
  0, 0, 0, 0, 0,
  // Solution
  0, P_MIRROR_TARGET_OPT_BR, P_SPLIT_TLBR, P_MIRROR_TARGET_OPT_BL, 0,
  P_MIRROR_TARGET_OPT_TL, 0, P_CELL_BLOCKER, P_SPLIT_TRBL, 0,
  P_MIRROR_TARGET_REQ_TL, 0, P_DBL_MIRROR_TRBL, 0, 0,
  0, P_LASER_T, 0, P_CHECKPOINT_LCRC, 0,
  0, 0, 0, P_MIRROR_TARGET_OPT_BL, 0,
  // Hand
  P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_OPT_U, P_SPLIT_U, 0,
  // Targets
  3,

  // LEVEL 60
  // Puzzle
  0, 0, 0, P_MIRROR_TARGET_REQ_BR, 0,
  P_MIRROR_TARGET_OPT_TR, 0, 0, 0, 0,
  0, P_CELL_BLOCKER, P_CHECKPOINT_U, 0, 0,
  0, 0, 0, 0, P_MIRROR_TARGET_REQ_BL,
  0, P_DBL_MIRROR_U, 0, 0, 0,
  // Solution
  P_LASER_R, P_SPLIT_TLBR, 0, P_MIRROR_TARGET_REQ_BR, 0,
  P_MIRROR_TARGET_OPT_TR, 0, 0, 0, 0,
  P_SPLIT_TRBL, P_CELL_BLOCKER, P_CHECKPOINT_TCBC, 0, P_MIRROR_TARGET_OPT_BL,
  0, 0, 0, 0, P_MIRROR_TARGET_REQ_BL,
  P_MIRROR_TARGET_OPT_TR, P_DBL_MIRROR_TRBL, 0, 0, 0,
  // Hand
  P_LASER_U, P_MIRROR_TARGET_OPT_U, P_MIRROR_TARGET_OPT_U, P_SPLIT_U, P_SPLIT_U,
  // Targets
  3,

#if 0
  // LEVEL ?
  // Puzzle
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  // Solution
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  // Hand
  0, 0, 0, 0, 0,
  // Targets
  0,
#endif
};

#define LEVEL_SIZE 56
#define LEVELS (sizeof(levelData) / LEVEL_SIZE)

/* Some pieces are fixed in position, but may be rotated.  These are
   drawn facing their default directions, but with a rotation overlay
   sprite, rather than with a padlock overlay sprite. */
bool NeedsRotationOverlay(uint8_t piece)
{
  return ((piece == P_LASER_U) ||
          (piece == P_MIRROR_TARGET_OPT_U) ||
          (piece == P_MIRROR_TARGET_REQ_U) ||
          (piece == P_SPLIT_U) ||
          (piece == P_DBL_MIRROR_U) ||
          (piece == P_CHECKPOINT_U));
}

/* If a piece is defined as having an unknown rotation, it needs to
   have its rotate bit set, and a default direction should be
   chosen. */
uint8_t DefaultDirection(uint8_t piece)
{
  switch (piece) {
  case P_LASER_U:
    return P_LASER_B;
  case P_MIRROR_TARGET_OPT_U:
    return P_MIRROR_TARGET_OPT_TR;
  case P_MIRROR_TARGET_REQ_U:
    return P_MIRROR_TARGET_REQ_TR;
  case P_SPLIT_U:
    return P_SPLIT_TLBR;
  case P_DBL_MIRROR_U:
    return P_DBL_MIRROR_TLBR;
  case P_CHECKPOINT_U:
    return P_CHECKPOINT_TCBC;
  default:
    return piece;
  }
}

/* Maps from a numeric piece number to a pointer to the equivalent
   tilemap. Unknown rotations are not included. They need to have a
   valid direction. */
const VRAM_PTR_TYPE* MapName(uint8_t piece)
{
  switch (piece) {
  case P_BLANK:
    return map_blank;
  case P_LASER_T:
    return map_laser_t;
  case P_LASER_R:
    return map_laser_r;
  case P_LASER_B:
    return map_laser_b;
  case P_LASER_L:
    return map_laser_l;
  case P_MIRROR_TARGET_OPT_BR:
    return map_mirror_target_opt_br;
  case P_MIRROR_TARGET_OPT_BL:
    return map_mirror_target_opt_bl;
  case P_MIRROR_TARGET_OPT_TL:
    return map_mirror_target_opt_tl;
  case P_MIRROR_TARGET_OPT_TR:
    return map_mirror_target_opt_tr;
  case P_MIRROR_TARGET_REQ_BR:
    return map_mirror_target_req_br;
  case P_MIRROR_TARGET_REQ_BL:
    return map_mirror_target_req_bl;
  case P_MIRROR_TARGET_REQ_TL:
    return map_mirror_target_req_tl;
  case P_MIRROR_TARGET_REQ_TR:
    return map_mirror_target_req_tr;
  case P_SPLIT_TRBL:
    return map_split_trbl;
  case P_SPLIT_TLBR:
    return map_split_tlbr;
  case P_DBL_MIRROR_TRBL:
    return map_dbl_mirror_trbl;
  case P_DBL_MIRROR_TLBR:
    return map_dbl_mirror_tlbr;
  case P_CHECKPOINT_TCBC:
    return map_checkpoint_tcbc;
  case P_CHECKPOINT_LCRC:
    return map_checkpoint_lcrc;
  case P_CELL_BLOCKER:
    return map_cell_blocker;
  }
  return map_blank;
}

/*
 * BCD_addConstant
 *
 * Adds a constant (binary number) to a BCD number
 *
 * num [in, out]
 *   The BCD number
 *
 * digits [in]
 *   The number of digits in the BCD number, num
 *
 * x [in]
 *   The binary value to be added to the BCD number
 *
 *   Note: The largest value that can be safely added to a BCD number
 *         is BCD_ADD_CONSTANT_MAX. If the result would overflow num,
 *         then num will be clamped to its maximum value (all 9's).
 *
 * Returns:
 *   A boolean that is true if num has been clamped to its maximum
 *   value (all 9's), or false otherwise.
 */
#define BCD_ADD_CONSTANT_MAX 244
static bool BCD_addConstant(uint8_t* const num, const uint8_t digits, uint8_t x)
{
  for (uint8_t i = 0; i < digits; ++i) {
    uint8_t val = num[i] + x;
    if (val < 10) { // speed up the common cases
      num[i] = val;
      x = 0;
      break;
    } else if (val < 20) {
      num[i] = val - 10;
      x = 1;
    } else if (val < 30) {
      num[i] = val - 20;
      x = 2;
    } else if (val < 40) {
      num[i] = val - 30;
      x = 3;
    } else { // handle the rest of the cases (up to 255 - 9) with a loop
      for (uint8_t j = 5; j < 26; ++j) {
        if (val < (j * 10)) {
          num[i] = val - ((j - 1) * 10);
          x = (j - 1);
          break;
        }
      }
    }
  }

  if (x > 0) {
    for (uint8_t i = 0; i < digits; ++i)
      num[i] = 9;
    return true;
  }

  return false;
}

#define PREV_NEXT_X 2
#define PREV_NEXT_Y 25
#define FIRST_DIGIT_SPRITE 3
// The highest sprite index is for the "mouse cursor"
// and the 9 highest below that are reserved for drag-and-drop
#define RESERVED_SPRITES 10

static void LoadLevel(const uint8_t level, bool solution)
{
  for (uint8_t i = 0; i < MAX_SPRITES - 1; ++i)
    sprites[i].x = OFF_SCREEN;
  
  ClearVram();
  
  // Draw a colored strip along the bottom that corresponds to the level difficulty
  uint8_t color = TILE_BACKGROUND;
  if ((level >= 1) && (level <= 15))
    color = TILE_GREEN;
  else if ((level >= 16) && (level <= 30))
    color = TILE_YELLOW;
  else if ((level >= 31) && (level <= 45))
    color = TILE_BLUE;
  else if ((level >= 46) && (level <= 60))
    color = TILE_RED;
  for (uint8_t h = 0; h < VRAM_TILES_H; ++h)
    SetTile(h, VRAM_TILES_V - 1, color);
  
  DrawMap(1, 3, map_laser_puzzle_ii);
  
  DrawMap(9, 22, map_move_to_grid);

  DrawMap(PREV_NEXT_X, PREV_NEXT_Y, map_prev);
  DrawMap(PREV_NEXT_X + 2, PREV_NEXT_Y, map_next);

  DrawMap(PREV_NEXT_X + 1, 9, map_level);
  
  uint8_t levelDisplay[2] = {0};
  BCD_addConstant(levelDisplay, 2, level);
  // Since we ran out of unique background tile indices, we have to resort to using sprites
  sprites[0].tileIndex = levelDisplay[0] + FIRST_DIGIT_SPRITE;
  sprites[1].tileIndex = levelDisplay[1] + FIRST_DIGIT_SPRITE;
  
  sprites[0].y = 11 * TILE_HEIGHT;
  sprites[1].y = 11 * TILE_HEIGHT;
  sprites[0].x = (PREV_NEXT_X + 2) * TILE_WIDTH;
  sprites[1].x = (PREV_NEXT_X + 1) * TILE_WIDTH;

  
  const uint16_t levelOffset = (level - 1) * LEVEL_SIZE;
  
  DrawMap(PREV_NEXT_X, 17, map_targets);
  uint8_t targets = pgm_read_byte(&levelData[levelOffset + LEVEL_SIZE - 1]);
  sprites[2].tileIndex = targets + FIRST_DIGIT_SPRITE;
  sprites[2].x = (PREV_NEXT_X + 1) * TILE_WIDTH + (TILE_WIDTH / 2);
  sprites[2].y = 19 * TILE_HEIGHT;

  uint8_t currentSprite = 3;
  for (uint8_t y = 0; y < 5; ++y)
    for (uint8_t x = 0; x < 5; ++x) {
      uint8_t piece = (uint8_t)pgm_read_byte(&levelData[(levelOffset + (solution ? 25 : 0)) + y * 5 + x]);
      bool rotationBit = NeedsRotationOverlay(piece);
      piece = DefaultDirection(piece);
      if (rotationBit)
        board[y][x] = piece | 0x40; // set the rotation bit
      else
        board[y][x] = piece | 0x80; // set the lock bit
      DrawMap(9 + x * 4, 1 + y * 4, MapName(piece));
      /* Any pieces that are part of the inital setup can't be moved,
         so add either a lock or rotate icon */
      if ((piece != P_BLANK) && (currentSprite < (MAX_SPRITES - RESERVED_SPRITES))
          && (solution == false)) {
        sprites[currentSprite].tileIndex = rotationBit ? 1 : 0;
        sprites[currentSprite].x = (11 + x * 4) * TILE_WIDTH;
        sprites[currentSprite].y = (3 + y * 4) * TILE_HEIGHT;
        ++currentSprite;
      }
    }
  
  if (solution) {
    for (uint8_t x = 0; x < 5; ++x)
      DrawMap(9 + x * 4, 23, map_blank);
    return;
  }
    
  for (uint8_t x = 0; x < 5; ++x) {
    uint8_t piece = (uint8_t)pgm_read_byte(&levelData[(levelOffset + 50) + x]);
    piece = DefaultDirection(piece);
    hand[x] = piece;
    DrawMap(9 + x * 4, 23, MapName(piece));
  }
}

void SimulatePhoton(void)
{
  // First find where the laser piece is. If it's not on the grid, then don't turn it on.
  int8_t laser_x = -1;
  int8_t laser_y = -1;
  uint8_t laser_d = 0;

  for (uint8_t y = 0; y < 5; ++y)
    for (uint8_t x = 0; x < 5; ++x)
      switch (board[y][x] & 0x1F) { // ignore the flag bits
      case P_LASER_T:
        laser[y][x] |= D_OUT_T;
        laser_x = x;
        laser_y = y - 1;
        laser_d = D_IN_B;
        break;
      case P_LASER_R:
        laser[y][x] |= D_OUT_R;
        laser_x = x + 1;
        laser_y = y;
        laser_d = D_IN_L;
        break;
      case P_LASER_B:
        laser[y][x] |= D_OUT_B;
        laser_x = x;
        laser_y = y + 1;
        laser_d = D_IN_T;
        break;
      case P_LASER_L:
        laser[y][x] |= D_OUT_L;
        laser_x = x - 1;
        laser_y = y;
        laser_d = D_IN_R;
        break;
      }
  
  bool bounce = false;
  bool halt = false;
  uint8_t ttl = 0;
  while (!halt && (++ttl != 0) && laser_x >= 0 && laser_x <= 4 && laser_y >= 0 && laser_y <= 4)
    switch (board[laser_y][laser_x] & 0x1F) { // ignore the flag bits
    case P_BLANK:
    case P_CELL_BLOCKER:
      switch (laser_d) {
      case D_IN_T:
        laser[laser_y][laser_x] |= (D_IN_T | D_OUT_B);
        laser_y++;
        break;
      case D_IN_B:
        laser[laser_y][laser_x] |= (D_IN_B | D_OUT_T);
        laser_y--;
        break;
      case D_IN_L:
        laser[laser_y][laser_x] |= (D_IN_L | D_OUT_R);
        laser_x++;
        break;
      case D_IN_R:
        laser[laser_y][laser_x] |= (D_IN_R | D_OUT_L);
        laser_x--;
        break;
      }
      break;

    case P_LASER_T:
    case P_LASER_R:
    case P_LASER_B:
    case P_LASER_L:
      halt = true;
      break;

    case P_MIRROR_TARGET_OPT_BR:
    case P_MIRROR_TARGET_REQ_BR:
      switch (laser_d) {
      case D_IN_T:
        halt = true;
        break;
      case D_IN_B:
        laser[laser_y][laser_x] |= (D_IN_B | D_OUT_R);
        laser_d = D_IN_L;
        laser_x++;
        break;
      case D_IN_L:
        laser[laser_y][laser_x] |= laser_d;
        halt = true;
        break;
      case D_IN_R:
        laser[laser_y][laser_x] |= (D_IN_R | D_OUT_B);
        laser_d = D_IN_T;
        laser_y++;
        break;
      }
      break;

    case P_MIRROR_TARGET_OPT_BL:
    case P_MIRROR_TARGET_REQ_BL:
      switch (laser_d) {
      case D_IN_T:
        laser[laser_y][laser_x] |= laser_d;
        halt = true;
        break;
      case D_IN_B:
        laser[laser_y][laser_x] |= (D_IN_B | D_OUT_L);
        laser_d = D_IN_R;
        laser_x--;
        break;
      case D_IN_L:
        laser[laser_y][laser_x] |= (D_IN_L | D_OUT_B);
        laser_d = D_IN_T;
        laser_y++;
        break;
      case D_IN_R:
        halt = true;
        break;
      }
      break;

    case P_MIRROR_TARGET_OPT_TL:
    case P_MIRROR_TARGET_REQ_TL:
      switch (laser_d) {
      case D_IN_T:
        laser[laser_y][laser_x] |= (D_IN_T | D_OUT_L);
        laser_d = D_IN_R;
        laser_x--;
        break;
      case D_IN_B:
        halt = true;
        break;
      case D_IN_L:
        laser[laser_y][laser_x] |= (D_IN_L | D_OUT_T);
        laser_d = D_IN_B;
        laser_y--;
        break;
      case D_IN_R:
        laser[laser_y][laser_x] |= laser_d;
        halt = true;
        break;
      }
      break;

    case P_MIRROR_TARGET_OPT_TR:
    case P_MIRROR_TARGET_REQ_TR:
      switch (laser_d) {
      case D_IN_T:
        laser[laser_y][laser_x] |= (D_IN_T | D_OUT_R);
        laser_d = D_IN_L;
        laser_x++;
        break;
      case D_IN_B:
        laser[laser_y][laser_x] |= laser_d;
        halt = true;
        break;
      case D_IN_L:
        halt = true;
        break;
      case D_IN_R:
        laser[laser_y][laser_x] |= (D_IN_R | D_OUT_T);
        laser_d = D_IN_B;
        laser_y--;
        break;
      }
      break;

    case P_SPLIT_TRBL:
      // Generate a random number, and decide whether the beam passes through, or bounces
      bounce = (rand() > (RAND_MAX / 2));
      switch (laser_d) {
      case D_IN_T:
        if (bounce) {
          laser[laser_y][laser_x] |= (D_IN_T | D_OUT_L);
          laser_d = D_IN_R;
          laser_x--;
        } else {
          laser[laser_y][laser_x] |= (D_IN_T | D_OUT_B);
          laser_y++;
        }
        break;
      case D_IN_B:
        if (bounce) {
          laser[laser_y][laser_x] |= (D_IN_B | D_OUT_R);
          laser_d = D_IN_L;
          laser_x++;
        } else {
          laser[laser_y][laser_x] |= (D_IN_B | D_OUT_T);
          laser_y--;
        }
        break;
      case D_IN_L:
        if (bounce) {
          laser[laser_y][laser_x] |= (D_IN_L | D_OUT_T);
          laser_d = D_IN_B;
          laser_y--;
        } else {
          laser[laser_y][laser_x] |= (D_IN_L | D_OUT_R);
          laser_x++;
        }
        break;
      case D_IN_R:
        if (bounce) {
          laser[laser_y][laser_x] |= (D_IN_R | D_OUT_B);
          laser_d = D_IN_T;
          laser_y++;
        } else {
          laser[laser_y][laser_x] |= (D_IN_R | D_OUT_L);
          laser_x--;
        }
        break;
      }
      break;

    case P_SPLIT_TLBR:
      // Generate a random number, and decide whether the beam passes through, or bounces
      bounce = (rand() > (RAND_MAX / 2));
      switch (laser_d) {
      case D_IN_T:
        if (bounce) {
          laser[laser_y][laser_x] |= (D_IN_T | D_OUT_R);
          laser_d = D_IN_L;
          laser_x++;
        } else {
          laser[laser_y][laser_x] |= (D_IN_T | D_OUT_B);
          laser_y++;
        }
        break;
      case D_IN_B:
        if (bounce) {
          laser[laser_y][laser_x] |= (D_IN_B | D_OUT_L);
          laser_d = D_IN_R;
          laser_x--;
        } else {
          laser[laser_y][laser_x] |= (D_IN_B | D_OUT_T);
          laser_y--;
        }
        break;
      case D_IN_L:
        if (bounce) {
          laser[laser_y][laser_x] |= (D_IN_L | D_OUT_B);
          laser_d = D_IN_T;
          laser_y++;
        } else {
          laser[laser_y][laser_x] |= (D_IN_L | D_OUT_R);
          laser_x++;
        }
        break;
      case D_IN_R:
        if (bounce) {
          laser[laser_y][laser_x] |= (D_IN_R | D_OUT_T);
          laser_d = D_IN_B;
          laser_y--;
        } else {
          laser[laser_y][laser_x] |= (D_IN_R | D_OUT_L);
          laser_x--;
        }
        break;
      }
      break;

    case P_DBL_MIRROR_TRBL:
      switch (laser_d) {
      case D_IN_T:
        laser[laser_y][laser_x] |= (D_IN_T | D_OUT_L);
        laser_d = D_IN_R;
        laser_x--;
        break;
      case D_IN_B:
        laser[laser_y][laser_x] |= (D_IN_B | D_OUT_R);
        laser_d = D_IN_L;
        laser_x++;
        break;
      case D_IN_L:
        laser[laser_y][laser_x] |= (D_IN_L | D_OUT_T);
        laser_d = D_IN_B;
        laser_y--;
        break;
      case D_IN_R:
        laser[laser_y][laser_x] |= (D_IN_R | D_OUT_B);
        laser_d = D_IN_T;
        laser_y++;
        break;
      }
      break;

    case P_DBL_MIRROR_TLBR:
      switch (laser_d) {
      case D_IN_T:
        laser[laser_y][laser_x] |= (D_IN_T | D_OUT_R);
        laser_d = D_IN_L;
        laser_x++;
        break;
      case D_IN_B:
        laser[laser_y][laser_x] |= (D_IN_B | D_OUT_L);
        laser_d = D_IN_R;
        laser_x--;
        break;
      case D_IN_L:
        laser[laser_y][laser_x] |= (D_IN_L | D_OUT_B);
        laser_d = D_IN_T;
        laser_y++;
        break;
      case D_IN_R:
        laser[laser_y][laser_x] |= (D_IN_R | D_OUT_T);
        laser_d = D_IN_B;
        laser_y--;
        break;
      }
      break;

    case P_CHECKPOINT_TCBC:
      switch (laser_d) {
      case D_IN_T:
        halt = true;
        break;
      case D_IN_B:
        halt = true;
        break;
      case D_IN_L:
        laser[laser_y][laser_x] |= (D_IN_L | D_OUT_R);
        laser_x++;
        break;
      case D_IN_R:
        laser[laser_y][laser_x] |= (D_IN_R | D_OUT_L);
        laser_x--;
        break;
      }
      break;

    case P_CHECKPOINT_LCRC:
      switch (laser_d) {
      case D_IN_T:
        laser[laser_y][laser_x] |= (D_IN_T | D_OUT_B);
        laser_y++;
        break;
      case D_IN_B:
        laser[laser_y][laser_x] |= (D_IN_B | D_OUT_T);
        laser_y--;
        break;
      case D_IN_L:
        halt = true;
        break;
      case D_IN_R:
        halt = true;
        break;
      }
      break;
    }
}

void DrawLaser(void)
{
  /* DrawMap(7, 5, map_laser_source); */
  for (uint8_t y = 0; y < 5; ++y)
    for (uint8_t x = 0; x < 5; ++x) {
      uint8_t l = laser[y][x];
      switch (board[y][x] & 0x1F) { // ignore the flag bits
      case P_BLANK:
        {
          bool h = ((l & D_IN_L) && (l & D_OUT_R)) || ((l & D_IN_R) && (l & D_OUT_L));
          bool v = ((l & D_IN_T) && (l & D_OUT_B)) || ((l & D_IN_B) && (l & D_OUT_T));
          if (h && v)
            DrawMap(9 + x * 4, 1 + y * 4, map_blank_on_hv);
          else if (h)
            DrawMap(9 + x * 4, 1 + y * 4, map_blank_on_h);
          else if (v)
            DrawMap(9 + x * 4, 1 + y * 4, map_blank_on_v);
        }
        break;

      case P_CELL_BLOCKER:
        {
          bool h = ((l & D_IN_L) && (l & D_OUT_R)) || ((l & D_IN_R) && (l & D_OUT_L));
          bool v = ((l & D_IN_T) && (l & D_OUT_B)) || ((l & D_IN_B) && (l & D_OUT_T));
          if (h && v)
            DrawMap(9 + x * 4, 1 + y * 4, map_cell_blocker_on_hv);
          else if (h)
            DrawMap(9 + x * 4, 1 + y * 4, map_cell_blocker_on_h);
          else if (v)
            DrawMap(9 + x * 4, 1 + y * 4, map_cell_blocker_on_v);
        }
        break;

      case P_LASER_T:
        if (l & D_OUT_T)
          DrawMap(9 + x * 4, 1 + y * 4, map_laser_on_t);
        break;

      case P_LASER_R:
        if (l & D_OUT_R)
          DrawMap(9 + x * 4, 1 + y * 4, map_laser_on_r);
        break;

      case P_LASER_B:
        if (l & D_OUT_B)
          DrawMap(9 + x * 4, 1 + y * 4, map_laser_on_b);
        break;

      case P_LASER_L:
        if (l & D_OUT_L)
          DrawMap(9 + x * 4, 1 + y * 4, map_laser_on_l);
        break;

      case P_MIRROR_TARGET_OPT_BR:
      case P_MIRROR_TARGET_REQ_BR:
        {
          bool mirror_on = (((l & D_IN_B) && (l & D_OUT_R)) ||
                            ((l & D_IN_R) && (l & D_OUT_B)));
          bool target_on = (l & D_IN_L);
          if (mirror_on && target_on)
            DrawMap(9 + x * 4, 1 + y * 4, map_mirror_on_target_on_br);
          else if (mirror_on) {
            if ((board[y][x] & 0x1F) == P_MIRROR_TARGET_OPT_BR)
              DrawMap(9 + x * 4, 1 + y * 4, map_mirror_on_target_opt_br);
            else
              DrawMap(9 + x * 4, 1 + y * 4, map_mirror_on_target_req_br);
          } else if (target_on)
            DrawMap(9 + x * 4, 1 + y * 4, map_mirror_target_on_br);
        }
        break;

      case P_MIRROR_TARGET_OPT_BL:
      case P_MIRROR_TARGET_REQ_BL:
        {
          bool mirror_on = (((l & D_IN_B) && (l & D_OUT_L)) ||
                            ((l & D_IN_L) && (l & D_OUT_B)));
          bool target_on = (l & D_IN_T);
          if (mirror_on && target_on)
            DrawMap(9 + x * 4, 1 + y * 4, map_mirror_on_target_on_bl);
          else if (mirror_on) {
            if ((board[y][x] & 0x1F) == P_MIRROR_TARGET_OPT_BL)
              DrawMap(9 + x * 4, 1 + y * 4, map_mirror_on_target_opt_bl);
            else
              DrawMap(9 + x * 4, 1 + y * 4, map_mirror_on_target_req_bl);
          } else if (target_on)
            DrawMap(9 + x * 4, 1 + y * 4, map_mirror_target_on_bl);
        }
        break;
        
      case P_MIRROR_TARGET_OPT_TL:
      case P_MIRROR_TARGET_REQ_TL:
        {
          bool mirror_on = (((l & D_IN_T) && (l & D_OUT_L)) ||
                            ((l & D_IN_L) && (l & D_OUT_T)));
          bool target_on = (l & D_IN_R);
          if (mirror_on && target_on)
            DrawMap(9 + x * 4, 1 + y * 4, map_mirror_on_target_on_tl);
          else if (mirror_on) {
            if ((board[y][x] & 0x1F) == P_MIRROR_TARGET_OPT_TL)
              DrawMap(9 + x * 4, 1 + y * 4, map_mirror_on_target_opt_tl);
            else
              DrawMap(9 + x * 4, 1 + y * 4, map_mirror_on_target_req_tl);
          } else if (target_on)
            DrawMap(9 + x * 4, 1 + y * 4, map_mirror_target_on_tl);
        }
        break;

      case P_MIRROR_TARGET_OPT_TR:
      case P_MIRROR_TARGET_REQ_TR:
        {
          bool mirror_on = (((l & D_IN_T) && (l & D_OUT_R)) ||
                            ((l & D_IN_R) && (l & D_OUT_T)));
          bool target_on = (l & D_IN_B);
          if (mirror_on && target_on)
            DrawMap(9 + x * 4, 1 + y * 4, map_mirror_on_target_on_tr);
          else if (mirror_on) {
            if ((board[y][x] & 0x1F) == P_MIRROR_TARGET_OPT_TR)
              DrawMap(9 + x * 4, 1 + y * 4, map_mirror_on_target_opt_tr);
            else
              DrawMap(9 + x * 4, 1 + y * 4, map_mirror_on_target_req_tr);
          } else if (target_on)
            DrawMap(9 + x * 4, 1 + y * 4, map_mirror_target_on_tr);
        }
        break;

      case P_SPLIT_TRBL:
        {
          bool in_l = (l & D_IN_L) && (l & D_OUT_R) && (l & D_OUT_T);
          bool in_t = (l & D_IN_T) && (l & D_OUT_B) && (l & D_OUT_L);
          bool in_r = (l & D_IN_R) && (l & D_OUT_L) && (l & D_OUT_B);
          bool in_b = (l & D_IN_B) && (l & D_OUT_T) && (l & D_OUT_R);
          if (in_l && !in_t && !in_r && !in_b)
            DrawMap(9 + x * 4, 1 + y * 4, map_split_trbl_on_l);
          else if (in_t && !in_l && !in_r && !in_b)
            DrawMap(9 + x * 4, 1 + y * 4, map_split_trbl_on_t);
          else if (in_r && !in_l && !in_t && !in_b)
            DrawMap(9 + x * 4, 1 + y * 4, map_split_trbl_on_r);
          else if (in_b && !in_l && !in_t && !in_r)
            DrawMap(9 + x * 4, 1 + y * 4, map_split_trbl_on_b);
          else if (in_l || in_t || in_r || in_b)
            DrawMap(9 + x * 4, 1 + y * 4, map_split_trbl_on_a);
        }
        break;

      case P_SPLIT_TLBR:
        {
          bool in_l = (l & D_IN_L) && (l & D_OUT_R) && (l & D_OUT_B);
          bool in_t = (l & D_IN_T) && (l & D_OUT_B) && (l & D_OUT_R);
          bool in_r = (l & D_IN_R) && (l & D_OUT_L) && (l & D_OUT_T);
          bool in_b = (l & D_IN_B) && (l & D_OUT_T) && (l & D_OUT_L);
          if (in_l && !in_t && !in_r && !in_b)
            DrawMap(9 + x * 4, 1 + y * 4, map_split_tlbr_on_l);
          else if (in_t && !in_l && !in_r && !in_b)
            DrawMap(9 + x * 4, 1 + y * 4, map_split_tlbr_on_t);
          else if (in_r && !in_l && !in_t && !in_b)
            DrawMap(9 + x * 4, 1 + y * 4, map_split_tlbr_on_r);
          else if (in_b && !in_l && !in_t && !in_r)
            DrawMap(9 + x * 4, 1 + y * 4, map_split_tlbr_on_b);
          else if (in_l || in_t || in_r || in_b)
            DrawMap(9 + x * 4, 1 + y * 4, map_split_tlbr_on_a);
        }
        break;

      case P_DBL_MIRROR_TRBL:
        {
          bool mirror_tl = (((l & D_IN_T) && (l & D_OUT_L)) ||
                            ((l & D_IN_L) && (l & D_OUT_T)));
          bool mirror_br = (((l & D_IN_B) && (l & D_OUT_R)) ||
                            ((l & D_IN_R) && (l & D_OUT_B)));
          if (mirror_tl && mirror_br)
            DrawMap(9 + x * 4, 1 + y * 4, map_dbl_mirror_trbl_on_a);
          else if (mirror_tl)
            DrawMap(9 + x * 4, 1 + y * 4, map_dbl_mirror_trbl_on_tl);
          else if (mirror_br)
            DrawMap(9 + x * 4, 1 + y * 4, map_dbl_mirror_trbl_on_br);
        }
        break;

      case P_DBL_MIRROR_TLBR:
        {
          bool mirror_tr = (((l & D_IN_T) && (l & D_OUT_R)) ||
                            ((l & D_IN_R) && (l & D_OUT_T)));
          bool mirror_bl = (((l & D_IN_B) && (l & D_OUT_L)) ||
                            ((l & D_IN_L) && (l & D_OUT_B)));
          if (mirror_tr && mirror_bl)
            DrawMap(9 + x * 4, 1 + y * 4, map_dbl_mirror_tlbr_on_a);
          else if (mirror_tr)
            DrawMap(9 + x * 4, 1 + y * 4, map_dbl_mirror_tlbr_on_tr);
          else if (mirror_bl)
            DrawMap(9 + x * 4, 1 + y * 4, map_dbl_mirror_tlbr_on_bl);
        }
        break;

      case P_CHECKPOINT_TCBC:
        if (((l & D_IN_L) && (l & D_OUT_R)) || ((l & D_IN_R) && (l & D_OUT_L)))
          DrawMap(9 + x * 4, 1 + y * 4, map_checkpoint_on_tcbc);
        break;

      case P_CHECKPOINT_LCRC:
        if (((l & D_IN_T) && (l & D_OUT_B)) || ((l & D_IN_B) && (l & D_OUT_T)))
          DrawMap(9 + x * 4, 1 + y * 4, map_checkpoint_on_lcrc);
        break;  
      }
    }

  // Fill in the gaps between squares with lasers
  for (uint8_t y = 0; y < 5; ++y)
    for (uint8_t x = 0; x < 4; ++x)
      if ((laser[y][x] & D_OUT_R) || (laser[y][x + 1] & D_OUT_L))
        DrawMap(12 + x * 4, 2 + y * 4, map_gap_h);
  for (uint8_t y = 0; y < 4; ++y)
    for (uint8_t x = 0; x < 5; ++x)
      if ((laser[y][x] & D_OUT_B) || (laser[y + 1][x] & D_OUT_T))
        DrawMap(10 + x * 4, 4 + y * 4, map_gap_v);
}

void EraseLaser(void)
{
  for (uint8_t y = 0; y < 5; ++y)
    for (uint8_t x = 0; x < 5; ++x)
      DrawMap(9 + x * 4, 1 + y * 4, MapName(board[y][x] & 0x1F));
      
  // Erase any lasers between squares
  for (uint8_t y = 0; y < 5; ++y)
    for (uint8_t x = 0; x < 4; ++x)
      if ((laser[y][x] & D_OUT_R) || (laser[y][x + 1] & D_OUT_L))
        SetTile(12 + x * 4, 2 + y * 4, TILE_BACKGROUND);
  for (uint8_t y = 0; y < 4; ++y)
    for (uint8_t x = 0; x < 5; ++x)
      if ((laser[y][x] & D_OUT_B) || (laser[y + 1][x] & D_OUT_T))
        SetTile(10 + x * 4, 4 + y * 4, TILE_BACKGROUND);
  /* DrawMap(7, 5, map_laser_source_off); */
}

const int8_t hitMap[] PROGMEM = {
  0, 0, 0, -1,
  1, 1, 1, -1,
  2, 2, 2, -1,
  3, 3, 3, -1,
  4, 4, 4,
};

const uint8_t rotateClockwise[] PROGMEM = {
  P_BLANK, 
  P_LASER_R, 
  P_LASER_B,
  P_LASER_L,
  P_LASER_T,
  P_LASER_U,
  P_MIRROR_TARGET_OPT_BL,
  P_MIRROR_TARGET_OPT_TL,
  P_MIRROR_TARGET_OPT_TR,
  P_MIRROR_TARGET_OPT_BR,
  P_MIRROR_TARGET_OPT_U,
  P_MIRROR_TARGET_REQ_BL,
  P_MIRROR_TARGET_REQ_TL,
  P_MIRROR_TARGET_REQ_TR,
  P_MIRROR_TARGET_REQ_BR,
  P_MIRROR_TARGET_REQ_U,
  P_SPLIT_TLBR,
  P_SPLIT_TRBL,
  P_SPLIT_U,
  P_DBL_MIRROR_TLBR,
  P_DBL_MIRROR_TRBL,
  P_DBL_MIRROR_U,
  P_CHECKPOINT_LCRC,
  P_CHECKPOINT_TCBC,
  P_CHECKPOINT_U,
  P_CELL_BLOCKER,
};

const uint8_t rotateCounterClockwise[] PROGMEM = {
  P_BLANK,
  P_LASER_L,
  P_LASER_T,
  P_LASER_R,
  P_LASER_B,
  P_LASER_U,
  P_MIRROR_TARGET_OPT_TR,
  P_MIRROR_TARGET_OPT_BR,
  P_MIRROR_TARGET_OPT_BL,
  P_MIRROR_TARGET_OPT_TL,
  P_MIRROR_TARGET_OPT_U,
  P_MIRROR_TARGET_REQ_TR,
  P_MIRROR_TARGET_REQ_BR,
  P_MIRROR_TARGET_REQ_BL,
  P_MIRROR_TARGET_REQ_TL,
  P_MIRROR_TARGET_REQ_U,
  P_SPLIT_TLBR,
  P_SPLIT_TRBL,
  P_SPLIT_U,
  P_DBL_MIRROR_TLBR,
  P_DBL_MIRROR_TRBL,
  P_DBL_MIRROR_U,
  P_CHECKPOINT_LCRC,
  P_CHECKPOINT_TCBC,
  P_CHECKPOINT_U,
  P_CELL_BLOCKER,
};

int8_t old_piece = -1;
int8_t old_x = -1;
int8_t old_y = -1; // if this is 5, then it refers to hand

void TryRotation(const uint8_t* rotation_lut)
{
  if (old_piece == -1) { // nothing being dragged and dropped
    uint8_t tx = sprites[MAX_SPRITES - 1].x / TILE_WIDTH;
    uint8_t ty = sprites[MAX_SPRITES - 1].y / TILE_HEIGHT;
    if ((ty >= 1) && (ty <= 19) && (tx >= 9) && (tx <= 27)) { // from grid
      int8_t x = pgm_read_byte(&hitMap[tx - 9]);
      int8_t y = pgm_read_byte(&hitMap[ty - 1]);
      if ((x >= 0) && (y >= 0) && !(board[y][x] & 0x80)) { // respect lock bit
        // Save the rotate bit, if set
        uint8_t flags = board[y][x] & 0xE0;
        board[y][x] = flags | pgm_read_byte(&rotation_lut[board[y][x] & 0x1F]);
        DrawMap(9 + x * 4, 1 + y * 4, MapName(board[y][x] & 0x1F));
        TriggerNote(4, 3, 23, 255);
      }
    } else if ((ty >= 23) && (ty <= 25) && (tx >= 9) && (tx <= 27)) {
      int8_t x = pgm_read_byte(&hitMap[tx - 9]);
      if (x >= 0) {
        hand[x] = pgm_read_byte(&rotation_lut[hand[x]]);
        DrawMap(9 + x * 4, 23, MapName(hand[x]));
        TriggerNote(4, 3, 23, 255);
      }
    }
  } else {
    old_piece = pgm_read_byte(&rotation_lut[old_piece]);
    MapSprite2(MAX_SPRITES - 10, MapName(old_piece), SPRITE_BANK1);
    MoveSprite(MAX_SPRITES - 10, sprites[MAX_SPRITES - 1].x - 8, sprites[MAX_SPRITES - 1].y - 8, 3, 3);
    TriggerNote(4, 3, 23, 255);
  }
}

const uint8_t myramfont[] PROGMEM = {
  0x1c, 0x36, 0x63, 0x63, 0x7f, 0x63, 0x63, 0x00, 
  0x3f, 0x63, 0x63, 0x3f, 0x63, 0x63, 0x3f, 0x00, 
  0x3c, 0x66, 0x03, 0x03, 0x03, 0x66, 0x3c, 0x00, 
  0x1f, 0x33, 0x63, 0x63, 0x63, 0x33, 0x1f, 0x00, 
  0x7f, 0x03, 0x03, 0x3f, 0x03, 0x03, 0x7f, 0x00, 
  0x7f, 0x03, 0x03, 0x3f, 0x03, 0x03, 0x03, 0x00, 
  0x7c, 0x06, 0x03, 0x73, 0x63, 0x66, 0x7c, 0x00, 
  0x63, 0x63, 0x63, 0x7f, 0x63, 0x63, 0x63, 0x00, 
  0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x7e, 0x00, 
  0x78, 0x30, 0x30, 0x30, 0x30, 0x31, 0x1e, 0x00, 
  0x63, 0x33, 0x1b, 0x0f, 0x1f, 0x3b, 0x73, 0x00, 
  0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x7e, 0x00, 
  0x63, 0x77, 0x7f, 0x7f, 0x6b, 0x63, 0x63, 0x00, 
  0x63, 0x67, 0x6f, 0x7f, 0x7b, 0x73, 0x63, 0x00, 
  0x3e, 0x63, 0x63, 0x63, 0x63, 0x63, 0x3e, 0x00, 
  0x3f, 0x63, 0x63, 0x63, 0x3f, 0x03, 0x03, 0x00, 
  0x3e, 0x63, 0x63, 0x63, 0x7b, 0x33, 0x5e, 0x00, 
  0x3f, 0x63, 0x63, 0x73, 0x1f, 0x3b, 0x73, 0x00, 
  0x1e, 0x33, 0x03, 0x3e, 0x60, 0x63, 0x3e, 0x00, 
  0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 
  0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x3e, 0x00, 
  0x63, 0x63, 0x63, 0x77, 0x3e, 0x1c, 0x08, 0x00, 
  0x63, 0x63, 0x6b, 0x7f, 0x7f, 0x77, 0x63, 0x00, 
  0x63, 0x77, 0x3e, 0x1c, 0x3e, 0x77, 0x63, 0x00, 
  0x66, 0x66, 0x66, 0x3c, 0x18, 0x18, 0x18, 0x00, 
  0x7f, 0x70, 0x38, 0x1c, 0x0e, 0x07, 0x7f, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x0c, 0x0c, 0x06, 0x00, 
  0x00, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x00, 
};

void RamFont_Load(const uint8_t* ramfont, uint8_t len, uint8_t fg_color, uint8_t bg_color)
{
  SetUserRamTilesCount(len);
  for (uint8_t tile = 0; tile < len; ++tile) {
    uint8_t* ramTile = GetUserRamTile(tile);
    for (uint8_t row = 0; row < 8; ++row) {
      uint8_t data = (uint8_t)pgm_read_byte(&ramfont[tile * 8 + row]);
      for (uint8_t bit = 0; bit < 8; ++bit)
        if (data & (1 << bit))
          ramTile[row * 8 + bit] = fg_color;
        else
          ramTile[row * 8 + bit] = bg_color;
    }
  }
}

const char pgm_inventor[] PROGMEM = "INVENTOR  LUKE HOOPER";
const char pgm_puzzles1[] PROGMEM = "PUZZLES  WEI\\HUANG[";
const char pgm_puzzles2[] PROGMEM = "TYLER SOMER[";
const char pgm_puzzles3[] PROGMEM = "LUKE HOOPER[";
const char pgm_puzzles4[] PROGMEM = "TANYA THOMPSON";
const char pgm_tada[] PROGMEM     = "TADA SOUND  MIKE KOENIG";
const char pgm_uzebox[] PROGMEM   = "UZEBOX GAME  MATT PANDINA";
const char pgm_play[] PROGMEM     = "PLAY";
const char pgm_controls[] PROGMEM = "CONTROLS";
const char pgm_tokens[] PROGMEM   = "TOKENS";

const char pgm_instructions1[] PROGMEM = "THE LASER MUST TOUCH EVERY";
const char pgm_instructions2[] PROGMEM = "TOKEN AT LEAST ONCE";
const char pgm_instructions3[] PROGMEM = "EXCLUDING THE CELL BLOCKER";

const char pgm_instructions4[] PROGMEM = "D\\PAD  MOVE CURSOR";
const char pgm_instructions5[] PROGMEM = "A  CLICK[ DRAG\\AND\\DROP";
const char pgm_instructions6[] PROGMEM = "Y  ACTIVATE LASER";
const char pgm_instructions7[] PROGMEM = "L[ B  ROTATE TOKEN LEFT";
const char pgm_instructions8[] PROGMEM = "R[ X  ROTATE TOKEN RIGHT";
const char pgm_instructions9[] PROGMEM = "SELECT  TOGGLE MUSIC";

const char pgm_laser[] PROGMEM         = "LASER";
const char pgm_target[] PROGMEM        = "TARGET";
const char pgm_comma[] PROGMEM         = "[";
const char pgm_mirror[] PROGMEM        = "MIRROR";
const char pgm_beam_splitter[] PROGMEM = "BEAM SPLITTER";
const char pgm_double_mirror[] PROGMEM = "DOUBLE MIRROR";
const char pgm_checkpoint[] PROGMEM    = "CHECKPOINT";
const char pgm_cell_blocker[] PROGMEM  = "CELL BLOCKER";
const char pgm_must[] PROGMEM          = "MUST";
const char pgm_can[] PROGMEM           = "CAN";
const char pgm_be_used_as_a[] PROGMEM  = "BE USED AS A";
const char pgm_press_a1[] PROGMEM      = "PRESS A TO CONTINUE";
const char pgm_this_does_not[] PROGMEM = "THIS DOES NOT BLOCK LASER";

void RamFont_Print(uint8_t x, uint8_t y, const char *message, uint8_t len)
{
  for (uint8_t i = 0; i < len; ++i) {
    int8_t tileno = (int8_t)pgm_read_byte(&message[i]) - 'A';
    if (tileno >= 0)
      SetRamTile(x + i, y, tileno);
  }
}

int main()
{
  BUTTON_INFO buttons;
  memset(&buttons, 0, sizeof(BUTTON_INFO));
  InitMusicPlayer(patches);

  /* INTRO AND TITLE SCREEN */ {
    ClearVram();  
    RamFont_Load(myramfont, sizeof(myramfont) / 8, 0x00, 0x00);
    RamFont_Print(5, 5, pgm_inventor, sizeof(pgm_inventor));
    RamFont_Print(6, 9, pgm_puzzles1, sizeof(pgm_puzzles1));
    RamFont_Print(15, 11, pgm_puzzles2, sizeof(pgm_puzzles2));
    RamFont_Print(15, 13, pgm_puzzles3, sizeof(pgm_puzzles3));
    RamFont_Print(15, 15, pgm_puzzles4, sizeof(pgm_puzzles4));
    RamFont_Print(3, 19, pgm_tada, sizeof(pgm_tada));
    RamFont_Print(2, 23, pgm_uzebox, sizeof(pgm_uzebox));
  
    uint8_t col = 0;
    for (;;) {
      WaitVsync(5);
      ++col;
      RamFont_Load(myramfont, sizeof(myramfont) / 8, col, 0x00);
      if (col == 7) {
        WaitVsync(235);
        break;
      }
    }
    for (;;) {
      WaitVsync(5);
      --col;
      RamFont_Load(myramfont, sizeof(myramfont) / 8, col, 0x00);
      if (col == 0) {
        WaitVsync(60);
        break;
      }
    }

  title_screen:
    ClearVram();
    SetTileTable(titlescreen);
    RamFont_Load(myramfont, sizeof(myramfont) / 8, 0x00, 0xad);
    DrawMap(6, 7, map_title_big);
    RamFont_Print(13, 15, pgm_play, sizeof(pgm_play));
    RamFont_Print(11, 17, pgm_controls, sizeof(pgm_controls));
    RamFont_Print(12, 19, pgm_tokens, sizeof(pgm_tokens));

    uint8_t selection = 0;
    for (;;) {
      WaitVsync(1);
      
      switch (selection) {
      case 0:
        // Erase laser bar for CONTROLS
        for (uint8_t i = 0; i < SCREEN_TILES_H; ++i)
          if (i < 10 || i > 19)
            SetTile(i, 17, TILE_BACKGROUND);
        // Erase laser bar for TOKENS
        for (uint8_t i = 0; i < SCREEN_TILES_H; ++i)
          if (i < 11 || i > 18)
            SetTile(i, 19, TILE_BACKGROUND);      
        // Draw laser bar for PLAY
        for (uint8_t i = 0; i < SCREEN_TILES_H; ++i)
          if (i < 12 || i > 17)
            SetTile(i, 15, TILE_TITLE_LASER);
        break;
      case 1:
        // Erase laser bar for PLAY
        for (uint8_t i = 0; i < SCREEN_TILES_H; ++i)
          if (i < 12 || i > 17)
            SetTile(i, 15, TILE_BACKGROUND);
        // Erase laser bar for TOKENS
        for (uint8_t i = 0; i < SCREEN_TILES_H; ++i)
          if (i < 11 || i > 18)
            SetTile(i, 19, TILE_BACKGROUND);      
        // Draw laser bar for CONTROLS
        for (uint8_t i = 0; i < SCREEN_TILES_H; ++i)
          if (i < 10 || i > 19)
            SetTile(i, 17, TILE_TITLE_LASER);
        break;
      case 2:
        // Erase laser bar for PLAY
        for (uint8_t i = 0; i < SCREEN_TILES_H; ++i)
          if (i < 12 || i > 17)
            SetTile(i, 15, TILE_BACKGROUND);
        // Erase laser bar for CONTROLS
        for (uint8_t i = 0; i < SCREEN_TILES_H; ++i)
          if (i < 10 || i > 19)
            SetTile(i, 17, TILE_BACKGROUND);
        // Draw laser bar for TOKENS
        for (uint8_t i = 0; i < SCREEN_TILES_H; ++i)
          if (i < 11 || i > 18)
            SetTile(i, 19, TILE_TITLE_LASER);     
        break;
      }
        
      // Read the current state of the player's controller
      buttons.prev = buttons.held;
      buttons.held = ReadJoypad(0);
      buttons.pressed = buttons.held & (buttons.held ^ buttons.prev);
      buttons.released = buttons.prev & (buttons.held ^ buttons.prev);

      if (buttons.pressed & BTN_DOWN) {
        TriggerNote(4, 3, 23, 255);
        if (++selection == 3)
          selection = 0;
      }
      if (buttons.pressed & BTN_UP) {
        TriggerNote(4, 4, 23, 255);
        if (--selection == 255)
          selection = 2;
      }
        
      if ((buttons.pressed & BTN_A) || (buttons.pressed & BTN_START)) {
        if (selection == 0) {
          break;
        } else if (selection == 1) {
          TriggerNote(4, 3, 23, 255);
          ClearVram();
          RamFont_Print(2, 2, pgm_instructions1, sizeof(pgm_instructions1));
          RamFont_Print(5, 4, pgm_instructions2, sizeof(pgm_instructions2));
          RamFont_Print(2, 6, pgm_instructions3, sizeof(pgm_instructions3));

          RamFont_Print(2, 10, pgm_instructions4, sizeof(pgm_instructions4));
          RamFont_Print(6, 13, pgm_instructions5, sizeof(pgm_instructions5));
          RamFont_Print(6, 16, pgm_instructions6, sizeof(pgm_instructions6));
          RamFont_Print(3, 19, pgm_instructions7, sizeof(pgm_instructions7));
          RamFont_Print(3, 22, pgm_instructions8, sizeof(pgm_instructions8));
          RamFont_Print(1, 25, pgm_instructions9, sizeof(pgm_instructions9));

          for (;;) {
            WaitVsync(1);
            
            // Read the current state of the player's controller
            buttons.prev = buttons.held;
            buttons.held = ReadJoypad(0);
            buttons.pressed = buttons.held & (buttons.held ^ buttons.prev);
            buttons.released = buttons.prev & (buttons.held ^ buttons.prev);

            if ((buttons.pressed & BTN_A) || (buttons.pressed & BTN_START)) {
              TriggerNote(4, 4, 23, 255);
              goto title_screen;
            }
          }
        } else if (selection == 2) {
          TriggerNote(4, 3, 23, 255);
          ClearVram();
          SetTileTable(tileset);
          
          uint8_t piece = 0;
          for (;;) {
            WaitVsync(1);

            RamFont_Print(10, 26, pgm_press_a1, sizeof(pgm_press_a1));

            switch (piece) {
            case 0:
              RamFont_Print(2, 2, pgm_laser, sizeof(pgm_laser));
              DrawMap(13, 12, map_laser_b);
              break;
            case 1:
              RamFont_Print(2, 2, pgm_target, sizeof(pgm_target));
              RamFont_Print(8, 2, pgm_comma, sizeof(pgm_comma));
              RamFont_Print(10, 2, pgm_mirror, sizeof(pgm_mirror));
              DrawMap(13, 12, map_mirror_target_req_tr);
              RamFont_Print(2, 20, pgm_must, sizeof(pgm_must));
              RamFont_Print(7, 20, pgm_be_used_as_a, sizeof(pgm_be_used_as_a));
              RamFont_Print(20, 20, pgm_target, sizeof(pgm_target));
              RamFont_Print(26, 20, pgm_comma, sizeof(pgm_comma));
              RamFont_Print(2, 22, pgm_can, sizeof(pgm_can));
              RamFont_Print(6, 22, pgm_be_used_as_a, sizeof(pgm_be_used_as_a));
              RamFont_Print(19, 22, pgm_mirror, sizeof(pgm_mirror));
              break;
            case 2:
              RamFont_Print(2, 2, pgm_target, sizeof(pgm_target));
              RamFont_Print(8, 2, pgm_comma, sizeof(pgm_comma));
              RamFont_Print(10, 2, pgm_mirror, sizeof(pgm_mirror));
              DrawMap(13, 12, map_mirror_target_opt_tr);
              RamFont_Print(2, 20, pgm_can, sizeof(pgm_can));
              RamFont_Print(6, 20, pgm_be_used_as_a, sizeof(pgm_be_used_as_a));
              RamFont_Print(19, 20, pgm_target, sizeof(pgm_target));
              RamFont_Print(25, 20, pgm_comma, sizeof(pgm_comma));
              RamFont_Print(2, 22, pgm_can, sizeof(pgm_can));
              RamFont_Print(6, 22, pgm_be_used_as_a, sizeof(pgm_be_used_as_a));
              RamFont_Print(19, 22, pgm_mirror, sizeof(pgm_mirror));
              break;
            case 3:
              RamFont_Print(2, 2, pgm_beam_splitter, sizeof(pgm_beam_splitter));
              DrawMap(13, 12, map_split_tlbr);
              break;
            case 4:
              RamFont_Print(2, 2, pgm_double_mirror, sizeof(pgm_double_mirror));
              DrawMap(13, 12, map_dbl_mirror_tlbr);
              break;
            case 5:
              RamFont_Print(2, 2, pgm_checkpoint, sizeof(pgm_checkpoint));
              DrawMap(13, 12, map_checkpoint_tcbc);
              break;
            case 6:
              RamFont_Print(2, 2, pgm_cell_blocker, sizeof(pgm_cell_blocker));
              DrawMap(13, 12, map_cell_blocker);
	      RamFont_Print(2, 20, pgm_this_does_not, sizeof(pgm_this_does_not));
              break;
            }
            // Read the current state of the player's controller
            buttons.prev = buttons.held;
            buttons.held = ReadJoypad(0);
            buttons.pressed = buttons.held & (buttons.held ^ buttons.prev);
            buttons.released = buttons.prev & (buttons.held ^ buttons.prev);

            if (buttons.pressed & BTN_A) {
              ++piece;
              if (piece > 6) {
                TriggerNote(4, 4, 23, 255);
                goto title_screen;
              }
              TriggerNote(4, 3, 23, 255);

              ClearVram();
            }
          }
        }
      }
    }
  
    SetUserRamTilesCount(0);
  }
  
  // END TITLE SCREEN
  
  memset(&buttons, 0, sizeof(BUTTON_INFO));

  ClearVram();
  SetTileTable(tileset);
  SetSpritesTileBank(0, mysprites);
  SetSpritesTileBank(1, tileset);

  StartSong(midisong);

  uint8_t currentLevel = 1;
  LoadLevel(currentLevel, false);
  
  sprites[MAX_SPRITES - 1].tileIndex = 2;
  sprites[MAX_SPRITES - 1].x = 7 * TILE_WIDTH;
  sprites[MAX_SPRITES - 1].y = 24 * TILE_HEIGHT;
  uint8_t saved_cursor_x = 0;

  bool flashNext = false;
  uint8_t flashCounter = 0;
  
  for (;;) {
    WaitVsync(1);
 
    // Read the current state of the player's controller
    buttons.prev = buttons.held;
    buttons.held = ReadJoypad(0);
    buttons.pressed = buttons.held & (buttons.held ^ buttons.prev);
    buttons.released = buttons.prev & (buttons.held ^ buttons.prev);

    // This "solution view" is for debug purposes only!
    /* if (buttons.pressed & BTN_START) */
    /*   LoadLevel(currentLevel, true); */
    /* if (buttons.released & BTN_START) */
    /*   LoadLevel(currentLevel, false); */

    if (flashNext) {
      if (flashCounter == 0)
        DrawMap(PREV_NEXT_X + 2, PREV_NEXT_Y, map_next_red);
      else if (flashCounter == 19)
        DrawMap(PREV_NEXT_X + 2, PREV_NEXT_Y, map_next);
      else if (flashCounter == 39)
        flashCounter = 255;
      ++flashCounter;
    }

    // Allow song to be paused/unpaused
    if (buttons.pressed & BTN_SELECT) {
      if (IsSongPlaying())
        StopSong();
      else
        ResumeSong();
    }
    
    if (buttons.pressed & BTN_Y) {
      // Hide the cursor when the laser is on
      saved_cursor_x = sprites[MAX_SPRITES - 1].x;

      if (!(buttons.held & BTN_A)) { // Don't turn the laser on if you are dragging and dropping
        sprites[MAX_SPRITES - 1].x = OFF_SCREEN;
        memset(laser, 0, sizeof(laser));
        for (uint8_t i = 0; i < 100; ++i)
          SimulatePhoton();
      
        DrawLaser();
        // Check to see if the puzzle has been solved
        const uint16_t offset = (currentLevel - 1) * LEVEL_SIZE + 25;
        bool win = true;
        for (uint8_t y = 0; y < 5; ++y)
          for (uint8_t x = 0; x < 5; ++x) {
            uint8_t piece = (uint8_t)pgm_read_byte(&levelData[offset + y * 5 + x]);
            if ((board[y][x] & 0x1F) != piece)
              win = false;
          }
        if (win) {
          TriggerNote(4, 5, 15, 255);
          flashNext = true;
          WaitVsync(150);
        } else {
          WaitVsync(10);
        }
      }
    } else if (buttons.released & BTN_Y) {
      EraseLaser();
      // Restore the cursor when the laser is off
      sprites[MAX_SPRITES - 1].x = saved_cursor_x;
    }
        
#define CUR_SPEED 2
#define X_LB (1 * TILE_WIDTH)
#define X_UB ((SCREEN_TILES_H - 2) * TILE_WIDTH)
#define Y_LB (1 * TILE_HEIGHT)
#define Y_UB ((SCREEN_TILES_V - 2) * TILE_HEIGHT)
    
    if (!(buttons.held & BTN_Y)) { // Don't allow the hidden cursor to be moved if the laser is on
    
      // Move the "mouse cursor"
      if (buttons.held & BTN_RIGHT) {
        uint8_t x = sprites[MAX_SPRITES - 1].x;
        x += CUR_SPEED;
        if (x > X_UB)
          x = X_UB;
        sprites[MAX_SPRITES - 1].x = x;
      } else if (buttons.held & BTN_LEFT) {
        uint8_t x = sprites[MAX_SPRITES - 1].x;
        x -= CUR_SPEED;
        if (x < X_LB)
          x = X_LB;
        sprites[MAX_SPRITES - 1].x = x;
      }
      if (buttons.held & BTN_UP) {
        uint8_t y = sprites[MAX_SPRITES - 1].y;
        y -= CUR_SPEED;
        if (y < Y_LB)
          y = Y_LB;
        sprites[MAX_SPRITES - 1].y = y;
      } else if (buttons.held & BTN_DOWN) {
        uint8_t y = sprites[MAX_SPRITES - 1].y;
        y += CUR_SPEED;
        if (y > Y_UB)
          y = Y_UB;
        sprites[MAX_SPRITES - 1].y = y;
      }
      // Dragging
      if (old_piece != -1) {
        MoveSprite(MAX_SPRITES - 10, sprites[MAX_SPRITES - 1].x - 8, sprites[MAX_SPRITES - 1].y - 8, 3, 3);
        // Highlight blank squares when we're hovering over them
        uint8_t tx = sprites[MAX_SPRITES - 1].x / TILE_WIDTH;
        uint8_t ty = sprites[MAX_SPRITES - 1].y / TILE_HEIGHT;
        if ((ty >= 1) && (ty <= 19) && (tx >= 9) && (tx <= 27)) { // from grid
          int8_t x = pgm_read_byte(&hitMap[tx - 9]);
          int8_t y = pgm_read_byte(&hitMap[ty - 1]);
          if ((x >= 0) && (y >= 0)) {
            if ((board[y][x] & 0x1F) == P_BLANK)
              DrawMap(9 + x * 4, 1 + y * 4, map_blank_highlight);
          } else {
            for (uint8_t y = 0; y < 5; ++y)
              for (uint8_t x = 0; x < 5; ++x)
                if ((board[y][x] & 0x1F) == P_BLANK)
                  DrawMap(9 + x * 4, 1 + y * 4, map_blank);
          }
        } else if ((ty >= 23) && (ty <= 25) && (tx >= 9) && (tx <= 27)) { // from hand
          int8_t x = pgm_read_byte(&hitMap[tx - 9]);
          if ((x >= 0)) {
            if ((hand[x] & 0x1F) == P_BLANK)
              DrawMap(9 + x * 4, 23, map_blank_highlight);
          } else {
            for (uint8_t x = 0; x < 5; ++x)
              if ((hand[x] & 0x1F) == P_BLANK)
                DrawMap(9 + x * 4, 23, map_blank);
          }
        } else {
          for (uint8_t y = 0; y < 5; ++y)
            for (uint8_t x = 0; x < 5; ++x)
              if ((board[y][x] & 0x1F) == P_BLANK)
                DrawMap(9 + x * 4, 1 + y * 4, map_blank);
          for (uint8_t x = 0; x < 5; ++x)
            if ((hand[x] & 0x1F) == P_BLANK)
              DrawMap(9 + x * 4, 23, map_blank);
        }
        
      }
    }

    // Process rotations
    if (!(buttons.held & BTN_Y)) { // Don't process rotations if the laser is on
      if ((buttons.pressed & BTN_X) || (buttons.pressed & BTN_SR))
        TryRotation(rotateClockwise);
      else if ((buttons.pressed & BTN_B) || (buttons.pressed & BTN_SL))
        TryRotation(rotateCounterClockwise);
    }
    
    // Process any "mouse" clicks
    if (buttons.pressed & BTN_A) {
      uint8_t tx = sprites[MAX_SPRITES - 1].x / TILE_WIDTH;
      uint8_t ty = sprites[MAX_SPRITES - 1].y / TILE_HEIGHT;
      if ((ty >= PREV_NEXT_Y - 1) && (ty <= PREV_NEXT_Y + 1)) {
        if ((tx >= PREV_NEXT_X) && (tx <= PREV_NEXT_X + 1)) {
          if (--currentLevel == 0)
            currentLevel = LEVELS;
          TriggerNote(4, 3, 23, 255);
          flashNext = false;
          flashCounter = 0;
          LoadLevel(currentLevel, false);
        }
        if ((tx >= PREV_NEXT_X + 2) && (tx <= PREV_NEXT_X + 3)) {
          if (++currentLevel == LEVELS + 1)
            currentLevel = 1;
          TriggerNote(4, 3, 23, 255);
          flashNext = false;
          flashCounter = 0;
          LoadLevel(currentLevel, false);
        }       
      }

      // Drag and drop
      if ((ty >= 1) && (ty <= 19) && (tx >= 9) && (tx <= 27)) { // from grid
        int8_t x = pgm_read_byte(&hitMap[tx - 9]);
        int8_t y = pgm_read_byte(&hitMap[ty - 1]);
        if ((x >= 0) && (y >= 0) && !(board[y][x] & 0x80) && !(board[y][x] & 0x40) && (board[y][x] != P_BLANK)) { // respect lock bit
          old_piece = board[y][x];
          old_x = x;
          old_y = y;
          DrawMap(9 + x * 4, 1 + y * 4, map_blank);
          board[y][x] = P_BLANK;
          MapSprite2(MAX_SPRITES - 10, MapName(old_piece), SPRITE_BANK1);
          MoveSprite(MAX_SPRITES - 10, sprites[MAX_SPRITES - 1].x - 8, sprites[MAX_SPRITES - 1].y - 8, 3, 3);
          TriggerNote(4, 3, 23, 255);
        }
      } else if ((ty >= 23) && (ty <= 25) && (tx >= 9) && (tx <= 27)) { // from hand
        int8_t x = pgm_read_byte(&hitMap[tx - 9]);
        if ((x >= 0) && (hand[x] != P_BLANK)) {
          old_piece = hand[x];
          old_x = x;
          old_y = 5; // this piece came from hand
          DrawMap(9 + x * 4, 23, map_blank);
          hand[x] = P_BLANK;
          MapSprite2(MAX_SPRITES - 10, MapName(old_piece), SPRITE_BANK1);
          MoveSprite(MAX_SPRITES - 10, sprites[MAX_SPRITES - 1].x - 8, sprites[MAX_SPRITES - 1].y - 8, 3, 3);
          TriggerNote(4, 3, 23, 255);
        }
      }
      
    } else if (buttons.released & BTN_A) {
      if ((old_piece != -1) && (old_y != -1)) { // valid piece is being held
        uint8_t tx = sprites[MAX_SPRITES - 1].x / TILE_WIDTH;
        uint8_t ty = sprites[MAX_SPRITES - 1].y / TILE_HEIGHT;
        // Figure out where to drop it
        if ((ty >= 1) && (ty <= 19) && (tx >= 9) && (tx <= 27)) { // to grid
          int8_t x = pgm_read_byte(&hitMap[tx - 9]);
          int8_t y = pgm_read_byte(&hitMap[ty - 1]);
          if ((x >= 0) && (y >= 0) && ((board[y][x] & 0x1F) == P_BLANK)) {
            old_x = x;
            old_y = y;
          }
        } else if ((ty >= 23) && (ty <= 25) && (tx >= 9) && (tx <= 27)) { // to hand
          int8_t x = pgm_read_byte(&hitMap[tx - 9]);
          if ((x >= 0) && ((hand[x] & 0x1F) == P_BLANK)) {
            old_x = x;
            old_y = 5; // hand
          }
        }
        
        // Drop it like it's hot
        for (uint8_t i = 0; i < 9; ++i)
          sprites[i + MAX_SPRITES - 10].x = OFF_SCREEN;
        if (old_y == 5) {
          DrawMap(9 + old_x * 4, 23, MapName(old_piece));
          hand[old_x] = old_piece;
        } else {
          DrawMap(9 + old_x * 4, 1 + old_y * 4, MapName(old_piece));
          board[old_y][old_x] = old_piece;
        }
        old_piece = old_x = old_y = -1;
        TriggerNote(4, 4, 23, 255);
      }
    }
    
  }
}
