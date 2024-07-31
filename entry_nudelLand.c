const int tile_width = 16;
const float entity_selection_radius = 10.0f;
const int player_health = 10;
const int plastic_health = 1;
const int wood_health = 1;
const float player_pickup_radius = 20.0f;
// ^^^ constants


inline float v2_dist(Vector2 a, Vector2 b) {
    return v2_length(v2_sub(a, b));
}
int world_to_tile_pos(float world_pos){
	return world_pos / (float) tile_width;
}

// ^^^ game engine stuff 


// moves from 0 -> 1 instead of
float sin_breathe(float time, float rate){
	return (sin(time * rate) + 1.0) / 2.0;
}

bool almost_equals(float a, float b, float epsilon) {
 return fabs(a - b) <= epsilon;
}

bool animate_f32_to_target(float* value, float target, float delta_t, float rate) {
	*value += (target - *value) * (1.0 - pow(2.0f, -rate * delta_t));
	if (almost_equals(*value, target, 0.001f))
	{
		*value = target;
		return true; // reached
	}
	return false;
}

void animate_v2_to_target(Vector2* value, Vector2 target, float delta_t, float rate) {
	animate_f32_to_target(&(value->x), target.x, delta_t, rate);
	animate_f32_to_target(&(value->y), target.y, delta_t, rate);
}

// ^^^ generic utils


typedef struct Sprite {
	Gfx_Image* image;
} Sprite;

typedef enum SpriteID {
	SPRITE_nil,
	SPRITE_player,
	SPRITE_plastic_0,
	SPRITE_wood,
	SPRITE_item_plastic,
	SPRITE_item_wood,
	SPRITE_MAX,
} SpriteID;

Sprite sprites[SPRITE_MAX];

Sprite* get_sprite(SpriteID id) {
	if (id >= 0 && id < SPRITE_MAX){
		return &sprites[id];
	}
	return &sprites[0];
}

Vector2 get_sprite_size(Sprite* sprite){
	return (Vector2) {sprite->image->width, sprite->image->height};
}

typedef enum EntityArchetype {
	arch_nil = 0,
	arch_wood_tile = 1,
	arch_plastic_0 = 2,
	arch_player = 3,
	arch_wood = 4,

	arch_item_plastic = 5,
	arch_item_wood = 6,
	ARCH_MAX,
	
} EntityArchetype;

typedef struct Entity {
	bool is_valid;
	Vector2 pos;
	EntityArchetype arch;
	bool render_sprite;
	SpriteID sprite_id;
	int health;
	bool destructable;
	bool is_item;
} Entity;
# define MAX_ENTITY_COUNT 1024
// :entity

typedef struct ItemData{
	int amount;
} ItemData;

typedef struct World {
	Entity entities[MAX_ENTITY_COUNT];
	ItemData inventory_items[ARCH_MAX];
} World;

World* world;

typedef struct WorldFrame{
	Entity* selected_en;

} WorldFrame;
WorldFrame world_frame;

Entity* entity_create() {
	Entity* entity_found = 0;
	for (int i = 0; i < MAX_ENTITY_COUNT; i++){
		Entity* existing_entity = &world->entities[i];
		if (!existing_entity->is_valid){
			entity_found = existing_entity;
			break;
		}
	}
	
	assert(entity_found, "No more free entities! The entity array is full");
	entity_found->is_valid = true;
	return entity_found;
}

void entity_destroy(Entity* en) {
	memset(en, 0, sizeof(Entity));
}

void setup_player(Entity* en) {
	en->arch = arch_player;
	en->sprite_id = SPRITE_player;
	en->health = player_health;
}

void setup_plastic_0(Entity* en) {
	en->arch = arch_plastic_0;
	en->sprite_id = SPRITE_plastic_0;
	en->health = plastic_health;
	en->destructable = true;
}


void setup_wood(Entity* en) {
	en->arch = arch_wood;
	en->sprite_id = SPRITE_wood;
	en->health = wood_health;
	en->destructable = true;
}

void setup_item_plastic(Entity* en){
	en->arch = arch_item_plastic;
	en->sprite_id = SPRITE_item_plastic;
	en->is_item = true;
}

void setup_item_wood(Entity* en){
	en->arch = arch_item_wood;
	en->sprite_id = SPRITE_item_wood;
	en->is_item = true;
}

