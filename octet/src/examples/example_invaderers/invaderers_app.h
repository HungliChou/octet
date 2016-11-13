////////////////////////////////////////////////////////////////////////////////
//
// (C) Andy Thomason 2012-2014
//
// Modular Framework for OpenGLES2 rendering on multiple platforms.
//
// invaderer example: simple game with sprites and sounds
//
// Level: 3
//
// Demonstrates:
//   Basic framework app
//   Shaders
//   Basic Matrices
//   Simple game mechanics
//   Texture loaded from GIF file
//   Audio
//

#include <fstream>
#include <vector>

using namespace std;

namespace octet {
	class sprite {
		// where is our sprite (overkill for a 2D game!)
		mat4t modelToWorld;

		// half the width of the sprite
		float halfWidth;

		// half the height of the sprite
		float halfHeight;

		// what texture is on our sprite
		int texture;

		// true if this sprite is enabled.
		bool enabled;

		//change this value to control uv
		float uv_w;
		float uv_h;
		

	public:
		sprite() {
			texture = 0;
			enabled = true;
			uv_w = 1;
			uv_h = 1;
		}

		void init(int _texture, float x, float y, float w, float h) {
			modelToWorld.loadIdentity();
			modelToWorld.translate(x, y, 0);
			halfWidth = w * 0.5f;
			halfHeight = h * 0.5f;
			texture = _texture;
			enabled = true;
		}

		void set_size(float w, float h)
		{
			halfWidth = w;
			halfHeight = h;
		}

		void set_texture(int _texture)
		{
			texture = _texture;
		}

		float &get_uv_w() { return uv_w; }
		float &get_uv_h() { return uv_h; }

		void render(texture_shader &shader, mat4t &cameraToWorld, vec4 color) {
			// invisible sprite... used for gameplay.
			if (!texture) return;

			// build a projection matrix: model -> world -> camera -> projection
			// the projection space is the cube -1 <= x/w, y/w, z/w <= 1
			mat4t modelToProjection = mat4t::build_projection_matrix(modelToWorld, cameraToWorld);
			
			// set up opengl to draw textured triangles using sampler 0 (GL_TEXTURE0)
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texture);
			//printf("%f", color[1]);
			// use "old skool" rendering
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			shader.render(modelToProjection, 0, color);

			// this is an array of the positions of the corners of the sprite in 3D
			// a straight "float" here means this array is being generated here at runtime.
			float vertices[] = {
				-halfWidth , -halfHeight, 0,
				halfWidth, -halfHeight, 0,
				halfWidth,  halfHeight, 0,
				-halfWidth,  halfHeight, 0,
			};

			

			// attribute_pos (=0) is position of each corner
			// each corner has 3 floats (x, y, z)
			// there is no gap between the 3 floats and hence the stride is 3*sizeof(float)
			glVertexAttribPointer(attribute_pos, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)vertices);
			glEnableVertexAttribArray(attribute_pos);

			
			/*static const float uvs[] = {
				0,  0,
				1,  0,
				1,  1,
				0,  1,
			};*/

			// this is an array of the positions of the corners of the texture in 2D
			float uvv[] =
			{
				0, 0,
				uv_w,  0,
				uv_w,  uv_h,
				0,  uv_h,
			};

