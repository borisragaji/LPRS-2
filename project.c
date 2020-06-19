
///////////////////////////////////////////////////////////////////////////////
// Headers.

#include <stdint.h>
#include "system.h"
#include <stdio.h>
#include <stdlib.h>

#include "sprites_rgb333.h"
///////////////////////////////////////////////////////////////////////////////
// HW stuff.

#define WAIT_UNITL_0(x) while(x != 0){}
#define WAIT_UNITL_1(x) while(x != 1){}

#define SCREEN_IDX1_W 640
#define SCREEN_IDX1_H 480
#define SCREEN_IDX4_W 320
#define SCREEN_IDX4_H 240
#define SCREEN_RGB333_W 160
#define SCREEN_RGB333_H 120

#define SCREEN_IDX4_W8 (SCREEN_IDX4_W/8)

#define gpu_p32 ((volatile uint32_t*)LPRS2_GPU_BASE)
#define palette_p32 ((volatile uint32_t*)(LPRS2_GPU_BASE+0x1000))
#define unpack_idx1_p32 ((volatile uint32_t*)(LPRS2_GPU_BASE+0x400000))
#define pack_idx1_p32 ((volatile uint32_t*)(LPRS2_GPU_BASE+0x600000))
#define unpack_idx4_p32 ((volatile uint32_t*)(LPRS2_GPU_BASE+0x800000))
#define pack_idx4_p32 ((volatile uint32_t*)(LPRS2_GPU_BASE+0xa00000))
#define unpack_rgb333_p32 ((volatile uint32_t*)(LPRS2_GPU_BASE+0xc00000))
#define joypad_p32 ((volatile uint32_t*)LPRS2_JOYPAD_BASE)

typedef struct {
	unsigned a      : 1;
	unsigned b      : 1;
	unsigned z      : 1;
	unsigned start  : 1;
	unsigned up     : 1;
	unsigned down   : 1;
	unsigned left   : 1;
	unsigned right  : 1;
} bf_joypad;
#define joypad (*((volatile bf_joypad*)LPRS2_JOYPAD_BASE))

typedef struct {
	uint32_t m[SCREEN_IDX1_H][SCREEN_IDX1_W];
} bf_unpack_idx1;
#define unpack_idx1 (*((volatile bf_unpack_idx1*)unpack_idx1_p32))



///////////////////////////////////////////////////////////////////////////////
// Game config.

#define STEP 1

#define TANK_ANIM_DELAY 3

#define ENEMY_ANIM_DELAY 5

///////////////////////////////////////////////////////////////////////////////
// Game data structures.

typedef struct {
	uint16_t x;
	uint16_t y;
} point_t;

typedef enum {
	TANK_STATE1,
	TANK_STATE2,
} tank_anim_states_t;

typedef enum {
	TANK_TURNING_LEFT,
	TANK_TURNING_RIGHT,
	TANK_TURNING_DOWN,
	TANK_TURNING_UP
} tank_anim_smer_t;

typedef struct {
	tank_anim_smer_t smer;
	tank_anim_states_t state;
	uint8_t delay_cnt;
} tank_anim_t;


typedef struct {
	point_t pos;
	tank_anim_t anim;
} tank_t;

typedef enum{
	ENEMY_STATE1,
	ENEMY_STATE2,
	DESTROYED1,
	DESTROYED2,
	DESTROYED3
} enemy_anim_state_t;

typedef enum{
	ENEMY_LEFT,
	ENEMY_RIGHT,		
} enemy_anim_smer_t;

typedef struct{
	enemy_anim_smer_t smer;
	enemy_anim_state_t state;
	uint8_t delay_cnt;
	uint8_t delay_cnt2;
} enemy_anim_t;

typedef struct{
	point_t pos;
	enemy_anim_t anim;
} enemy_t;

// bullet

typedef enum {
	BULLET_LEFT,
	BULLET_RIGHT,
	BULLET_DOWN,
	BULLET_UP
} bullet_anim_smer_t;

typedef struct{
	bullet_anim_smer_t smer;
	uint8_t delay_cnt;
} bullet_anim_t;

typedef struct{
	point_t pos;
	bullet_anim_t anim;
} bullet_t;

//

typedef struct {
	tank_t tank;
	enemy_t enemy;
	bullet_t bullet;
} game_state_t;