Vector2 screen_to_world() {
	float mouse_x = input_frame.mouse_x;
	float mouse_y = input_frame.mouse_y;
	Matrix4 proj = draw_frame.projection;
	Matrix4 view = draw_frame.view;
	float window_w = window.width;
	float window_h = window.height;

	// Normalize the mouse coordinates
	float ndc_x = (mouse_x / (window_w * 0.5f)) - 1.0f;
	float ndc_y = (mouse_y / (window_h * 0.5f)) - 1.0f;

	// Transform to world coordinates
	Vector4 world_pos = v4(ndc_x, ndc_y, 0, 1);
	world_pos = m4_transform(m4_inverse(proj), world_pos);
	world_pos = m4_transform(view, world_pos);
	// log("%f, %f", world_pos.x, world_pos.y);

	// Return as 2D vector
	return (Vector2){ world_pos.x, world_pos.y };
}


int entry(int argc, char **argv) {
	
	window.title = STR("nudelLand");
	window.scaled_width = 1280; // We need to set the scaled size if we want to handle system scaling (DPI)
	window.scaled_height = 720; 
	window.x = 200;
	window.y = 90;
	window.clear_color = hex_to_rgba(0x3978a8ff);

	world = alloc(get_heap_allocator(), sizeof(World));
	memset(world, 0, sizeof(World));
	
	// :sprites
	sprites[SPRITE_player] = (Sprite){ .image = load_image_from_disk(STR("resources/player.png"), get_heap_allocator())};
	sprites[SPRITE_plastic_0] = (Sprite){ .image = load_image_from_disk(STR("resources/plastic_0.png"), get_heap_allocator())};
	sprites[SPRITE_wood] = (Sprite){ .image = load_image_from_disk(STR("resources/wood.png"), get_heap_allocator())};
	sprites[SPRITE_item_plastic] = (Sprite){ .image = load_image_from_disk(STR("resources/item_plastic.png"), get_heap_allocator())};
	sprites[SPRITE_item_wood] = (Sprite){ .image = load_image_from_disk(STR("resources/item_wood.png"), get_heap_allocator())};

	// :font
	Gfx_Font *font = load_font_from_disk(STR("C:/windows/fonts/arial.ttf"), get_heap_allocator());
	assert(font, "Failed loading arial.ttf");
	
	const u32 font_height = 48;

	// :init

	// test item adding
	{
		world->inventory_items[arch_item_plastic].amount = 5; 
	}

	Entity* player_en = entity_create();
	setup_player(player_en);
	player_en->pos = v2(0.0, 0.0);

	for (int i = 0; i < 10; i++) {
		Entity* en = entity_create();
		setup_plastic_0(en);
		en->pos = v2(get_random_float32_in_range(-100.0, 100), get_random_float32_in_range(-100.0, 100.0));

	}


	float64 seconds_counter = 0.0;
	s32 frames_counter = 0;

	float zoom = 4; // 5.3
	Vector2 camera_pos = v2(0.0, 0.0);


	float64 last_time = os_get_current_time_in_seconds();
	while (!window.should_close) {
		reset_temporary_storage();
		world_frame = (WorldFrame){0};
		float64 now = os_get_current_time_in_seconds();
		float64 delta_t = now - last_time;
		last_time = now;
		
		os_update(); 
		draw_frame.projection = m4_make_orthographic_projection(window.width * -0.5, window.width * 0.5, window.height * -0.5, window.height * 0.5, -1, 10);

		// :camera
		{
			Vector2 target_pos = player_en->pos;
			animate_v2_to_target(&camera_pos, target_pos, delta_t, 10.0f);
			draw_frame.view = m4_make_scale(v3(1.0, 1.0, 1.0));
			draw_frame.view = m4_mul(draw_frame.view, m4_make_translation(v3(camera_pos.x, camera_pos.y, 0)));
			draw_frame.view = m4_mul(draw_frame.view, m4_make_scale(v3(1.0 / zoom, 1.0 / zoom, 1.0)));

		}
		
		// :mouse to world
		{

			float smallest_dist = INFINITY;

			Vector2 mouse_pos_world = screen_to_world();

			for (int i = 0; i < MAX_ENTITY_COUNT; i++){
				Entity* en = &world->entities[i];
				if(en->is_valid && en->destructable){
					Sprite* sprite = get_sprite(en->sprite_id);
					
						float dist = fabsf(v2_dist(en->pos, mouse_pos_world));
						if(dist < entity_selection_radius){
							if (!world_frame.selected_en || (dist < smallest_dist)){
								world_frame.selected_en = en;
								smallest_dist = dist;

							}
						}
						
					}



				}
			

		}


		// :tile rendering
		// float player_tile_x = world_to_tile_pos(player_en->pos.x);
		// float player_tile_y = world_to_tile_pos(player_en->pos.y);
		// const int tile_radius_x = 5;
		// const int tile_radius_y = 5;

		// for (int x = player_tile_x - tile_radius_x; x < player_tile_x + tile_radius_x; x++) {
		// 	for (int y = player_tile_y - tile_radius_y; y < player_tile_y + tile_radius_y; y++) {
		// 		if((x + (y % 2 == 0)) % 2 == 0){
		// 			float x_pos = x * tile_width;
		// 			float y_pos = y * tile_width;
		// 			draw_rect(v2(x_pos, y_pos), v2(tile_width, tile_width), COLOR_PURPLE);
		// 		}
		// 	}
		// }

		// :update entities
		{
			for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
				Entity* en = &world->entities[i];
				if (en->is_valid){
					
					// pick up item
					if (en->is_item){
						// TODO: - make physics for pick up
						if (fabsf(v2_dist(en->pos, player_en->pos)) < player_pickup_radius){
							world->inventory_items[en->arch].amount += 1;
							entity_destroy(en);
						}
					}
				}
			}
		}


		// :click-detection
		{
			Entity* selected_en = world_frame.selected_en;

			if (is_key_just_pressed(MOUSE_BUTTON_LEFT)){
				consume_key_just_pressed(MOUSE_BUTTON_LEFT);

				if (selected_en){
					selected_en->health -= 1;
					if (selected_en->health <= 0){
						
						// this will spawn an item at the current player position
						// TODO: when ineventory is added add it to the inventory instead of spawning it onto the player
						switch(selected_en->arch){
							case (arch_plastic_0):
							{
								Entity* en = entity_create();
								setup_item_plastic(en);
								en->pos = player_en->pos;
								break;
							}
							case (arch_wood):
							{
								Entity* en = entity_create();
								setup_item_wood(en);
								en->pos = player_en->pos;
								break;
							}
						}


						entity_destroy(selected_en);

					}

				}

			}
		}

		// :render
		for (int i = 0; i < MAX_ENTITY_COUNT; i++){
			Entity* en = &world->entities[i];
			if (en->is_valid) {

				switch (en->arch){

					default:
					{
						Sprite* sprite = get_sprite(en->sprite_id);
						Matrix4 xform = m4_scalar(1.0);
						if (en->is_item){
							xform = m4_translate(xform, v3(0, 2.0 * sin_breathe(os_get_current_time_in_seconds(), 3.0f), 0));

						}
						xform         = m4_translate(xform, v3(en->pos.x, en->pos.y, 0));
						xform         = m4_translate(xform, v3(get_sprite_size(sprite).x * -0.5, 0, 0));

						Vector4 color = COLOR_WHITE;
						if (world_frame.selected_en == en){
							color = COLOR_BLACK;
						}
						draw_image_xform(sprite->image, xform, get_sprite_size(sprite), color);
						// draw_text(font, sprint(temp_allocator, STR("%.2f, %.2f"), en->pos.x, en->pos.y), font_height * 0.90, en->pos, v2(0.1, 0.1), COLOR_GREEN);
						break;
					}
				}
			}
		}


		// :garbage floating
		for (int i = 0; i < MAX_ENTITY_COUNT; i++){
			Entity* en = &world->entities[i];

			if (is_key_down('H')) {

				if(en->arch == arch_plastic_0){
					en->pos.x += get_random_float32_in_range(-0.025, 0.1);
					en->pos.y -= get_random_float32_in_range(-0.025, 0.1);

					if(en->pos.x >= 300 || en->pos.y >= 300){
						en->pos = v2(get_random_float32_in_range(-500.0, -50), get_random_float32_in_range(200.0, 100.0));
					}
					

				}
			}
		}


		if (is_key_just_released(KEY_ESCAPE)) {
			window.should_close = true;
		}

		Vector2 input_axis = v2(0, 0);
		if (is_key_down('A')) {
			input_axis.x -= 1.0;
		}
		if (is_key_down('D')) {
			input_axis.x += 1.0;
		}
		if (is_key_down('S')) {
			input_axis.y -= 1.0;
		}
		if (is_key_down('W')) {
			input_axis.y += 1.0;
		}
		input_axis = v2_normalize(input_axis);

		player_en->pos = v2_add(player_en->pos, v2_mulf(input_axis, 150.0 * delta_t));


		// :scroll-zoom
		for (u64 i = 0; i < input_frame.number_of_events; i++) {
			Input_Event e = input_frame.events[i];
			switch (e.kind) {
				case (INPUT_EVENT_SCROLL):
				{				
					zoom += e.yscroll * 0.25;
					break;
				}
				case (INPUT_EVENT_KEY):
				{
					break;
				}   
				case (INPUT_EVENT_TEXT):
				{
					break;
				}
			}
		}
		
		gfx_update();

		seconds_counter += delta_t;
		frames_counter += 1;
		if (seconds_counter > 1.0){
			log("FPS: %i", frames_counter);
			seconds_counter = 0.0;
			frames_counter = 0;
		}

	}

	return 0;
}