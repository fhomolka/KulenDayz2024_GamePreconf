#include "raylib.h"
#include "raymath.h"
#include <math.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define TARGET_FPS 60
#define CRT_BLACK (Color){18, 18, 18, 255}

#define GRAVITY 10.0f

static inline float sprite_dir(float val)
{
	if (val < -EPSILON) return -1.0f;
	return 1.0f;
}

// Player

#define PLAYER_SPRITESHEET_PATH "res/sprites/player_sprite.png"
#define PLAYER_TILE_SIZE 32.0f
#define PLAYER_SPEED 200.0f
#define PLAYER_FRAME_TIME (6.0f/(float)TARGET_FPS)
#define PLAYER_SHOOT_DELAY 0.3f;


enum PLAYER_TILES
{
	P_IDLE,
	P_WALK0,
	P_WALK1,
	P_WALK2,
	P_WALK3,
	P_SHOOT0,
	P_PAIN,
};

const Rectangle PLAYER_SHEET[] = 
{
	{0 * PLAYER_TILE_SIZE, 0 * PLAYER_TILE_SIZE, PLAYER_TILE_SIZE, PLAYER_TILE_SIZE}, //IDLE
	{0 * PLAYER_TILE_SIZE, 1 * PLAYER_TILE_SIZE, PLAYER_TILE_SIZE, PLAYER_TILE_SIZE}, //WALK0
	{1 * PLAYER_TILE_SIZE, 1 * PLAYER_TILE_SIZE, PLAYER_TILE_SIZE, PLAYER_TILE_SIZE}, //WALK1
	{2 * PLAYER_TILE_SIZE, 1 * PLAYER_TILE_SIZE, PLAYER_TILE_SIZE, PLAYER_TILE_SIZE}, //WALK2
	{3 * PLAYER_TILE_SIZE, 1 * PLAYER_TILE_SIZE, PLAYER_TILE_SIZE, PLAYER_TILE_SIZE}, //WALK3
	{0 * PLAYER_TILE_SIZE, 2 * PLAYER_TILE_SIZE, PLAYER_TILE_SIZE, PLAYER_TILE_SIZE}, //SHOOT0
	{0 * PLAYER_TILE_SIZE, 3 * PLAYER_TILE_SIZE, PLAYER_TILE_SIZE, PLAYER_TILE_SIZE}, //PAIN

};

typedef enum player_state_u
{
	PLAYER_IDLE,
	PLAYER_WALK,
	PLAYER_SHOOT,
	PLAYER_DAMAGED,
	PLAYER_DEAD,
} player_state_t;

typedef struct player_s
{
	player_state_t state;
	Texture texture;
	int current_frame;

	Vector2 pos;
	Vector2 move_dir;
	Vector2 velocity;
	
	float next_frame_time;
	float next_shot;

	int health;
} player_t;
player_t g_player;

void draw_player(player_t *player)
{
	Rectangle dest = 
	{
		.x = player->pos.x, 
		.y = player->pos.y - PLAYER_TILE_SIZE, 
		.width = PLAYER_TILE_SIZE, 
		.height = PLAYER_TILE_SIZE
	};

	Rectangle src = PLAYER_SHEET[g_player.current_frame];
	src.width *= sprite_dir(g_player.move_dir.x);

	DrawTexturePro(g_player.texture, src, dest, (Vector2){0,0}, 0, WHITE);
}

// Bullets

#define MAX_BULLETS 3
#define BULLET_SPEED 200.0f
typedef struct bullet_s
{
	Rectangle collision_rect;
	_Bool active;
} bullet_t;

bullet_t bullets[MAX_BULLETS];
int bullet_count = 0;

// Bubbles

#define MAX_BUBBLES 8
#define BUBBLE_RADIUS_L 100.0f
#define BUBBLE_RADIUS_M 40.0f
#define BUBBLE_RADIUS_S 10.0f
#define BUBBLE_COLOR_L (RED)
#define BUBBLE_COLOR_M (YELLOW)
#define BUBBLE_COLOR_S (GREEN)

typedef struct bubble_s
{
	_Bool active;
	Vector2 pos;
	float radius;
	Color color;

	Vector2 velocity;
} bubble_t;

bubble_t bubbles[MAX_BUBBLES];
int active_bubbles = 1;