void draw_sprite_from_atlas(
	uint16_t src_x,
	uint16_t src_y,
	uint16_t w,
	uint16_t h,
	uint16_t dst_x,
	uint16_t dst_y
) {	
	for(uint16_t y = 0; y < h; y++){
		for(uint16_t x = 0; x < w; x++){
			uint32_t src_idx = 
				(src_y+y)*Tank_Sprite_Map__w +
				(src_x+x);
			uint32_t dst_idx = 
				(dst_y+y)*SCREEN_RGB333_W +
				(dst_x+x);
			uint16_t pixel = Tank_Sprite_Map__p[src_idx];
			if(pixel != 0000){
				unpack_rgb333_p32[dst_idx] = pixel;
			}	
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
// Game code.

int main(void) {
	
	// Setup.
	gpu_p32[0] = 3; // Color bar.
	gpu_p32[0x800] = 0x00ff00ff; // Magenta for HUD.


	// Game state.
	game_state_t gs;
	game_state_t en1;
	game_state_t bul;	

	gs.tank.pos.x = 16;
	gs.tank.pos.y = 96;
	gs.tank.anim.smer = TANK_TURNING_UP;
	gs.tank.anim.state = TANK_STATE1;
	gs.tank.anim.delay_cnt = 0;

	en1.enemy.pos.x = 128;
	en1.enemy.pos.y = 16;
	en1.enemy.anim.smer = ENEMY_RIGHT;
	en1.enemy.anim.state = ENEMY_STATE1;
	en1.enemy.anim.delay_cnt = 0;
	en1.enemy.anim.delay_cnt2 = 0;	
	
	bul.bullet.pos.x = 1000;
	bul.bullet.anim.delay_cnt = 0;

	while(1){
		
		
		/////////////////////////////////////
		// Poll controls.
		double mov_x = 0;
		double mov_y = 0;
		
		if(joypad.right){
			if(gs.tank.pos.x != 128 && (gs.tank.pos.y == 16 || gs.tank.pos.y == 48 || gs.tank.pos.y == 47 || gs.tank.pos.y == 49 || gs.tank.pos.y == 96)) {
				mov_x = +1;
				gs.tank.anim.smer = TANK_TURNING_RIGHT;
			}
		}
		if(joypad.left){
			if(gs.tank.pos.x != 16 && (gs.tank.pos.y == 16 || gs.tank.pos.y == 48 || gs.tank.pos.y == 47 || gs.tank.pos.y == 49 || gs.tank.pos.y == 96)) {
				mov_x = -1;
				gs.tank.anim.smer = TANK_TURNING_LEFT;
			}
		}
		if(joypad.up){
			if(gs.tank.pos.y != 16 && (gs.tank.pos.x == 16 || gs.tank.pos.x == 128 || gs.tank.pos.x == 48 || gs.tank.pos.x == 47 || gs.tank.pos.x == 49)) {
				mov_y = -1;
				gs.tank.anim.smer = TANK_TURNING_UP;
			}
		}
		if(joypad.down){
			if(gs.tank.pos.y != 96 && (gs.tank.pos.x == 16 || gs.tank.pos.x == 128 || gs.tank.pos.x == 48 || gs.tank.pos.x == 47 || gs.tank.pos.x == 49)) {
				mov_y = +1;
				gs.tank.anim.smer = TANK_TURNING_DOWN;
			}
		}
		// shooting
		
		if(joypad.b){
			if(bul.bullet.pos.x == 1000) {
				bul.bullet.pos.x = gs.tank.pos.x;
				bul.bullet.pos.y = gs.tank.pos.y;
				if(gs.tank.anim.smer == TANK_TURNING_RIGHT) {
					bul.bullet.anim.smer = BULLET_RIGHT;
				} else if(gs.tank.anim.smer == TANK_TURNING_LEFT) {
					bul.bullet.anim.smer = BULLET_LEFT;
				} else if(gs.tank.anim.smer == TANK_TURNING_UP) {
					bul.bullet.anim.smer = BULLET_UP;
				} else if(gs.tank.anim.smer == TANK_TURNING_DOWN) {
					bul.bullet.anim.smer = BULLET_DOWN;
				}
			}		
		}
	
		//

		/////////////////////////////////////
		// Gameplay.
		
		gs.tank.pos.x += mov_x*STEP;
		gs.tank.pos.y += mov_y*STEP;
		
		// enemy destroyed
		if((en1.enemy.pos.x == bul.bullet.pos.x && en1.enemy.pos.y == bul.bullet.pos.y) || (en1.enemy.pos.x == bul.bullet.pos.x && en1.enemy.pos.y+1 == bul.bullet.pos.y) || (en1.enemy.pos.x+1 == bul.bullet.pos.x && en1.enemy.pos.y == bul.bullet.pos.y) || (en1.enemy.pos.x-1 == bul.bullet.pos.x && en1.enemy.pos.y == bul.bullet.pos.y) || (en1.enemy.pos.x+2 == bul.bullet.pos.x && en1.enemy.pos.y == bul.bullet.pos.y) || (en1.enemy.pos.x-2 == bul.bullet.pos.x && en1.enemy.pos.y == bul.bullet.pos.y) || (en1.enemy.pos.x+3 == bul.bullet.pos.x && en1.enemy.pos.y == bul.bullet.pos.y) || (en1.enemy.pos.x-3 == bul.bullet.pos.x && en1.enemy.pos.y == bul.bullet.pos.y)) {
			switch (en1.enemy.anim.state)
		{
		case ENEMY_STATE1:
			if(en1.enemy.anim.delay_cnt2 != 0){
				en1.enemy.anim.delay_cnt2--;
			} else
			{	
				en1.enemy.anim.delay_cnt2 = ENEMY_ANIM_DELAY;
				en1.enemy.anim.state = DESTROYED1;
				bul.bullet.pos.x = en1.enemy.pos.x;
				bul.bullet.pos.y = en1.enemy.pos.y;
			}
			break;
		case ENEMY_STATE2:
			if(en1.enemy.anim.delay_cnt2 != 0){
				en1.enemy.anim.delay_cnt2--;
			} else
			{	
				en1.enemy.anim.delay_cnt2 = ENEMY_ANIM_DELAY;
				en1.enemy.anim.state = DESTROYED1;
				bul.bullet.pos.x = en1.enemy.pos.x;
				bul.bullet.pos.y = en1.enemy.pos.y;
			}
			break;
		case DESTROYED1:
			if(en1.enemy.anim.delay_cnt2 != 0){
				en1.enemy.anim.delay_cnt2--;
			} else
			{	
				en1.enemy.anim.delay_cnt2 = ENEMY_ANIM_DELAY;
				en1.enemy.anim.state = DESTROYED2;
				bul.bullet.pos.x = en1.enemy.pos.x;
				bul.bullet.pos.y = en1.enemy.pos.y;
			}
			break;
		case DESTROYED2:
			if(en1.enemy.anim.delay_cnt2 != 0){
				en1.enemy.anim.delay_cnt2--;
			} else
			{	
				en1.enemy.anim.delay_cnt2 = ENEMY_ANIM_DELAY;
				en1.enemy.anim.state = DESTROYED3;
				bul.bullet.pos.x = en1.enemy.pos.x;
				bul.bullet.pos.y = en1.enemy.pos.y;
			}
			break;
		case DESTROYED3:
			if(en1.enemy.anim.delay_cnt2 != 0){
				en1.enemy.anim.delay_cnt2--;
			} else
			{	
				en1.enemy.anim.delay_cnt2 = ENEMY_ANIM_DELAY;
				bul.bullet.pos.x = en1.enemy.pos.x;
				bul.bullet.pos.y = en1.enemy.pos.y;
				
				// crna pozadina
				sleep(1);
				for(uint16_t r = 0; r < SCREEN_RGB333_H; r++){
					for(uint16_t c = 0; c < SCREEN_RGB333_W; c++){
						unpack_rgb333_p32[r*SCREEN_RGB333_W + c] = 0000;
					}
				}
				// ispis win
				draw_sprite_from_atlas(256, 16, 8, 8, 30, 48);
				draw_sprite_from_atlas(256, 16, 8, 8, 35, 56);
				draw_sprite_from_atlas(256, 16, 8, 8, 40, 64);
				draw_sprite_from_atlas(256, 16, 8, 8, 48, 48);
				draw_sprite_from_atlas(256, 16, 8, 8, 48, 56);
				draw_sprite_from_atlas(256, 16, 8, 8, 56, 64);
				draw_sprite_from_atlas(256, 16, 8, 8, 61, 56);
				draw_sprite_from_atlas(256, 16, 8, 8, 66, 48);
				draw_sprite_from_atlas(256, 16, 8, 8, 82, 48);
				draw_sprite_from_atlas(256, 16, 8, 8, 82, 56);
				draw_sprite_from_atlas(256, 16, 8, 8, 82, 64);
				draw_sprite_from_atlas(256, 16, 8, 8, 98, 48);
				draw_sprite_from_atlas(256, 16, 8, 8, 98, 56);
				draw_sprite_from_atlas(256, 16, 8, 8, 98, 64);
				draw_sprite_from_atlas(256, 16, 8, 8, 106, 56);
				draw_sprite_from_atlas(256, 16, 8, 8, 112, 64);
				draw_sprite_from_atlas(256, 16, 8, 8, 120, 48);
				draw_sprite_from_atlas(256, 16, 8, 8, 120, 56);
				draw_sprite_from_atlas(256, 16, 8, 8, 120, 64);
	
				sleep(120);

			}
			break;
		default:
			break;
		}
			//printf("POBEDA");			
			//break;
		}	
		

		// ako se tenkici dotaknu, game over
		if(en1.enemy.pos.x == gs.tank.pos.x && en1.enemy.pos.y == gs.tank.pos.y) {
			for(uint16_t r = 0; r < SCREEN_RGB333_H; r++){
				for(uint16_t c = 0; c < SCREEN_RGB333_W; c++){
					unpack_rgb333_p32[r*SCREEN_RGB333_W + c] = 0000;
				}
			}

			draw_sprite_from_atlas(288, 184, 32, 16, 64, 48);		// game over
			sleep(120);

			//exit(1);
		}

		switch (en1.enemy.anim.state)
		{
		case ENEMY_STATE1:
			if(en1.enemy.anim.delay_cnt != 0){
				en1.enemy.anim.delay_cnt--;
			} else
			{
				if(en1.enemy.pos.x != 16 && en1.enemy.anim.smer == ENEMY_LEFT) {
					en1.enemy.anim.delay_cnt = TANK_ANIM_DELAY;
					en1.enemy.anim.state = ENEMY_STATE2;
					en1.enemy.pos.x -= 1;
				} else if(en1.enemy.pos.x == 16 && en1.enemy.anim.smer == ENEMY_LEFT) {
					en1.enemy.anim.delay_cnt = TANK_ANIM_DELAY;
					en1.enemy.anim.state = ENEMY_STATE2;
					en1.enemy.anim.smer = ENEMY_RIGHT;
					en1.enemy.pos.x += 1;
				} else if(en1.enemy.pos.x != 128 && en1.enemy.anim.smer == ENEMY_RIGHT) {
					en1.enemy.anim.delay_cnt = TANK_ANIM_DELAY;
					en1.enemy.anim.state = ENEMY_STATE2;
					en1.enemy.pos.x += 1;				
				} else if(en1.enemy.pos.x == 128 && en1.enemy.anim.smer == ENEMY_RIGHT) {
					en1.enemy.anim.delay_cnt = TANK_ANIM_DELAY;
					en1.enemy.anim.state = ENEMY_STATE2;
					en1.enemy.anim.smer = ENEMY_LEFT;
					en1.enemy.pos.x -= 1;			
				}
			}			
			break;
		case ENEMY_STATE2:
			if(en1.enemy.anim.delay_cnt != 0){
				en1.enemy.anim.delay_cnt--;
			} else
			{
				if(en1.enemy.pos.x != 16 && en1.enemy.anim.smer == ENEMY_LEFT) {
					en1.enemy.anim.delay_cnt = TANK_ANIM_DELAY;
					en1.enemy.anim.state = ENEMY_STATE1;
					en1.enemy.pos.x -= 1;
				} else if(en1.enemy.pos.x == 16 && en1.enemy.anim.smer == ENEMY_LEFT) {
					en1.enemy.anim.delay_cnt = TANK_ANIM_DELAY;
					en1.enemy.anim.state = ENEMY_STATE1;
					en1.enemy.anim.smer = ENEMY_RIGHT;
					en1.enemy.pos.x += 1;
				} else if(en1.enemy.pos.x != 128 && en1.enemy.anim.smer == ENEMY_RIGHT) {
					en1.enemy.anim.delay_cnt = TANK_ANIM_DELAY;
					en1.enemy.anim.state = ENEMY_STATE1;
					en1.enemy.pos.x += 1;				
				} else if(en1.enemy.pos.x == 128 && en1.enemy.anim.smer == ENEMY_RIGHT) {
					en1.enemy.anim.delay_cnt = TANK_ANIM_DELAY;
					en1.enemy.anim.state = ENEMY_STATE1;
					en1.enemy.anim.smer = ENEMY_LEFT;
					en1.enemy.pos.x -= 1;			
				}	
			}			
			break;
		default:
			break;
		}
		
		switch(gs.tank.anim.state){
		case TANK_STATE1:
			if(mov_x != 0 || mov_y != 0){
				gs.tank.anim.delay_cnt = TANK_ANIM_DELAY;
				gs.tank.anim.state = TANK_STATE2;
			}
			break;
		case TANK_STATE2:
			if(gs.tank.anim.delay_cnt != 0){
					gs.tank.anim.delay_cnt--;
			}else{
				gs.tank.anim.delay_cnt = TANK_ANIM_DELAY;
				gs.tank.anim.state = TANK_STATE1;
			}
			break;
		}		

		// bullet moving
		if(en1.enemy.pos.x != bul.bullet.pos.x || en1.enemy.pos.y != bul.bullet.pos.y) {
			if(en1.enemy.anim.state != DESTROYED3) {
				if(bul.bullet.pos.x != 130 && bul.bullet.pos.x != 15 && bul.bullet.pos.y != 15 && bul.bullet.pos.y != 100) {
					if(bul.bullet.anim.smer == BULLET_LEFT) {
						bul.bullet.pos.x -= 1;		
					} else if(bul.bullet.anim.smer == BULLET_RIGHT) {
						bul.bullet.pos.x += 1;
					} else if(bul.bullet.anim.smer == BULLET_UP) {
						bul.bullet.pos.y -= 1;
					} else if(bul.bullet.anim.smer == BULLET_DOWN) {
						bul.bullet.pos.y += 1;
					}
				} else {
					bul.bullet.pos.x = gs.tank.pos.x;
					bul.bullet.pos.y = gs.tank.pos.y;
					bul.bullet.anim.smer = gs.tank.anim.smer;		
				}
			}
		}
		//


		/////////////////////////////////////
		// Drawing.
		
		
		// Detecting rising edge of VSync.
		WAIT_UNITL_0(gpu_p32[2]);
		WAIT_UNITL_1(gpu_p32[2]);
		// Draw in buffer while it is in VSync.
		
		// Black background.
		for(uint16_t r = 0; r < SCREEN_RGB333_H; r++){
			for(uint16_t c = 0; c < SCREEN_RGB333_W; c++){
				unpack_rgb333_p32[r*SCREEN_RGB333_W + c] = 0000;
			}
		}
		

		// Draw enemy.
		switch (en1.enemy.anim.smer)
		{
		case ENEMY_RIGHT:
			switch (en1.enemy.anim.state)
			{
			
			case ENEMY_STATE1:
				draw_sprite_from_atlas(224, 0, 16, 16, en1.enemy.pos.x, en1.enemy.pos.y);
				break;
			case ENEMY_STATE2:
				draw_sprite_from_atlas(240, 0, 16, 16, en1.enemy.pos.x, en1.enemy.pos.y);
				break;
			case DESTROYED1:
				draw_sprite_from_atlas(272, 128, 16, 16, en1.enemy.pos.x, en1.enemy.pos.y);
				break;
			case DESTROYED2:
				draw_sprite_from_atlas(288, 128, 16, 16, en1.enemy.pos.x, en1.enemy.pos.y);
				break;
			case DESTROYED3:
				draw_sprite_from_atlas(304, 128, 16, 16, en1.enemy.pos.x, en1.enemy.pos.y);
				break;
			default:
				break;
			}
			break;
		
		case ENEMY_LEFT:
			switch (en1.enemy.anim.state)
			{
			case ENEMY_STATE1:
				draw_sprite_from_atlas(160, 0, 16, 16, en1.enemy.pos.x, en1.enemy.pos.y);
				break;
			case ENEMY_STATE2:
				draw_sprite_from_atlas(176, 0, 16, 16, en1.enemy.pos.x, en1.enemy.pos.y);
				break;
			case DESTROYED1:
				draw_sprite_from_atlas(272, 128, 16, 16, en1.enemy.pos.x, en1.enemy.pos.y);
				break;
			case DESTROYED2:
				draw_sprite_from_atlas(288, 128, 16, 16, en1.enemy.pos.x, en1.enemy.pos.y);
				break;
			case DESTROYED3:
				draw_sprite_from_atlas(304, 128, 16, 16, en1.enemy.pos.x, en1.enemy.pos.y);
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}

		
		// Draw tank.
		switch (gs.tank.anim.smer)
		{
		case TANK_TURNING_UP:
			if(gs.tank.anim.state == TANK_STATE1) {
				draw_sprite_from_atlas(
					0, 0, 16, 16, gs.tank.pos.x, gs.tank.pos.y
				);
			} else {
				draw_sprite_from_atlas(
					16, 0, 16, 16, gs.tank.pos.x, gs.tank.pos.y
				);
			}
			break;
		case TANK_TURNING_LEFT:
			if(gs.tank.anim.state == TANK_STATE1) {
				draw_sprite_from_atlas(
					32, 0, 16, 16, gs.tank.pos.x, gs.tank.pos.y
				);
			} else {
				draw_sprite_from_atlas(
					48, 0, 16, 16, gs.tank.pos.x, gs.tank.pos.y
				);
			}
			break;
		case TANK_TURNING_DOWN:
			if(gs.tank.anim.state == TANK_STATE1) {
				draw_sprite_from_atlas(
					64, 0, 16, 16, gs.tank.pos.x, gs.tank.pos.y
				);
			} else {
				draw_sprite_from_atlas(
					80, 0, 16, 16, gs.tank.pos.x, gs.tank.pos.y
				);
			}
			break;
		case TANK_TURNING_RIGHT:
			if(gs.tank.anim.state == TANK_STATE1) {
				draw_sprite_from_atlas(
					96, 0, 16, 16, gs.tank.pos.x, gs.tank.pos.y
				);
			} else {
				draw_sprite_from_atlas(
					112, 0, 16, 16, gs.tank.pos.x, gs.tank.pos.y
				);
			}
			break;
		default:
			break;
		}
		
		// wall
		for(int i=0; i<=144; i+=16) {
		draw_sprite_from_atlas(
					256, 0, 16, 16, i, 112
				);
		}
		for(int i=0; i<=144; i+=16) {
		draw_sprite_from_atlas(
					256, 0, 16, 16, i, 0
				);
		}
		for(int i=16; i<=96; i+=16) {
		draw_sprite_from_atlas(
					256, 0, 16, 16, 0, i
				);
		}
		for(int i=16; i<=96; i+=16) {
		draw_sprite_from_atlas(
					256, 0, 16, 16, 144, i
				);
		}
		for(int i=64; i<=112; i+=16) {
		draw_sprite_from_atlas(
					256, 0, 16, 16, i, 32
				);
		}
		for(int i=64; i<=112; i+=16) {
		draw_sprite_from_atlas(
					256, 0, 16, 16, i, 64
				);
		}
		draw_sprite_from_atlas(
					256, 0, 16, 16, 32, 32
				);
		draw_sprite_from_atlas(
					256, 0, 16, 16, 32, 64
				);
		draw_sprite_from_atlas(
					256, 0, 16, 16, 32, 80
				);
		// water
		for(int i=64; i<=112; i+=16) {
		draw_sprite_from_atlas(
					272, 48, 16, 16, i, 80
				);
		}
		// bush
		draw_sprite_from_atlas(
					272, 32, 16, 16, 48, 64
				);
		draw_sprite_from_atlas(
					272, 32, 16, 16, 128, 32
				);
		draw_sprite_from_atlas(
					272, 32, 16, 16, 32, 48
				);

		// Draw bullets.
		switch (bul.bullet.anim.smer)
		{
		case BULLET_UP:
				draw_sprite_from_atlas(
					320, 96, 8, 16, bul.bullet.pos.x+3, bul.bullet.pos.y-3
				);
			break;
		case BULLET_LEFT:
			draw_sprite_from_atlas(
					328, 96, 8, 16, bul.bullet.pos.x, bul.bullet.pos.y
				);
			break;
		case BULLET_DOWN:
			draw_sprite_from_atlas(
					336, 96, 8, 16, bul.bullet.pos.x+3, bul.bullet.pos.y+3
				);
			break;
		case BULLET_RIGHT:
			draw_sprite_from_atlas(
					344, 96, 8, 16, bul.bullet.pos.x+9, bul.bullet.pos.y
				);
			break;
		default:
			break;
		}
		
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
