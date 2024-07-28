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

	float64 last_time = os_get_current_time_in_seconds();
	while (!window.should_close) {
		reset_temporary_storage();
		
		draw_frame.projection = m4_make_orthographic_projection(window.width * -0.5, window.width * 0.5, window.height * -0.5, window.height * 0.5, -1, 10);

		float zoom = 5.3;
		draw_frame.view = m4_make_scale(v3(1 / zoom, 1 / zoom, 1));

		float64 now = os_get_current_time_in_seconds();
		float64 delta_t = now - last_time;
		last_time = now;
		
		os_update(); 

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
						break;
					}
				}
			}
		}

		for (int i = 0; i < MAX_ENTITY_COUNT; i++){
			Entity* en = &world->entities[i];

			if(en->arch == arch_plastic_0){
				en->pos.x += 0.001;
				en->pos.y += 0.001;
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

		
		// 	// wood_tile
		// {
		// 	Vector2 size = v2(16.0f, 16.0f);
		// 	Matrix4 xform = m4_scalar(1.0);
		// 	xform         = m4_translate(xform, v3(size.x * -0.5, 0, 0));
		// 	draw_image_xform(wood_tile, xform, size, COLOR_WHITE);
		// }

		
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