Sound bubble_pop_sfx;

// Game State

typedef enum game_state_u
{
	G_PLAYING,
	G_WIN,
	G_LOSS,
} game_state_u;

game_state_u g_game_state = G_PLAYING;

// Main Loop


void reset_state()
{
	g_game_state = G_PLAYING;

	g_player.current_frame = P_WALK0;
	g_player.next_frame_time = 0.0f;
	g_player.next_shot = 0.0f;
	g_player.pos.y = SCREEN_HEIGHT;
	g_player.pos.x = 10.0f;
	g_player.state = PLAYER_IDLE;
	g_player.health = 3;

	bubbles[0] = (bubble_t)
	{
		.pos.x = SCREEN_WIDTH / 2.0f,
		.pos.y = SCREEN_HEIGHT / 2.0f,
		.radius = BUBBLE_RADIUS_L,
		.color = BUBBLE_COLOR_L,
		.velocity = (Vector2){1.0f, -1.0f},
		.active = true,
	};

	active_bubbles = 1;

	for(int i = 1; i <= MAX_BUBBLES; i += 1)
	{
		bubbles[i].active = false;
	}

	for(int i = 0; i <= MAX_BULLETS; i += 1)
	{
		bullets[i].active = false;
	}
}

int main(void)
{
	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Bubble Destroyer from scratch");
	SetTargetFPS(TARGET_FPS);

	RenderTexture2D screen_texture = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
	Shader rainbow_shader = LoadShader(0, "res/shaders/rainbow.fs");
	Shader wave_shader = LoadShader(0, "res/shaders/wave.fs");
	Shader scanline_shader = LoadShader(0, "res/shaders/scanlines.fs");
	
	InitAudioDevice();
	bubble_pop_sfx = LoadSound("res/sfx/soft_boop.wav");
	Sound gun_fire = LoadSound("res/sfx/gun_fire.wav");
	Sound player_hurt_sfx = LoadSound("res/sfx/hurt.wav");

	g_player.texture = LoadTexture(PLAYER_SPRITESHEET_PATH);
	Texture space_texture = LoadTexture("res/sprites/space.png");

	float time_since_start = 0.0f;

	reset_state();

	while(!WindowShouldClose())
	{
		float delta_time = GetFrameTime();
		time_since_start += delta_time;

		//Update
		do
		{
			if(IsKeyPressed(KEY_R)) reset_state();
			if(IsKeyPressed(KEY_F1)) g_game_state = G_WIN;

			if(g_game_state != G_PLAYING) break;

			// Update Player
			do
			{	
				//Can't do anything if dead
				if (g_player.state == PLAYER_DEAD) break;
				if (g_player.state == PLAYER_SHOOT && g_player.next_frame_time > time_since_start)  break;

				//Collect inputs
				_Bool desired_shoot = IsKeyPressed(KEY_SPACE);
				float desired_move = IsKeyDown(KEY_RIGHT) - IsKeyDown(KEY_LEFT);

				if(g_player.state == PLAYER_DAMAGED && g_player.next_frame_time > time_since_start)
				{
					desired_shoot = false;
					desired_move = 0.0f;
					goto skip_inputs;
				}

				//Process Inputs
				do
				{
					if(desired_shoot && g_player.next_shot <= time_since_start && bullet_count < MAX_BULLETS)
					{
						g_player.state = PLAYER_SHOOT;
						break;
					}
					if(desired_move != 0.0f)
					{
						g_player.state = PLAYER_WALK;
						break;
					}
					g_player.state = PLAYER_IDLE;
				}
				while(0);

				skip_inputs:

				//Process States
				switch(g_player.state)
				{
					case PLAYER_SHOOT:
					{
						for(int i = 0; i < MAX_BULLETS; i += 1)
						{
							if(bullets[i].active) continue;
							bullets[i].active = true;

							bullets[i].collision_rect.x = g_player.pos.x;
							bullets[i].collision_rect.y = g_player.pos.y - PLAYER_TILE_SIZE;
							bullets[i].collision_rect.width = 5.0f;
							bullets[i].collision_rect.height = 6.0f;
							bullet_count += 1;
							PlaySound(gun_fire);
							break;
						}

						g_player.next_shot = time_since_start + PLAYER_SHOOT_DELAY;

						g_player.current_frame = P_SHOOT0;
						g_player.next_frame_time = time_since_start + PLAYER_FRAME_TIME + 0.05;

						break;
					}
					case PLAYER_WALK:
					{
						g_player.move_dir.x = desired_move;

						g_player.move_dir = Vector2Normalize(g_player.move_dir);
						g_player.velocity = Vector2Scale(g_player.move_dir, PLAYER_SPEED * delta_time);

						g_player.pos = Vector2Add(g_player.pos, g_player.velocity);

						if(g_player.pos.x < 0) g_player.pos.x = 0;
						else if (g_player.pos.x + PLAYER_TILE_SIZE > SCREEN_WIDTH) g_player.pos.x = SCREEN_WIDTH - PLAYER_TILE_SIZE;


						if(g_player.next_frame_time <= time_since_start)
						{
							g_player.current_frame += 1;
							g_player.next_frame_time = time_since_start + PLAYER_FRAME_TIME;
						}

						if (!(P_WALK0 < g_player.current_frame && g_player.current_frame < P_WALK3))
						{
							g_player.current_frame = P_WALK1;
						}

						break;
					}
					case PLAYER_IDLE: {g_player.current_frame = P_IDLE; break;}
					case PLAYER_DAMAGED: 
					{
						g_player.current_frame = P_PAIN; 
						break;
					}
					case PLAYER_DEAD: //fallthrough
					default: break;
					
				}
			}
			while(0);
			
			//Update Bullets
			for(int i = 0; i < MAX_BULLETS; i += 1)
			{
				if(!bullets[i].active) continue;
				bullet_t *bullet = &bullets[i];

				bullet->collision_rect.y -= BULLET_SPEED * delta_time;

				if(bullet->collision_rect.y + bullet->collision_rect.height < 0)
				{
					bullet->active = false;
					bullet_count -= 1;
					continue;
				}

				for(int j = 0; j < MAX_BUBBLES; j += 1)
				{
					bubble_t *bubble = &bubbles[j];
					if (!bubble->active) continue;

					if(CheckCollisionCircleRec(bubble->pos, bubble->radius, bullet->collision_rect))
					{
						if(bubble->radius <= BUBBLE_RADIUS_S)
						{
							bubble->active = false;
							bullet->active = false;
							bullet_count -= 1;
							active_bubbles -= 1;
							PlaySound(bubble_pop_sfx);

							if(active_bubbles <= 0) g_game_state = G_WIN;

							break;
						}
						else if (bubble->radius <= BUBBLE_RADIUS_M)
						{
							bubble->radius = BUBBLE_RADIUS_S;
							bubble->color = BUBBLE_COLOR_S;
							bubble->velocity.y = -fabsf(bubble->velocity.y);
						}
						else if (bubble->radius <= BUBBLE_RADIUS_L)
						{
							bubble->radius = BUBBLE_RADIUS_M;
							bubble->color = BUBBLE_COLOR_M;
							bubble->velocity.y = -fabsf(bubble->velocity.y);
						}

						for(int j = 0; j < MAX_BUBBLES; j += 1)
						{
							if(bubbles[j].active) continue;
							bubble_t *other_bubble = &bubbles[j];
							other_bubble->radius = bubble->radius;
							other_bubble->pos = bubble->pos;
							other_bubble->color = bubble->color;
							other_bubble->active = true;
							other_bubble->velocity = (Vector2){-bubble->velocity.x, bubble->velocity.y};
							active_bubbles += 1;
							break;
						}

						PlaySound(bubble_pop_sfx);

						bullet->active = false;
						bullet_count -= 1;
						break;
					}
				}
			}

			//Update Bubbles
			for(int i = 0; i < MAX_BUBBLES; i += 1)
			{
				bubble_t *bubble = &bubbles[i];
				if (!bubble->active) continue;

				bubble->velocity.y += GRAVITY * delta_time;

				bubble->pos = Vector2Add(bubble->pos, bubble->velocity);

				if(bubble->pos.x + bubble->radius >= SCREEN_WIDTH)
				{
					bubble->pos.x = SCREEN_WIDTH - bubble->radius;
					bubble->velocity.x = -bubble->velocity.x;
				}
				else if (bubble->pos.x - bubble->radius < 0)
				{
					bubble->pos.x = bubble->radius;
					bubble->velocity.x = -bubble->velocity.x;
				}

				if(bubble->pos.y + bubble->radius >= SCREEN_HEIGHT)
				{
					bubble->pos.y = SCREEN_HEIGHT - bubble->radius;
					bubble->velocity.y = -GRAVITY;
				}
				else if (bubble->pos.y - bubble->radius < 0)
				{
					bubble->pos.y = bubble->radius;
					bubble->velocity.y = -bubble->velocity.y;
				}

				if(CheckCollisionCircleRec(bubble->pos, bubble->radius, 
					(Rectangle){g_player.pos.x, g_player.pos.y - PLAYER_TILE_SIZE * 0.8f, PLAYER_TILE_SIZE, PLAYER_TILE_SIZE})
					)
				{
					//Deal damage to player
					if(g_player.state != PLAYER_DAMAGED)
					{
						g_player.state = PLAYER_DAMAGED;
						g_player.next_frame_time = time_since_start + PLAYER_FRAME_TIME + 0.5;

						g_player.health -= 1;
						PlaySound(player_hurt_sfx);

						if(!g_player.health)
						{
							g_game_state = G_LOSS;
						}
					}	

					//Boing!
					bubble->velocity.y = -bubble->velocity.y;
				}

			}	
		}while(0);

		//Draw
		{
			//Draw to Texture	
			BeginTextureMode(screen_texture);
			{
				ClearBackground(BLACK);

				switch(g_game_state)
				{
					case G_PLAYING:
					{
						BeginShaderMode(wave_shader);
						SetShaderValue(wave_shader, GetShaderLocation(wave_shader, "time_since_start"), &time_since_start, SHADER_UNIFORM_FLOAT);
						DrawTexture(space_texture, 0, 0, WHITE);
						EndShaderMode();

						for(int i = 0; i < MAX_BULLETS; i += 1)
						{
							if(!bullets[i].active) continue;
							DrawRectangleRec(bullets[i].collision_rect, WHITE);
						}

						for(int i = 0; i < MAX_BUBBLES; i += 1)
						{
							bubble_t *bubble = &bubbles[i];
							if (!bubble->active) continue;
							DrawCircleV(bubble->pos, bubble->radius, bubble->color);
						}

						draw_player(&g_player);

						const char *player_hp_text = TextFormat("%i", g_player.health);
						DrawText(player_hp_text, 0, 0, 50, WHITE);

						DrawText(TextFormat("%i", active_bubbles), 0, 60, 50, WHITE);

						break;
					}
					case G_LOSS:
					{
						const char *loss_text = "You Lose!";
						int len = MeasureText(loss_text, 100);
						int centre_x = (SCREEN_WIDTH / 2.0f) - (len / 2.0f);
						int centre_y = (SCREEN_HEIGHT / 2.0f) - (100 / 2.0f);
						DrawText(loss_text, centre_x, centre_y, 100, WHITE);
						break;
					}
					case G_WIN: 
					{
						const char *victory_text = "You Win!";
						int len = MeasureText(victory_text, 100);
						int centre_x = (SCREEN_WIDTH / 2.0f) - (len / 2.0f);
						int centre_y = (SCREEN_HEIGHT / 2.0f) - (100 / 2.0f);
						BeginShaderMode(rainbow_shader);
						SetShaderValue(rainbow_shader, GetShaderLocation(rainbow_shader, "time_since_start"), &time_since_start, SHADER_UNIFORM_FLOAT);
						DrawText(victory_text, centre_x, centre_y, 100, WHITE);
						EndShaderMode();
						break;
					}
 				}

				
			}
			EndTextureMode();

			//Draw to screen
			BeginDrawing();
			{
				BeginShaderMode(scanline_shader);
				SetShaderValue(scanline_shader, GetShaderLocation(scanline_shader, "time_since_start"), &time_since_start, SHADER_UNIFORM_FLOAT);
				//negative height because OpenGL renders in first quadrant, but we're in the fourth
				DrawTextureRec(screen_texture.texture, (Rectangle){0, 0, screen_texture.texture.width, -screen_texture.texture.height}, (Vector2){0, 0}, WHITE);
				EndShaderMode();
			}
			EndDrawing();
		}
	}

	CloseAudioDevice();
	CloseWindow();
	return 0;
}