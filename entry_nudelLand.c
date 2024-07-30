const int tile_width = 16;

int world_to_tile_pos(float world_pos){
	return world_pos / (float) tile_width;
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

typedef struct Sprite {
	Gfx_Image* image;
	Vector2 size;
} Sprite;

typedef enum SpriteID {
	SPRITE_nil,
	SPRITE_player,
	SPRITE_plastic_0,
	SPRITE_wood,
	SPRITE_MAX,
} SpriteID;

Sprite sprites[SPRITE_MAX];

Sprite* get_sprite(SpriteID s_id) {
	if (s_id >= 0 && s_id < SPRITE_MAX){
		return &sprites[s_id];
	}
	return &sprites[0];
}

typedef enum EntityArchetype {
	arch_nil = 0,
	arch_wood_tile = 1,
	arch_plastic_0 = 2,
	arch_player = 3,
	arch_wood = 4,
	
} EntityArchetype;

typedef struct Entity {
	bool is_valid;
	Vector2 pos;
	EntityArchetype arch;

	bool render_sprite;
	SpriteID sprite_id;
} Entity;

# define MAX_ENTITY_COUNT 1024

typedef struct World {
	Entity entities[MAX_ENTITY_COUNT];
} World;

World* world = 0;


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

}

void setup_plastic_0(Entity* en) {
	en->arch = arch_plastic_0;
	en->sprite_id = SPRITE_plastic_0;
}

void setup_wood(Entity* en) {
	en->arch = arch_wood;
	en->sprite_id = SPRITE_wood;
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
	// loading sprites
	sprites[SPRITE_player] = (Sprite){ .image = load_image_from_disk(STR("player.png"), get_heap_allocator()), .size = v2(12.0f, 15.0f)};
	sprites[SPRITE_plastic_0] = (Sprite){ .image = load_image_from_disk(STR("plastic_0.png"), get_heap_allocator()), .size = v2(4.0f, 9.0f)};
	sprites[SPRITE_wood] = (Sprite){ .image = load_image_from_disk(STR("wood.png"), get_heap_allocator()), .size = v2(4.0f, 9.0f)};

	// :font
	Gfx_Font *font = load_font_from_disk(STR("C:/windows/fonts/arial.ttf"), get_heap_allocator());
	assert(font, "Failed loading arial.ttf");
	
	const u32 font_height = 48;


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

	float zoom = 2.7; // 5.3
	Vector2 camera_pos = v2(0.0, 0.0);


	float64 last_time = os_get_current_time_in_seconds();
	while (!window.should_close) {
		reset_temporary_storage();
	
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
			Vector2 mouse_pos = screen_to_world();

			for (int i = 0; i < MAX_ENTITY_COUNT; i++){
				Entity* en = &world->entities[i];
				if(en->is_valid){
					Sprite* sprite = get_sprite(en->sprite_id);
					Range2f bounds = range2f_make_bottom_center(sprite->size);
					bounds = range2f_shift(bounds, en->pos);

					Vector4 col = COLOR_RED;
					col.a = 0.4;
					if(range2f_contains(bounds, mouse_pos)){
						col.a = 0.8;
					}
					draw_rect(bounds.min, range2f_size(bounds), col);

				}
			}

		}


		// :tiles
		float player_tile_x = world_to_tile_pos(player_en->pos.x);
		float player_tile_y = world_to_tile_pos(player_en->pos.y);
		const int tile_radius_x = 5;
		const int tile_radius_y = 5;

		for (int x = player_tile_x - tile_radius_x; x < player_tile_x + tile_radius_x; x++) {
			for (int y = player_tile_y - tile_radius_y; y < player_tile_y + tile_radius_y; y++) {
				if((x + (y % 2 == 0)) % 2 == 0){
					float x_pos = x * tile_width;
					float y_pos = y * tile_width;
					draw_rect(v2(x_pos, y_pos), v2(tile_width, tile_width), COLOR_PURPLE);
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
						xform         = m4_translate(xform, v3(en->pos.x, en->pos.y, 0));
						xform         = m4_translate(xform, v3(sprite->size.x * -0.5, 0, 0));
						draw_image_xform(sprite->image, xform, sprite->size, COLOR_WHITE);

						draw_text(font, sprint(temp, STR("%.2f, %.2f"), en->pos.x, en->pos.y), font_height * 0.90, en->pos, v2(0.1, 0.1), COLOR_GREEN);
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