#include "raylib.h"
#include "raymath.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define CRT_BLACK (Color){18, 18, 18, 255}

static Rectangle MoveRectangle(Rectangle rect, Vector2 vec)
{
	return (Rectangle){rect.x + vec.x, rect.y + vec.y, rect.width, rect.height};
}

typedef struct
{
	Rectangle rect;
	Color color;
	_Bool active;
} entity_t;

//Player
#define PLAYER_WIDTH 100
#define PLAYER_HEIGHT 20
#define PLAYER_SPEED 500.0f
entity_t player = (entity_t)
{
	.rect = (Rectangle) 
	{
		.x = (SCREEN_WIDTH / 2.0f) - (PLAYER_WIDTH / 2.0f), //Find centre of screen, then move player by half of it's width 
		.y = SCREEN_HEIGHT - (2.0f * PLAYER_HEIGHT), //Lift away from bottom of the screen by 2x player height 
		.width = PLAYER_WIDTH, 
		.height = PLAYER_HEIGHT 
	},
	.color = RED,
	.active = true
};


//Ball
#define BALL_SIZE 20
#define BALL_SPEED 600.0f
entity_t ball = (entity_t)
{
	.rect = (Rectangle)
	{
		.x = SCREEN_WIDTH / 2.0f - BALL_SIZE / 2.0f,// Same as player
		.y = SCREEN_HEIGHT - (3.5f * PLAYER_HEIGHT), //Lift he ball a bit above the player	
		.width = BALL_SIZE, .height = BALL_SIZE
	},
	.color = RED,
	.active = false
};
Vector2 ball_move_dir = {-1.0f, -1.0f};
Vector2 ball_last_move_dir = {-1.0f, -1.0f};

#define BRICK_COLUMN_COUNT 8
#define BRICK_ROW_COUNT 5
#define BRICK_MARGIN 2.0f
#define BRICK_WIDTH ((SCREEN_WIDTH - BRICK_MARGIN) / BRICK_COLUMN_COUNT - BRICK_MARGIN)
#define BRICK_HEIGHT 20
#define BRICK_COUNT (BRICK_COLUMN_COUNT * BRICK_ROW_COUNT)
entity_t bricks[BRICK_COUNT];
int remaining_bricks = 0;

Color brick_color_lut[] = {RED, ORANGE, YELLOW, GREEN, BLUE};

int main(void)
{
	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Breakout from scratch");
	SetTargetFPS(240);
	InitAudioDevice();

	float time_since_start = 0;

	RenderTexture2D screen_texture = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);

	Shader rainbow_shader = LoadShader(0, "res/shaders/rainbow.fs");
	Shader scanline_shader = LoadShader(0, "res/shaders/scanlines.fs");

	Sound bounce_sound = LoadSound("res/sfx/boop.wav");

//Set up bricks
#define STATIC_LEVEL
#ifdef STATIC_LEVEL
	Vector2 offset = (Vector2){BRICK_MARGIN, BRICK_MARGIN};
	for(int y = 0; y < BRICK_ROW_COUNT; y += 1)
	{
		for(int x = 0; x < BRICK_COLUMN_COUNT; x += 1)
		{
			entity_t *brick = &bricks[x + BRICK_COLUMN_COUNT * y];
			brick->rect.x = offset.x;
			brick->rect.y = offset.y;
			brick->rect.width = BRICK_WIDTH;
			brick->rect.height = BRICK_HEIGHT;
			brick->color = brick_color_lut[y];
			brick->active = true;

			offset.x += BRICK_WIDTH + BRICK_MARGIN;
			remaining_bricks += 1;
		}

		offset.x = BRICK_MARGIN;
		offset.y += BRICK_HEIGHT + BRICK_MARGIN;
	}
#else //STATIC_LEVEL
	Image map_image = LoadImage("res/maps/map01.png");
	Color *map_colormap = LoadImageColors(map_image);

	Vector2 offset = (Vector2){BRICK_MARGIN, BRICK_MARGIN};
	for(int y = 0; y < BRICK_ROW_COUNT; y += 1)
	{
		for(int x = 0; x < BRICK_COLUMN_COUNT; x += 1)
		{
			int idx = x + BRICK_COLUMN_COUNT * y;
			Color *current_tile = &map_colormap[idx];

			if(current_tile->a != 255) 
			{
				offset.x += BRICK_WIDTH + BRICK_MARGIN;
				continue;
			}

			entity_t *brick = &bricks[x + BRICK_COLUMN_COUNT * y];
			brick->rect.x = offset.x;
			brick->rect.y = offset.y;
			brick->rect.width = BRICK_WIDTH;
			brick->rect.height = BRICK_HEIGHT;
			brick->color = *current_tile;
			brick->active = true;

			remaining_bricks += 1;

			offset.x += BRICK_WIDTH + BRICK_MARGIN;
		}
		offset.x = BRICK_MARGIN;
		offset.y += BRICK_HEIGHT + BRICK_MARGIN;
	}

	UnloadImageColors(map_colormap);
	UnloadImage(map_image);