			// attribute_uv is position in the texture of each corner
			// each corner (vertex) has 2 floats (x, y)
			// there is no gap between the 2 floats and hence the stride is 2*sizeof(float)
			glVertexAttribPointer(attribute_uv, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)uvv);
			glEnableVertexAttribArray(attribute_uv);

			// finally, draw the sprite (4 vertices)
			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		}

		// move the object
		void translate(float x, float y) {
			modelToWorld.translate(x, y, 0);
		}

		void toPos(float x, float y, float w, float h)
		{
			modelToWorld.loadIdentity();
			modelToWorld.translate(x, y, 0);
			halfWidth = w ;
			halfHeight = h;
		}

		mat4t get_position() { return modelToWorld; }

		// position the object relative to another.
		void set_relative(sprite &rhs, float x, float y) {
			modelToWorld = rhs.modelToWorld;
			modelToWorld.translate(x, y, 0);
		}

		// return true if this sprite collides with another.
		// note the "const"s which say we do not modify either sprite
		bool collides_with(const sprite &rhs) const {
			float dx = rhs.modelToWorld[3][0] - modelToWorld[3][0];
			float dy = rhs.modelToWorld[3][1] - modelToWorld[3][1];

			// both distances have to be under the sum of the halfwidths
			// for a collision
			return
				(fabsf(dx) < halfWidth + rhs.halfWidth) &&
				(fabsf(dy) < halfHeight + rhs.halfHeight)
				;
		}

		bool is_above(const sprite &rhs, float margin) const {
			float dx = rhs.modelToWorld[3][0] - modelToWorld[3][0];

			return
				(fabsf(dx) < halfWidth + margin)
				;
		}

		bool &is_enabled() {
			return enabled;
		}
	};

	class weapon : public sprite {
		//the type of weapon
		int type;

		//the power of weapon
		int power;

		//disable time of weapon
		int disable_time;

		//how many time we can use this weapon(0->unlimited,>0->limited)
		int energy;

		//how many time we can use this weapon(0->unlimited,>0->limited)
		int cur_energy;
		
	public:
		weapon() { type = 1; power = 1; disable_time = 5; cur_energy=-1; energy = -1; }
		weapon(int _type, int _power, int _disable_time, int _energy) { type = _type; power = _power;  cur_energy = _energy; disable_time = _disable_time; energy = _energy; }
		int &get_type() { return type; }
		int &get_power() { return power; }
		int &get_cur_energy() { return cur_energy; }
		int &get_disable_time() { return disable_time; }
		int &get_energy() { return energy; }
		float get_energy_rate() { return (float)cur_energy / energy; }
	};

	class explosion : public sprite {
		int disable_timer;

	public:
		explosion() { disable_timer = 5; }
		int &get_disable() { return disable_timer; }
	};

	class condition : public sprite {
		//the type of condition
		int type;
		//the energy of condition
		int energy;

	public:
		condition() { type = 0; energy = 0; }
		condition(int _type, int _energy) { type = _type; energy = _energy; }
		int &get_type() { return type; }
		int &get_energy() { return energy; }
	};

	class icon : public sprite {
		//the type of icon
		int type;

	public:
		icon() { type = 0; }
		icon(int _type) { type = _type; }
		int &get_type() { return type; }
	};

	class enemy : public sprite {
		//invader max lives
		int max_lives;

		// invader current lives
		int cur_lives;

		// invader power
		int power;

		// invader level
		int level;

	public:
		enemy() { max_lives = 1; cur_lives = 1; level = 1; power = 1; }
		int &get_level() { return level; }
		int get_lives_rate() { return cur_lives * 100 / max_lives; }
		int &get_lives() { return cur_lives; }
		int &get_power() { return power; }
		void set_level(int _level, int _lives, int _power) { max_lives = _lives; cur_lives = _lives; level = _level; power = _power; }
	};

	class player : public sprite {
		int max_lives;
		int cur_lives;
		int hit_timer;
		int accumulated_hurt;

		weapon my_weapon;
		vector<condition> my_condition;
	public:
		player() { cur_lives = 100; max_lives = 100; accumulated_hurt = 0; }
		player(int _lives) { cur_lives = _lives; max_lives = _lives; }

		int &get_max_lives() { return max_lives; }

		//return lives
		int &get_lives() { return cur_lives; }

		//return accumulated damage to enemy value
		int &get_accumulated_hurt() { return accumulated_hurt; }

		////return percentage of health bar size
		float get_lives_rate_bar() { return (max_lives-(float)accumulated_hurt/2) / (float)max_lives; }

		//return percentage of lives player left
		float get_lives_rate() { return cur_lives / (float)max_lives; }

		//return hit time
		int &get_hit_timer() { return hit_timer; }

		//return weapon type
		weapon &get_weapon() { return my_weapon; }

		//return condition vector
		vector<condition> &get_condition(){ return my_condition; }
	};

	class invaderers_app : public octet::app {
		// Matrix to transform points in our camera space to the world.
		// This lets us move our camera
		mat4t cameraToWorld;

		// shader to draw a textured triangle
		texture_shader texture_shader_;

		enum {
			num_sound_sources = 8,
			num_rows = 4,
			num_cols = 5,
			num_conditions = 2,
			num_icon = 5,
			num_missiles = 5,
			num_bombs = 3,
			num_borders = 5,
			num_invaderers = num_rows * num_cols,
			num_max_stage = 3,

			bar_halfwidth = 3,

			//texture searching type
			type_invader = 1,
			type_weapon,
			type_icon,
			type_player,
			type_bomb,

			//man texture 
			tex_man = 0,
			tex_pika,
			tex_pika_a,

			//invaders sprite definitions
			first_invaderer_sprite = 0,
			last_invaderer_sprite = first_invaderer_sprite + num_invaderers - 1,

			//weapon sprite definitions
			first_missile_sprite = 0,
			last_missile_sprite = first_missile_sprite + num_missiles - 1,

			first_bomb_sprite,
			last_bomb_sprite = first_bomb_sprite + num_bombs - 1,
			num_weapon_sprites,

			//icon sprite definitions
			first_icon_sprite = 0,
			last_icon_sprite = first_icon_sprite + num_icon - 1,

			//icon category recognition definitions
			icon_type_weapon = 0,
			icon_type_condition,

			//condition sprite definitions
			first_condition_sprtie = 0,
			last_condition_sprite = first_condition_sprtie + num_conditions - 1,

			//condition definitions
			condition_slow = 0,
			condition_heart,

			//icon definitions
			icon_ball = 0,
			icon_lightning,
			icon_star,
			icon_slow,
			icon_heart,
			num_weapon = 3,
			
			//bar sprite definitions
			bar_base = 0,
			bar_value,
			num_bar_sprite,

			// sprite definitions
			game_over_sprite = 0,

			first_border_sprite,
			last_border_sprite = first_border_sprite + num_borders - 1,
			background,
			num_sprites,

		};

		// timers for missiles and bombs
		int missiles_disabled;
		int bombs_disabled;
		int icon_disabled;
		// accounting for bad guys
		int live_invaderers;

		// game state
		int stage;
		int score;
		bool game_over;
		bool is_invader_slow;
		bool is_invader_rage;

		// speed of enemy
		float regular_invader_velocity;
		float invader_velocity;
		float bomb_speed;

		//
		int man_texture;

		//count Y for the ship
		float upY;

		float lightning_upY;

		//default color float array
		vec4 default_color = { 1,1,1,1 };
		vec4 background_color = { 0.8f,0.98f,0.8f,1 };
		vec4 man_color = { 1,1,1,1 };

		// sounds
		ALuint whoosh;
		ALuint bang;
		unsigned cur_source;
		ALuint sources[num_sound_sources];

		// big array of sprites
		sprite sprites[num_sprites];
		// big array of invader sprites
		enemy invader_sprite[num_invaderers];
		// big array of explosion sprites
		explosion explosion_sprite[num_invaderers];
		// big array of weapon sprites
		weapon weapon_sprite[num_weapon_sprites];
		// big array of icon sprites
		icon icon_sprite[num_icon];
		// big array of condition sprites
		condition condition_sprite[num_conditions];
		// player sprite
		player man;
		// weapon energy bar sprite
		sprite energy_bar[num_bar_sprite];
		// health bar sprite
		sprite health_bar[num_bar_sprite];

		// random number generator
		class random randomizer;

		// a texture for our text
		GLuint font_texture;

		// information for our text
		bitmap_font font;

		ALuint get_sound_source() { return sources[cur_source++ % num_sound_sources]; }

		//data vectors (store values loaded from files)
		vector<float> enemy_w;
		vector<float> enemy_h;
		vector<int> enemy_power;
		vector<int> enemy_lives;
		vector<int> weapon_power;
		vector<int> weapon_disable_timer;
		vector<int> weapon_energy;
		vector<int> condition_energy;

		//load game data from csv and txt files.
		void load_data() {
			//load enemy ability to vector
			std::ifstream is("enemy.csv");
			if (is.good())
			{
				// store the line here
				char buffer[2048];
				int type = 0;
				// loop over lines
				while (!is.eof()) {
					is.getline(buffer,sizeof(buffer));
					if (*buffer != '-')
					{
						if (type == 0)
						{
							enemy_w.push_back(atof(buffer));
							type++;
						}
						else if (type == 1)
						{
							enemy_h.push_back(atof(buffer));
							type++;
						}
						else if (type == 2)
						{
							enemy_lives.push_back(atoi(buffer));
							type++;
						}
						else
						{
							enemy_power.push_back(atoi(buffer));
							type = 0;
						}
					}	
				}
			}

			//load icon(weapon and condition) ability to vector
			std::ifstream is2("icon.txt");
			if (is2.good())
			{
				// store the line here
				char buffer[2048];
				//data type
				int type = 0;
				//weapon->0 or conditio->1
				int format = 0;
				// loop over lines
				while (!is2.eof()) {
					is2.getline(buffer, sizeof(buffer));
					if (format == 0)
					{
						if (*buffer != '-' && *buffer != '+')
						{
							if (type == 0)
							{
								weapon_power.push_back(atoi(buffer));
								type++;
							}
							else if (type == 1)
							{
								weapon_disable_timer.push_back(atoi(buffer));
								type++;
							}
							else if (type == 2)
							{
								weapon_energy.push_back(atoi(buffer));
								type = 0;
							}
						}
						else if (*buffer == '+')
						{
							format++;
						}
					}
					else
					{
						if (*buffer != '+')
						{
							condition_energy.push_back(atoi(buffer));
						}
					}
					
				}
			}

		}

		//generating new set of enemies for the new stage
		void generate_invaders()
		{
			GLuint texture = resource_dict::get_texture_handle(GL_RGBA, texture_position(stage, type_invader));

			//change background color
			if (stage == 2)
			{
				background_color = vec4(0.69f, 0.765f, 0.867f, 1);
			}
			else if (stage == 3)
			{
				background_color = vec4(0.253f, 0.41f, 0.787f, 1);
			}

			//move enemy and set their abilities according to the data vectors loaded from csv file
			for (int j = 0; j != num_rows; ++j) {
				for (int i = 0; i != num_cols; ++i) {
					assert(first_invaderer_sprite + i + j*num_cols <= last_invaderer_sprite);
					invader_sprite[first_invaderer_sprite + i + j*num_cols].init(
						texture, ((float)i - num_cols * 0.5f) * 0.5f, 2.0f - ((float)j * 0.5f), enemy_w[stage-1], enemy_h[stage - 1]
					);
					invader_sprite[first_invaderer_sprite + i + j*num_cols].set_level(stage,enemy_lives[stage-1],enemy_power[stage-1]);
				}
			}

			//change bomb image
			texture = resource_dict::get_texture_handle(GL_RGBA, texture_position(stage, type_bomb));
			for (int i = 0; i != num_bombs; ++i) {
				weapon_sprite[first_bomb_sprite + i].set_texture(texture);
			}
		}

		//set the new stage for the game
		void set_next_stage()
		{
			stage++;

			//reset enemy condition
			is_invader_rage = false;
			if (is_invader_slow)
				invader_velocity /= 4;
			else
				invader_velocity = regular_invader_velocity;
			
			live_invaderers = num_invaderers;

			//generate new invaders
			generate_invaders();
		}

		//find the texture path in the asset folder
		char* texture_position(int value, int type)
		{
			char *path = "assets/invaderers/invader1_1.gif";
			if (type == type_invader)
			{
				switch (stage)
				{
				case 1:
					if (value == 1)
						path = "assets/invaderers/invader1_1.gif";
					break;
				case 2:
					if (value == 1)
						path = "assets/invaderers/invader2_1.gif";
					else if (value == 2)
						path = "assets/invaderers/invader2_2.gif";
					break;
				case 3:
					if (value == 1)
						path = "assets/invaderers/invader3_1.gif";
					else if (value == 2)
						path = "assets/invaderers/invader3_2.gif";
					else if (value == 3)
						path = "assets/invaderers/invader3_3.gif";
					break;
				default:
					break;
				}
				//path = path_array;
			}
			else if (type == type_weapon)
			{
				switch (value)
				{
				case icon_lightning:
					path = "assets/invaderers/lightning.gif";
					break;
				default:
					path = "assets/invaderers/ball.gif";
					break;
				}
			}
			else if (type == type_icon)
			{
				switch (value)
				{
				case icon_lightning:
					path = "assets/invaderers/icon_lightning.gif";
					break;
				case icon_slow:
					path = "assets/invaderers/icon_slow.gif";
					break;
				case icon_star:
					path = "assets/invaderers/icon_star.gif";
					break;
				case icon_heart:
					path = "assets/invaderers/icon_heart.gif";
					break;
				default:
					path = "assets/invaderers/ball.gif";
					break;
				}
			}
			else if (type == type_player)
			{
				switch (value) {
				case 1:
					path = "assets/invaderers/pika.gif";
					break;
				case 2:
					path = "assets/invaderers/pika_a.gif";
					break;
				default:
					path = "assets/invaderers/ash.gif";
					break;
				}
			}
			else if (type == type_bomb)
			{
				switch (value) {
				case 1:
					path = "assets/invaderers/weapon_lightning.gif";
					break;
				case 2:
					path = "assets/invaderers/weapon_goast.gif";
					break;
				case 3:
					path = "assets/invaderers/weapon_turnado.gif"; 
					break;
				}
			}
			return path;
		}
		
		// called when missile hit an enemy
		void on_hit_invaderer(enemy &invaderer) {
			GLuint newtexture;
			//calculate damage
			int power = man.get_weapon().get_power() + (int)(man.get_weapon().get_power()*(stage - 1));
			int lives = invaderer.get_lives() - power;
			int level = invaderer.get_level();

			//sound effect
			ALuint source = get_sound_source();
			alSourcei(source, AL_BUFFER, bang);
			alSourcePlay(source);

			//if enemy got killed
			if (lives <= 0)
			{
				//create special weapon icon if icon_timer is open(icon_disabled == 0)
				if (icon_disabled == 0)
				{
					for (int i = 0; i != num_icon; ++i) {
						if (!icon_sprite[first_icon_sprite + i].is_enabled()) {
							int random_weapon = randomizer.get(1, num_icon);
							newtexture = resource_dict::get_texture_handle(GL_RGBA, texture_position(random_weapon, type_icon));
							icon_sprite[first_icon_sprite + i].set_relative(invaderer, 0, 0);
							icon_sprite[first_icon_sprite + i].get_type() = random_weapon;
							icon_sprite[first_icon_sprite + i].is_enabled() = true;
							icon_sprite[first_icon_sprite + i].set_texture(newtexture);
							icon_disabled = 0;
							break;
						}
					}

					//kill enemy
					invaderer.is_enabled() = false;
					invaderer.translate(20, 0);
					live_invaderers--;

					//get score
					score = score + 10 * level*level;

					//TODO sound effect of dropping icon
					/*ALuint source = get_sound_source();
					alSourcei(source, AL_BUFFER, bang);
					alSourcePlay(source);*/
				}
			}
			else
			{
				//not get killed - got damage
				invaderer.get_lives() = lives;
			}

			check_enemy_condition();	
		}

		//check the number of enemies to set game status
		void check_enemy_condition()
		{
			//if number <4, enemy will be raged
			if (live_invaderers == 4) {
				if (!is_invader_rage)
				{
					invader_velocity *= 4;
					is_invader_rage = true;
				}

			}
			//if number = 0, go to next stage or get victory(pass all stages)
			else if (live_invaderers == 0) {
				printf("stage:%d", stage);
				if (stage<num_max_stage)
					set_next_stage();
				else
				{
					show_game_over(true);
				}
			}
		}

		//showing game is over
		void show_game_over(bool pass)
		{
			if(!pass)
				sprites[game_over_sprite].translate(-20, 0);
			game_over = true;
		}

		// called when we are hit
		void on_hit_ship(int damage) {
			//sound effect
			ALuint source = get_sound_source();
			alSourcei(source, AL_BUFFER, bang);
			alSourcePlay(source);

			//calculate health of player
			int cur_lives = man.get_lives();
			cur_lives -= damage;
			man.get_accumulated_hurt() += damage;
			man.get_lives() = cur_lives;


			//set hit timer()
			man.get_hit_timer() = 5;
			man_color = vec4(1, 0, 0, 1);

			//set game status if they are dead
			if (cur_lives <= 0) {
				man.get_lives() = 0;
				show_game_over(false);
			}
		}

		// use the keyboard to move the ship
		void move_player() {
			// player moving speed
			const float speed = 0.1f;

			// left and right arrows
			if (is_key_down(key_left)) {
				man.translate(-speed, 0);
				if (man.collides_with(sprites[first_border_sprite + 2])) {
					man.translate(+speed, 0);
				}
			}
			else if (is_key_down(key_right)) {
				man.translate(+speed, 0);
				if (man.collides_with(sprites[first_border_sprite + 3])) {
					man.translate(-speed, 0);
				}
			}

			// up and down arrows(move up and down)
			// give limit of key_up
			else if (is_key_down(key_up)) {

				if (man.is_enabled() && upY<1.7f)
				{
					man.translate(0, +speed);
					if (man.collides_with(sprites[first_border_sprite + 1])) {
						man.translate(0, -speed);
					}
					else
						upY += speed;
				}

			}
			else if (is_key_down(key_down)) {
				man.translate(0, -speed);
				if (man.collides_with(sprites[first_border_sprite])) {
					man.translate(0, +speed);
				}
				else
					upY -= speed;
			}
		}

		// fire button (space)
		void fire_missiles() {
			if (man.get_weapon().get_type() != icon_ball)
			{
				// timer will count down for special weapons
				if (man.get_weapon().get_cur_energy() > 0)
					man.get_weapon().get_cur_energy()--;
				else
				{
					// change weapon to normal ball
					on_icon_effect(icon_ball);
				}
			}

			// missile fire timer
			if (missiles_disabled) {
				--missiles_disabled;
				return;
			}

			// temporary texture value for player sprite
			int temp_texture = tex_man;
			// check if the texture needs to be changed
			bool change_texture = false;

			//if weapon is lightning
			if (man.get_weapon().get_type() == icon_lightning)
			{
				//press key(space) down
				if (is_key_down(' '))
				{
					//if current texture is not pika_a -> change it
					if (man_texture != tex_pika_a)
					{
						temp_texture = tex_pika_a;
						change_texture = true;
					}
					
					//choose missile to fire
					for (int i = 0; i < num_missiles; i++)
					{
						if (!weapon_sprite[first_missile_sprite + i].is_enabled())
						{
							weapon_sprite[first_missile_sprite + i].set_relative(man, 0, 0.7f);
							weapon_sprite[first_missile_sprite + i].is_enabled() = true;
							missiles_disabled = man.get_weapon().get_disable_time();
							goto next;
						}
					}
				next:;
				}
				else//key up
				{
					//change player texture to normal pika
					if (man_texture != tex_pika)
					{
						temp_texture = tex_pika;
						change_texture = true;
					}
					
				}
			}
			else
			{
				//if current texture is not man(ash) -> change it
				if (man_texture != tex_man)
				{
					temp_texture = tex_man;
					change_texture = true;
				}

				/*if (missiles_disabled) {
					--missiles_disabled;
				}
				else */
				// find a missile
				if (is_key_down(' ')) {
					for (int i = 0; i != num_missiles; ++i) {
						if (!weapon_sprite[first_missile_sprite + i].is_enabled()) {
							weapon_sprite[first_missile_sprite + i].set_relative(man, 0, 0);
							weapon_sprite[first_missile_sprite + i].is_enabled() = true;
							missiles_disabled = man.get_weapon().get_disable_time();
							ALuint source = get_sound_source();
							alSourcei(source, AL_BUFFER, whoosh);
							alSourcePlay(source);
							break;
						}
					}
				}
			}

			//if texture needs to be change -> change it and set it to be the current texture for player sprite
			if (change_texture)
			{
				GLuint texture = resource_dict::get_texture_handle(GL_RGBA, texture_position(temp_texture, type_player));
				man.set_texture(texture);
				man_texture = temp_texture;
			}
		}

		// pick and invader and fire a bomb
		void fire_bombs() {
			//count down to allow bomb firing
			if (bombs_disabled) {
				--bombs_disabled;
			}
			else {
				// find an invaderer
				for (int j = randomizer.get(0, num_invaderers); j < num_invaderers; ++j) {
					enemy &invaderer = invader_sprite[first_invaderer_sprite + j];
					if (invaderer.is_enabled() && invaderer.is_above(man, 0.3f)) {
						// find a bomb
						for (int i = 0; i != num_bombs; ++i) {
							if (!weapon_sprite[first_bomb_sprite + i].is_enabled()) {
								weapon_sprite[first_bomb_sprite + i].set_relative(invaderer, 0, -0.25f);
								weapon_sprite[first_bomb_sprite + i].is_enabled() = true;
								weapon_sprite[first_bomb_sprite + i].get_power() = invaderer.get_power();
								bombs_disabled = 30;
								ALuint source = get_sound_source();
								alSourcei(source, AL_BUFFER, whoosh);
								alSourcePlay(source);
								return;
							}
						}
						return;
					}
				}
			}
		}

		// animate the missiles
		void move_missiles() {
			//missile speed
			const float missile_speed = 0.4f;
			//offset x value for weapon lightning (since it should follow the player x position)
			float moveX = 0;
			// current weapon type
			int type;

			//find the current missiles
			for (int i = 0; i != num_missiles; ++i) {
				weapon &missile = weapon_sprite[first_missile_sprite + i];
				if (missile.is_enabled()) {
					type = man.get_weapon().get_type();

					//lightning weapon should follow the player
					if (type == icon_lightning)
						moveX = man.get_position().w().x() - missile.get_position().w().x();

					//move missile	
					missile.translate(moveX, missile_speed);

					//find invader
					for (int j = 0; j != num_invaderers; ++j) {
						enemy &invaderer = invader_sprite[first_invaderer_sprite + j];
						if (invaderer.is_enabled() && missile.collides_with(invaderer)) {
							if (type != icon_lightning)
							{
								missile.is_enabled() = false;
								missile.translate(20, 0);
							}
							
							//move explosion sprite to the enemy position
							explosion &expl_sprite = explosion_sprite[first_invaderer_sprite + j];
							expl_sprite.set_relative(invaderer, 0, -0.25f);
							expl_sprite.get_disable() = 3;
							expl_sprite.is_enabled() = true;

							//determine damage
							on_hit_invaderer(invaderer);

							goto next_missile;
						}
					}


					//destroy missiles
					if ((type!=icon_lightning && missile.collides_with(sprites[first_border_sprite + 1]))
					 || (type == icon_lightning && missile.collides_with(sprites[first_border_sprite + 4]))) 
					{
						missile.is_enabled() = false;
						missile.translate(20, 0);
					}
				}
			next_missile:;
			}
		}

		// animate the bombs
		void move_bombs() {
			for (int i = 0; i != num_bombs; ++i) {
				weapon &bomb = weapon_sprite[first_bomb_sprite + i];
				if (bomb.is_enabled()) {
					bomb.translate(0, -bomb_speed);
					if (bomb.collides_with(man)) {
						bomb.is_enabled() = false;
						bomb.translate(20, 0);
						bombs_disabled = 30;
						on_hit_ship(bomb.get_power());
						goto next_bomb;
					}
					if (bomb.collides_with(sprites[first_border_sprite + 0])) {
						bomb.is_enabled() = false;
						bomb.translate(20, 0);
					}
				}
			next_bomb:;
			}
		}

		// move the array of enemies
		void move_invaders(float dx, float dy) {
			for (int j = 0; j != num_invaderers; ++j) {
				enemy &invaderer = invader_sprite[first_invaderer_sprite + j];
				if (invaderer.is_enabled()) {
					invaderer.translate(dx, dy);
				}
			}
		}

		//move all icons
		void move_icon() {
			//icon dropping timer
			if (icon_disabled > 0)
				icon_disabled--;
			//dropping speed
			const float icon_speed = 0.05f;
			//find the current dropping icon to move
			for (int j = 0; j != num_icon; ++j) {
				icon &icon = icon_sprite[first_icon_sprite + j];
				if (icon.is_enabled()) {
					icon.translate(0, -icon_speed);
					//check collision with player
					if (icon.collides_with(man)) {
						icon.is_enabled() = false;
						icon.translate(20, 0);
						on_icon_effect(icon.get_type());
						goto next_icon;
					}
					//destroy icon
					if (icon.collides_with(sprites[first_border_sprite + 0])) {
						icon.is_enabled() = false;
						icon.translate(20, 0);
					}
				}
			next_icon:;
			}
		}

		// check if any invaders hit the sides.
		bool invaders_collide(sprite &border) {
			for (int j = 0; j != num_invaderers; ++j) {
				enemy &invaderer = invader_sprite[first_invaderer_sprite + j];
				if (invaderer.is_enabled() && invaderer.collides_with(border)) {
					return true;
				}
			}
			return false;
		}

		// Slow(condition): can slow invaders for a while
		void slow_invader(bool is_slow)
		{
			if (is_slow)
			{
				if (!is_invader_slow)
				{
					invader_velocity = invader_velocity / 3;
					bomb_speed = bomb_speed / 3;
				}
			}
			else
			{
				if (is_invader_slow)
				{
					invader_velocity = invader_velocity * 3;
					bomb_speed = bomb_speed * 3;
				}
			}
			is_invader_slow = is_slow;
		}

		//set received icon effect to the current weapon or condition 
		void on_icon_effect(int icon_type)
		{
			//local temporary variables of condition, weapon and texture
			condition new_condition;
			weapon new_weapon;
			GLuint new_texture;

			//defualt size of weapon or condition sprite
			float w = 0.4f;
			float h = 0.4f;

			//setting specific variables or function calling
			switch (icon_type) {
			case icon_lightning:
				w = 0.5f;
				h = 1.8f;
				printf("lightning!!!");
				break;
			case icon_slow:
				slow_invader(true);
				printf("slow defender");
				break;
			case icon_star:
				printf("star motion");
				break;
			case icon_ball:
				printf("become normal");
				break;
			case icon_heart:
				printf("health recovery");
				break;
			default:
				break;
			}

			//player gets the weapon icon
			if (icon_type < num_weapon)
			{
				//set new weapon data to current player weapon
				man.get_weapon() = weapon(icon_type, weapon_power[icon_type], weapon_disable_timer[icon_type], weapon_energy[icon_type]);
				missiles_disabled = weapon_disable_timer[icon_type];
				new_texture = resource_dict::get_texture_handle(GL_RGBA, texture_position(icon_type, type_weapon));
				for (int i = 0; i != num_missiles; ++i) {
					weapon_sprite[first_missile_sprite + i].init(new_texture, 50, 0, w, h);
					weapon_sprite[first_missile_sprite + i].is_enabled() = false;
				}
			}
			else//player gets the condition icon
			{
				//set new buff condition data to current player condition
				int condition_type = icon_type - num_weapon;
				new_condition = condition(condition_type, condition_energy[condition_type]);
				condition &_condition_sprite = condition_sprite[condition_type];
				new_texture = resource_dict::get_texture_handle(GL_RGBA, texture_position(icon_type, type_icon));
				_condition_sprite.set_texture(new_texture);

				//showing buff condition sprite
				if (!_condition_sprite.is_enabled()) {
					_condition_sprite.translate(-22.5f + condition_type, 0);
					_condition_sprite.is_enabled() = true;
				}

				//give data to condition sprite
				_condition_sprite.get_type() = condition_type;
				_condition_sprite.get_energy() = condition_energy[condition_type];
			}
		}

		//checking the status of all buff conditions
		void checking_conditions()
		{
			for (int type = 0; type < num_conditions; type++)
			{
				if (condition_sprite[type].is_enabled())
				{
					//if condition is "on"
					if (condition_sprite[type].get_energy() > 0)
					{
						switch (type)
						{
							//healing over time
							case condition_heart:
								if (condition_sprite[type].get_energy() % 30 == 0)
								{
									man.get_lives() += man.get_max_lives() / 10;
									if (man.get_lives() > man.get_max_lives())
										man.get_lives() = man.get_max_lives();
								}		
								break;
							default:
								break;
						}
						condition_sprite[type].get_energy()--;
					}
					//if condition should be "off"	
					else
					{
						condition_sprite[type].translate(22.5f - type, 0);
						condition_sprite[type].is_enabled() = false;
						switch (type)
						{
						case condition_slow:
							slow_invader(false);
							break;
						default:
							break;
						}
					}
				}	
			}
		}

		//let every explosion show a period of time(like a second), then close it
		void checking_explosion_and_hit()
		{
			//find all explosion sprite, and check if it should be turned off.
			for (int i = 0; i < num_invaderers; i++)
			{
				if (explosion_sprite[first_invaderer_sprite + i].is_enabled())
				{
					if (explosion_sprite[first_invaderer_sprite + i].get_disable() > 0)
					{
						explosion_sprite[first_invaderer_sprite + i].set_relative(invader_sprite[first_invaderer_sprite + i], 0, -0.25f);
						explosion_sprite[first_invaderer_sprite + i].get_disable()--;
					}
						
					else
					{
						explosion_sprite[first_invaderer_sprite + i].translate(20, 0);
						explosion_sprite[first_invaderer_sprite + i].is_enabled() = false;
					}
				}
			}

			//check the hit timer (for red color) if the sprite should be turned normal color
			if (man.get_hit_timer() > 0)
			{
				man.get_hit_timer()--;
			}
			else
			{
				man_color = vec4(1, 1, 1, 1);
			}

		}

		//updating health and energy bar sprites based on their actual values.
		void checking_bar()
		{
			health_bar[bar_value].toPos(-(bar_halfwidth - bar_halfwidth*man.get_lives_rate_bar()), -2.9f, bar_halfwidth*man.get_lives_rate_bar(), 0.1f);
			energy_bar[bar_value].toPos(-(bar_halfwidth - bar_halfwidth*man.get_weapon().get_energy_rate()), -2.7f,bar_halfwidth*man.get_weapon().get_energy_rate(), 0.1f);
		}

		//draw text image on the screen
		void draw_text(texture_shader &shader, float x, float y, float scale, const char *text) {
			mat4t modelToWorld;
			modelToWorld.loadIdentity();
			modelToWorld.translate(x, y, 0);
			modelToWorld.scale(scale, scale, 1);
			mat4t modelToProjection = mat4t::build_projection_matrix(modelToWorld, cameraToWorld);

			/*mat4t tmp;
			glLoadIdentity();
			glTranslatef(x, y, 0);
			glGetFloatv(GL_MODELVIEW_MATRIX, (float*)&tmp);
			glScalef(scale, scale, 1);
			glGetFloatv(GL_MODELVIEW_MATRIX, (float*)&tmp);*/

			enum { max_quads = 32 };
			bitmap_font::vertex vertices[max_quads * 4];
			uint32_t indices[max_quads * 6];
			aabb bb(vec3(0, 0, 0), vec3(256, 256, 0));

			unsigned num_quads = font.build_mesh(bb, vertices, indices, max_quads, text, 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, font_texture);

			//draw text in black color
			shader.render(modelToProjection, 0, vec4(0,0,0,1));

			glVertexAttribPointer(attribute_pos, 3, GL_FLOAT, GL_FALSE, sizeof(bitmap_font::vertex), (void*)&vertices[0].x);
			glEnableVertexAttribArray(attribute_pos);
			glVertexAttribPointer(attribute_uv, 3, GL_FLOAT, GL_FALSE, sizeof(bitmap_font::vertex), (void*)&vertices[0].u);
			glEnableVertexAttribArray(attribute_uv);

			glDrawElements(GL_TRIANGLES, num_quads * 6, GL_UNSIGNED_INT, indices);
		}

	public:

		// this is called when we construct the class
		invaderers_app(int argc, char **argv) : app(argc, argv), font(512, 256, "assets/big.fnt") {
		}

		// this is called once OpenGL is initialized
		void app_init() {

			//read csv file
			load_data();

			// set up the shader
			texture_shader_.init();

			// set up the matrices with a camera 5 units from the origin
			cameraToWorld.loadIdentity();
			cameraToWorld.translate(0, 0, 3);

			// sundry counters and game state.
			stage = 1;
			missiles_disabled = 15;
			bombs_disabled = 30;
			icon_disabled = 0;
			regular_invader_velocity = 0.01f;
			invader_velocity = 0.01f;
			bomb_speed = 0.2f;
			live_invaderers = num_invaderers;
			game_over = false;
			is_invader_slow = false;
			is_invader_rage = false;
			score = 0;
			upY = 0;
			lightning_upY = 0;
			man_texture = tex_man;
			
			font_texture = resource_dict::get_texture_handle(GL_RGBA, "assets/big_0.gif");

			GLuint ship = resource_dict::get_texture_handle(GL_RGBA, "assets/invaderers/ash.gif");
			weapon weapon_ball = weapon(icon_ball, 5, missiles_disabled, 0);
			man.get_weapon() = weapon_ball;
			man.init(ship, 0, -2.3f, 0.35f, 0.35f);
			
			GLuint GameOver = resource_dict::get_texture_handle(GL_RGBA, "assets/invaderers/GameOver.gif");
			sprites[game_over_sprite].init(GameOver, 20, 0, 3, 1.5f);

			//printf(texture_position(stage, type_invader));
			generate_invaders();

			// set the border to white for clarity
			GLuint white = resource_dict::get_texture_handle(GL_RGB, "#fffeee");
			sprites[first_border_sprite + 0].init(white, 0, -2.5f, 6, 0.2f);
			sprites[first_border_sprite + 1].init(white, 0, 3, 6, 0.2f);
			sprites[first_border_sprite + 2].init(white, -3, 0, 0.2f, 6);
			sprites[first_border_sprite + 3].init(white, 3, 0, 0.2f, 6);
			sprites[first_border_sprite + 4].init(white, 0, 6, 6, 0.2f);
			sprites[background].init(white, 0, 0.2f, 5.8f, 5.4f);

			// use the missile texture
			GLuint missile = resource_dict::get_texture_handle(GL_RGBA, "assets/invaderers/ball.gif");
			for (int i = 0; i != num_missiles; ++i) {
				// create missiles off-screen
				weapon_sprite[first_missile_sprite + i].init(missile, 50, 10, 0.4f, 0.4f);
				weapon_sprite[first_missile_sprite + i].is_enabled() = false;
			}

			// use the missile texture
			GLuint condition = resource_dict::get_texture_handle(GL_RGBA, "assets/invaderers/icon_slow.gif");
			for (int i = 0; i != num_conditions; ++i) {
				// create missiles off-screen
				condition_sprite[first_condition_sprtie + i].init(condition, 20, 2.5f, 0.25f, 0.25f);
				condition_sprite[first_condition_sprtie + i].is_enabled() = false;
			}

			GLuint explode = resource_dict::get_texture_handle(GL_RGBA, "assets/invaderers/explosion.gif");
			for (int i = 0; i != num_invaderers; ++i) {
				// create explosion off-screen
				explosion_sprite[first_invaderer_sprite + i].init(explode, 20, 2.5f, 0.25f, 0.25f);
				explosion_sprite[first_invaderer_sprite + i].is_enabled() = false;
			}

			// use the bomb texture
			GLuint bomb = resource_dict::get_texture_handle(GL_RGBA, "assets/invaderers/weapon_lightning.gif");
			for (int i = 0; i != num_bombs; ++i) {
				// create bombs off-screen
				weapon_sprite[first_bomb_sprite + i].init(bomb, 20, 0, 0.3f, 0.4f);
				weapon_sprite[first_bomb_sprite + i].is_enabled() = false;
			}

			// use the icon texture
			GLuint icon = resource_dict::get_texture_handle(GL_RGBA, "assets/invaderers/icon_lightning.gif");
			for (int i = 0; i != num_icon; ++i) {
				// create icons off-screen
				icon_sprite[first_icon_sprite + i].init(icon, 20, 0, 0.25f, 0.25f);
				icon_sprite[first_icon_sprite + i].is_enabled() = false;
			}

			// use the bars texture
			GLuint base = resource_dict::get_texture_handle(GL_RGBA, "assets/invaderers/bar_base.gif");
			GLuint value = resource_dict::get_texture_handle(GL_RGBA, "assets/invaderers/bar_value.gif");
			for (int i = 0; i != num_bar_sprite; ++i) {
				// create energy bar off-screen
				energy_bar[bar_base].init(base, 0, -2.7f, 6, 0.2f);
				energy_bar[bar_base].is_enabled() = false;
				energy_bar[bar_value].init(value, 0, -2.7f, 6, 0.2f);
				energy_bar[bar_value].is_enabled() = false;
				// create energy bar off-screen
				health_bar[bar_base].init(base, 0, -2.9f, 6, 0.2f);
				health_bar[bar_base].is_enabled() = false;
				health_bar[bar_value].init(base, 0, -2.9f, 6, 0.2f);
				health_bar[bar_value].is_enabled() = false;
			}


			// sounds
			whoosh = resource_dict::get_sound_handle(AL_FORMAT_MONO16, "assets/invaderers/whoosh.wav");
			bang = resource_dict::get_sound_handle(AL_FORMAT_MONO16, "assets/invaderers/bang.wav");
			cur_source = 0;
			alGenSources(num_sound_sources, sources);
		}

		// called every frame to move things
		void simulate() {
			if (game_over) {
				return;
			}

			move_player();

			fire_missiles();

			fire_bombs();

			move_missiles();

			move_icon();

			move_bombs();

			checking_conditions();

			checking_explosion_and_hit();

			checking_bar();

			move_invaders(invader_velocity, 0);

			sprite &border = sprites[first_border_sprite + (invader_velocity < 0 ? 2 : 3)];
			if (invaders_collide(border)) {
				invader_velocity = -invader_velocity;
				move_invaders(invader_velocity, -0.1f);
			}
		}

		// this is called to draw the world
		void draw_world(int x, int y, int w, int h) {

			//called every frame
			simulate();
			
			// set a viewport - includes whole window area
			glViewport(x, y, w, h);

			// clear the background to black
			glClearColor(0, 0, 0, 1);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// don't allow Z buffer depth testing (closer objects are always drawn in front of far ones)
			glDisable(GL_DEPTH_TEST);

			// allow alpha blend (transparency when alpha channel is 0)
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			// draw all the sprites
			for (int i = 0; i != num_sprites; ++i) {
				if (i != game_over_sprite && i != background)
					//draw borders
					sprites[i].render(texture_shader_, cameraToWorld, default_color);
				else if(i == background)
					//draw background
					sprites[background].render(texture_shader_, cameraToWorld, background_color);
			}
			
			for (int i = 0; i != num_invaderers; ++i) {
				//draw enemies
				invader_sprite[i].render(texture_shader_, cameraToWorld, default_color);
				//draw explosion under each enemies
				explosion_sprite[i].render(texture_shader_, cameraToWorld, default_color);
			}
			for (int i = 0; i != num_weapon_sprites; ++i) {
				//draw weapons
				weapon_sprite[i].render(texture_shader_, cameraToWorld, default_color);
			}
			for (int i = 0; i != num_icon; ++i) {
				//draw icons
				icon_sprite[i].render(texture_shader_, cameraToWorld, default_color);
			}
			for (int i = 0; i != num_conditions; ++i) {
				//draw buff conditions
				condition_sprite[i].render(texture_shader_, cameraToWorld, default_color);
			}

			//draw player
			man.render(texture_shader_, cameraToWorld, man_color);

			//draw energy and health bars
			health_bar[bar_base].render(texture_shader_, cameraToWorld, default_color);
			health_bar[bar_value].render(texture_shader_, cameraToWorld, vec4(1, 0, 0, 1));
			energy_bar[bar_base].render(texture_shader_, cameraToWorld, default_color);
			energy_bar[bar_value].render(texture_shader_, cameraToWorld, default_color);
			
			

			//draw game over sprite in red color 
			sprites[game_over_sprite].render(texture_shader_, cameraToWorld, vec4(1,0,0,1));

			char stage_text[32];
			sprintf(stage_text, "stage: %d", stage);
			draw_text(texture_shader_, -1.75f, 2, 1.0f / 256, stage_text);

			char score_text[32];
			sprintf(score_text, "score: %d   lives: %d", score, man.get_lives_rate());
			draw_text(texture_shader_, -0.85f, 2, 1.0f / 256, score_text);

			char iconTimer_text[32];
			sprintf(iconTimer_text, "icon: %d energy:%d weapon:%d", icon_disabled, man.get_weapon().get_energy()/30, man.get_weapon().get_type());
			draw_text(texture_shader_, 1.75, 2, 1.0f / 256, iconTimer_text);

			// move the listener with the camera
			vec4 &cpos = cameraToWorld.w();
			alListener3f(AL_POSITION, cpos.x(), cpos.y(), cpos.z());

			//if game over, draw score on the screen
			if (game_over)
			{
				char score_text[100];
				sprintf(score_text, "You got score:%d", score);
				draw_text(texture_shader_, 0.5f, -3, 2.0f / 256, score_text);
			}
		}
	};
}