#endif

	while(!WindowShouldClose())
	{
		float delta_time = GetFrameTime();
		time_since_start += delta_time;

		//Update
		{
			if(IsKeyPressed(KEY_F1))
			{
				remaining_bricks = 0;
			}

			/* This works because Booleans can covnert to integers, so left can be -1 and right can be 1 */
			float dir_input = IsKeyDown(KEY_RIGHT) - IsKeyDown(KEY_LEFT);

			if(!ball.active && IsKeyPressed(KEY_SPACE))
			{
				ball_move_dir = Vector2Normalize(ball_move_dir); 
				ball.active = true;
			}

			player.rect.x += dir_input * PLAYER_SPEED * delta_time;
			//player.rect.x = Clamp(player.rect.x, 0, SCREEN_WIDTH - PLAYER_WIDTH);
			if(player.rect.x < 0) player.rect.x = 0;
			else if(player.rect.x + PLAYER_WIDTH > SCREEN_WIDTH) player.rect.x = SCREEN_WIDTH - PLAYER_WIDTH;

			if (!ball.active)
			{
				ball.rect.x = (player.rect.x * 2 + PLAYER_WIDTH) / 2.0f - (BALL_SIZE / 2.0f);
				ball.rect.y = SCREEN_HEIGHT - (3.5f * PLAYER_HEIGHT);
			}
			else
			{
				Vector2 ball_velocity = Vector2Scale(ball_move_dir, BALL_SPEED * delta_time);
				ball.rect = MoveRectangle(ball.rect, ball_velocity);

				if (ball.rect.x < 0)
				{
					ball.rect.x = 0;
					ball_move_dir.x = -ball_move_dir.x;
				}
				else if (ball.rect.x + ball.rect.width > SCREEN_WIDTH)
				{
					ball.rect.x = SCREEN_WIDTH - ball.rect.width;
					ball_move_dir.x = -ball_move_dir.x;
				}

				if (ball.rect.y < 0)
				{
					ball.rect.y = 0;
					ball_move_dir.y = -ball_move_dir.y;
				}
				else if(ball.rect.y > SCREEN_HEIGHT)
				{
					ball.active = false;
				}

				//Check Collision against player
				if(CheckCollisionRecs(ball.rect, player.rect) && ball_move_dir.y > 0)
				{
					ball_move_dir.y = -ball_move_dir.y;

					float player_centre = (player.rect.x + player.rect.x + player.rect.width) / 2.0f;
					float ball_centre = (ball.rect.x + ball.rect.x + ball.rect.width) / 2.0f;

					if (ball_centre < player_centre) { ball_move_dir.x = -fabsf(ball_move_dir.x); }
					if (ball_centre > player_centre) { ball_move_dir.x =  fabsf(ball_move_dir.x); }
				}

				//Check Collisions against Bricks
				for(int i = 0; i < BRICK_COUNT; i += 1)
				{
					if(!bricks[i].active) continue; //Already inactive
					if(!CheckCollisionRecs(ball.rect, bricks[i].rect)) continue; //Did not hit the brick

					bricks[i].active = false;
					remaining_bricks -= 1;

					//This is just a dumb aabb check again, but this time with side info
					if(ball.rect.y < bricks[i].rect.y + bricks[i].rect.height) //ball is below
					{
						if (ball_move_dir.y < 0.0f)
							ball_move_dir.y = -ball_move_dir.y; 
						continue;
					}
					if(ball.rect.y + ball.rect.height > bricks[i].rect.y) //ball is above
					{
						if (ball_move_dir.y > 0.0f)
							ball_move_dir.y = -ball_move_dir.y; 
						continue;
					}
					if(ball.rect.x < bricks[i].rect.x + bricks[i].rect.width) //ball is right
					{
						if (ball_move_dir.x < 0.0f)
							ball_move_dir.x = -ball_move_dir.x;
						continue;
					}
					if(ball.rect.x + ball.rect.width > bricks[i].rect.x) //ball is left
					{
						if (ball_move_dir.x > 0.0f)
							ball_move_dir.x = -ball_move_dir.x; 
						continue;
					}
				}

				if(!Vector2Equals(ball_move_dir, ball_last_move_dir))
				{
					PlaySound(bounce_sound);
				}
				ball_last_move_dir = ball_move_dir;
			}
		}

		//Draw
		{
			// Draw to screen texture
			{
				BeginTextureMode(screen_texture);
				ClearBackground(BLACK);
				if(remaining_bricks <= 0)
				{
					const char *victory_text = "You Won!";
					int len = MeasureText(victory_text, 100);
					int centre_x = (SCREEN_WIDTH / 2.0f) - (len / 2.0f);
					int centre_y = (SCREEN_HEIGHT / 2.0f) - (100 / 2.0f);
					BeginShaderMode(rainbow_shader);
					SetShaderValue(rainbow_shader, GetShaderLocation(rainbow_shader, "time_since_start"), &time_since_start, SHADER_UNIFORM_FLOAT);
					DrawText(victory_text, centre_x, centre_y, 100, WHITE);
					EndShaderMode();
				}
				else
				{
					for(int i = 0; i < BRICK_COUNT; i += 1)
					{
						if(bricks[i].active)
						DrawRectangleRec(bricks[i].rect, bricks[i].color);
					}

					DrawRectangleRec(player.rect, player.color);
					DrawRectangleRec(ball.rect, ball.color);
				}
				EndTextureMode();
			}
			
			BeginDrawing();
			BeginShaderMode(scanline_shader);
			//DrawTexture(screen_texture.texture, 0, 0, WHITE); //Un-flipped mistake
			DrawTextureRec(screen_texture.texture, (Rectangle){0, 0, screen_texture.texture.width, -screen_texture.texture.height}, (Vector2){0, 0}, WHITE);
			EndShaderMode();
			EndDrawing();
		}
	}

	CloseAudioDevice();
	CloseWindow();
	return 0;
}


/*
lighting atmospheric scatering 
*/