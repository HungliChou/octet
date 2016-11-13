////////////////////////////////////////////////////////////////////////////////
//
// (C) Andy Thomason 2012-2014
//
// Modular Framework for OpenGLES2 rendering on multiple platforms.
//
// invaderer example: simple game with sprites and sounds
//
// Level: 1
//
// Demonstrates:
//   Basic framework app
//   Shaders
//   Basic Matrices
//   Simple game mechanics
//   Texture loaded from GIF file
//   Audio
//

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
  public:
    sprite() {
      texture = 0;
      enabled = true;
    }

    void init(int _texture, float x, float y, float w, float h) {
      modelToWorld.loadIdentity();
      modelToWorld.translate(x, y, 0);
      halfWidth = w * 0.5f;
      halfHeight = h * 0.5f;
      texture = _texture;
      enabled = true;
    }

    void render(texture_shader &shader, mat4t &cameraToWorld) {
      // invisible sprite... used for gameplay.
      if (!texture) return;

      // build a projection matrix: model -> world -> camera -> projection
      // the projection space is the cube -1 <= x/w, y/w, z/w <= 1
      mat4t modelToProjection = mat4t::build_projection_matrix(modelToWorld, cameraToWorld);

      // set up opengl to draw textured triangles using sampler 0 (GL_TEXTURE0)
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, texture);

      // use "old skool" rendering
      //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
      //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      shader.render(modelToProjection, 0);

      // this is an array of the positions of the corners of the sprite in 3D
      // a straight "float" here means this array is being generated here at runtime.
      float vertices[] = {
        -halfWidth, -halfHeight, 0,
         halfWidth, -halfHeight, 0,
         halfWidth,  halfHeight, 0,
        -halfWidth,  halfHeight, 0,
      };

      // attribute_pos (=0) is position of each corner
      // each corner has 3 floats (x, y, z)
      // there is no gap between the 3 floats and hence the stride is 3*sizeof(float)
      glVertexAttribPointer(attribute_pos, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)vertices );
      glEnableVertexAttribArray(attribute_pos);
    
      // this is an array of the positions of the corners of the texture in 2D
      static const float uvs[] = {
         0,  0,
         1,  0,
         1,  1,
         0,  1,
      };

      // attribute_uv is position in the texture of each corner
      // each corner (vertex) has 2 floats (x, y)
      // there is no gap between the 2 floats and hence the stride is 2*sizeof(float)
      glVertexAttribPointer(attribute_uv, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), (void*)uvs );
      glEnableVertexAttribArray(attribute_uv);
    
      // finally, draw the sprite (4 vertices)
      glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }

    // move the object
    void translate(float x, float y) {
      modelToWorld.translate(x, y, 0);
    }

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

  class invaderers_app : public octet::app {
    // Matrix to transform points in our camera space to the world.
    // This lets us move our camera
    mat4t cameraToWorld;

    // shader to draw a textured triangle
    texture_shader texture_shader_;

    enum {
      num_sound_sources = 8,
      num_rows = 5,
      num_cols = 10,
      num_missiles = 2,
      num_bombs = 2,
      num_borders = 4,
      num_invaderers = num_rows * num_cols,

      // sprite definitions
      ship_sprite = 0,
      game_over_sprite,

      first_invaderer_sprite,
      last_invaderer_sprite = first_invaderer_sprite + num_invaderers - 1,

      first_missile_sprite,
      last_missile_sprite = first_missile_sprite + num_missiles - 1,

      first_bomb_sprite,
      last_bomb_sprite = first_bomb_sprite + num_bombs - 1,

      first_border_sprite,
      last_border_sprite = first_border_sprite + num_borders - 1,

      num_sprites,

    };

    // timers for missiles and bombs
    int missiles_disabled;
    int bombs_disabled;

    // accounting for bad guys
    int live_invaderers;
    int num_lives;

    // game state
    bool game_over;
    int score;

    // speed of enemy
    float invader_velocity;

    // sounds
    ALuint whoosh;
    ALuint bang;
    unsigned cur_source;
    ALuint sources[num_sound_sources];

    // big array of sprites
    sprite sprites[num_sprites];

    // random number generator
    class random randomizer;

    // a texture for our text
    GLuint font_texture;

    // information for our text
    bitmap_font font;

    ALuint get_sound_source() { return sources[cur_source++ % num_sound_sources]; }

    // called when we hit an enemy
    void on_hit_invaderer() {
      ALuint source = get_sound_source();
      alSourcei(source, AL_BUFFER, bang);
      alSourcePlay(source);

      live_invaderers--;
      score++;
      if (live_invaderers == 4) {
        invader_velocity *= 4;
      } else if (live_invaderers == 0) {
        game_over = true;
        sprites[game_over_sprite].translate(-20, 0);
      }
    }

    // called when we are hit
    void on_hit_ship() {
      ALuint source = get_sound_source();
      alSourcei(source, AL_BUFFER, bang);
      alSourcePlay(source);

      if (--num_lives == 0) {
        game_over = true;
        sprites[game_over_sprite].translate(-20, 0);
      }
    }

    // use the keyboard to move the ship
    void move_ship() {
      const float ship_speed = 0.05f;
      // left and right arrows
      if (is_key_down(key_left)) {
        sprites[ship_sprite].translate(-ship_speed, 0);
        if (sprites[ship_sprite].collides_with(sprites[first_border_sprite+2])) {
          sprites[ship_sprite].translate(+ship_speed, 0);
        }
      } else if (is_key_down(key_right)) {
        sprites[ship_sprite].translate(+ship_speed, 0);
        if (sprites[ship_sprite].collides_with(sprites[first_border_sprite+3])) {
          sprites[ship_sprite].translate(-ship_speed, 0);
        }
      }
    }

    // fire button (space)
    void fire_missiles() {
      if (missiles_disabled) {
        --missiles_disabled;
      } else if (is_key_going_down(' ')) {
        // find a missile
        for (int i = 0; i != num_missiles; ++i) {
          if (!sprites[first_missile_sprite+i].is_enabled()) {
            sprites[first_missile_sprite+i].set_relative(sprites[ship_sprite], 0, 0.5f);
            sprites[first_missile_sprite+i].is_enabled() = true;
            missiles_disabled = 5;
            ALuint source = get_sound_source();
            alSourcei(source, AL_BUFFER, whoosh);
            alSourcePlay(source);
            break;
          }
        }
      }
    }

    // pick and invader and fire a bomb
    void fire_bombs() {
      if (bombs_disabled) {
        --bombs_disabled;
      } else {
        // find an invaderer
        sprite &ship = sprites[ship_sprite];
        for (int j = randomizer.get(0, num_invaderers); j < num_invaderers; ++j) {
          sprite &invaderer = sprites[first_invaderer_sprite+j];
          if (invaderer.is_enabled() && invaderer.is_above(ship, 0.3f)) {
            // find a bomb
            for (int i = 0; i != num_bombs; ++i) {
              if (!sprites[first_bomb_sprite+i].is_enabled()) {
                sprites[first_bomb_sprite+i].set_relative(invaderer, 0, -0.25f);
                sprites[first_bomb_sprite+i].is_enabled() = true;
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
      const float missile_speed = 0.3f;
      for (int i = 0; i != num_missiles; ++i) {
        sprite &missile = sprites[first_missile_sprite+i];
        if (missile.is_enabled()) {
          missile.translate(0, missile_speed);
          for (int j = 0; j != num_invaderers; ++j) {
            sprite &invaderer = sprites[first_invaderer_sprite+j];
            if (invaderer.is_enabled() && missile.collides_with(invaderer)) {
              invaderer.is_enabled() = false;
              invaderer.translate(20, 0);
              missile.is_enabled() = false;
              missile.translate(20, 0);
              on_hit_invaderer();

              goto next_missile;
            }
          }
          if (missile.collides_with(sprites[first_border_sprite+1])) {
            missile.is_enabled() = false;
            missile.translate(20, 0);
          }
        }
      next_missile:;
      }
    }

    // animate the bombs
    void move_bombs() {
      const float bomb_speed = 0.2f;
      for (int i = 0; i != num_bombs; ++i) {
        sprite &bomb = sprites[first_bomb_sprite+i];
        if (bomb.is_enabled()) {
          bomb.translate(0, -bomb_speed);
          if (bomb.collides_with(sprites[ship_sprite])) {
            bomb.is_enabled() = false;
            bomb.translate(20, 0);
            bombs_disabled = 50;
            on_hit_ship();
            goto next_bomb;
          }
          if (bomb.collides_with(sprites[first_border_sprite+0])) {
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
        sprite &invaderer = sprites[first_invaderer_sprite+j];
        if (invaderer.is_enabled()) {
          invaderer.translate(dx, dy);
        }
      }
    }

    // check if any invaders hit the sides.
    bool invaders_collide(sprite &border) {
      for (int j = 0; j != num_invaderers; ++j) {
        sprite &invaderer = sprites[first_invaderer_sprite+j];
        if (invaderer.is_enabled() && invaderer.collides_with(border)) {
          return true;
        }
      }
      return false;
    }


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
      bitmap_font::vertex vertices[max_quads*4];
      uint32_t indices[max_quads*6];
      aabb bb(vec3(0, 0, 0), vec3(256, 256, 0));

      unsigned num_quads = font.build_mesh(bb, vertices, indices, max_quads, text, 0);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, font_texture);

      shader.render(modelToProjection, 0);

      glVertexAttribPointer(attribute_pos, 3, GL_FLOAT, GL_FALSE, sizeof(bitmap_font::vertex), (void*)&vertices[0].x );
      glEnableVertexAttribArray(attribute_pos);
      glVertexAttribPointer(attribute_uv, 3, GL_FLOAT, GL_FALSE, sizeof(bitmap_font::vertex), (void*)&vertices[0].u );
      glEnableVertexAttribArray(attribute_uv);

      glDrawElements(GL_TRIANGLES, num_quads * 6, GL_UNSIGNED_INT, indices);
    }

  public:

    // this is called when we construct the class
    invaderers_app(int argc, char **argv) : app(argc, argv), font(512, 256, "assets/big.fnt") {
    }

    // this is called once OpenGL is initialized
    void app_init() {
      // set up the shader
      texture_shader_.init();

      // set up the matrices with a camera 5 units from the origin
      cameraToWorld.loadIdentity();
      cameraToWorld.translate(0, 0, 3);

      font_texture = resource_dict::get_texture_handle(GL_RGBA, "assets/big_0.gif");

      GLuint ship = resource_dict::get_texture_handle(GL_RGBA, "assets/invaderers/ship.gif");
      sprites[ship_sprite].init(ship, 0, -2.75f, 0.25f, 0.25f);

      GLuint GameOver = resource_dict::get_texture_handle(GL_RGBA, "assets/invaderers/GameOver.gif");
      sprites[game_over_sprite].init(GameOver, 20, 0, 3, 1.5f);

      GLuint invaderer = resource_dict::get_texture_handle(GL_RGBA, "assets/invaderers/invaderer.gif");
      for (int j = 0; j != num_rows; ++j) {
        for (int i = 0; i != num_cols; ++i) {
          assert(first_invaderer_sprite + i + j*num_cols <= last_invaderer_sprite);
          sprites[first_invaderer_sprite + i + j*num_cols].init(
            invaderer, ((float)i - num_cols * 0.5f) * 0.5f, 2.50f - ((float)j * 0.5f), 0.25f, 0.25f
          );
        }
      }

      // set the border to white for clarity
      GLuint white = resource_dict::get_texture_handle(GL_RGB, "#ffffff");
      sprites[first_border_sprite+0].init(white, 0, -3, 6, 0.2f);
      sprites[first_border_sprite+1].init(white, 0,  3, 6, 0.2f);
      sprites[first_border_sprite+2].init(white, -3, 0, 0.2f, 6);
      sprites[first_border_sprite+3].init(white, 3,  0, 0.2f, 6);

      // use the missile texture
      GLuint missile = resource_dict::get_texture_handle(GL_RGBA, "assets/invaderers/missile.gif");
      for (int i = 0; i != num_missiles; ++i) {
        // create missiles off-screen
        sprites[first_missile_sprite+i].init(missile, 20, 0, 0.0625f, 0.25f);
        sprites[first_missile_sprite+i].is_enabled() = false;
      }

      // use the bomb texture
      GLuint bomb = resource_dict::get_texture_handle(GL_RGBA, "assets/invaderers/bomb.gif");
      for (int i = 0; i != num_bombs; ++i) {
        // create bombs off-screen
        sprites[first_bomb_sprite+i].init(bomb, 20, 0, 0.0625f, 0.25f);
        sprites[first_bomb_sprite+i].is_enabled() = false;
      }

      // sounds
      whoosh = resource_dict::get_sound_handle(AL_FORMAT_MONO16, "assets/invaderers/whoosh.wav");
      bang = resource_dict::get_sound_handle(AL_FORMAT_MONO16, "assets/invaderers/bang.wav");
      cur_source = 0;
      alGenSources(num_sound_sources, sources);

      // sundry counters and game state.
      missiles_disabled = 0;
      bombs_disabled = 50;
      invader_velocity = 0.01f;
      live_invaderers = num_invaderers;
      num_lives = 3;
      game_over = false;
      score = 0;
    }

    // called every frame to move things
    void simulate() {
      if (game_over) {
        return;
      }

      move_ship();

      fire_missiles();

      fire_bombs();

      move_missiles();

      move_bombs();

      move_invaders(invader_velocity, 0);

      sprite &border = sprites[first_border_sprite+(invader_velocity < 0 ? 2 : 3)];
      if (invaders_collide(border)) {
        invader_velocity = -invader_velocity;
        move_invaders(invader_velocity, -0.1f);
      }
    }

    // this is called to draw the world
    void draw_world(int x, int y, int w, int h) {
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
        sprites[i].render(texture_shader_, cameraToWorld);
      }

      char score_text[32];
      sprintf(score_text, "score: %d   lives: %d\n", score, num_lives);
      draw_text(texture_shader_, -1.75f, 2, 1.0f/256, score_text);

      // move the listener with the camera
      vec4 &cpos = cameraToWorld.w();
      alListener3f(AL_POSITION, cpos.x(), cpos.y(), cpos.z());
    }
  };
		void set_texture(int _texture)
		{
			texture = _texture;
		}
	class weapon : public sprite {
		//the type of weapon
		int type;

		//the power of weapon
		int power;

		//disable time of weapon
		int disable_time;

		//how many time we can use this weapon(0->unlimited,>0->limited)
		int energy;

		//the texture name in the asset folder
		//char asset_name;

	public:
		weapon() { type = 1; power = 1; disable_time = 5; energy = -1; }
		weapon(int _type, int _power, int _disable_time, int _energy) { type = _type; power = _power; disable_time = _disable_time; energy = _energy; }
		int &get_type() { return type; }
		int &get_power() { return power; }
		int &get_disable_time() { return disable_time; }
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
		// invarder lives
		int lives;

		// invader level
		int level;

	public:
		enemy() { lives = 1; level = 1; }

		int &get_level()
		{
			return level;
		}

		int &get_lives()
		{
			return lives;
		}

		void set_level(int _level)
		{
			lives = _level;
			level = _level;
		}
	};

	class player : public sprite {
		int lives;
		weapon my_weapon;
	public:
		player() { lives = 6; }
		player(int _lives) { lives = _lives; }

		//return lives
		int &get_lives() { return lives; }

		// change different weapons
		//void set_weapon(weapon _weapon){my_weapon = _weapon;}

		weapon &get_weapon() { return my_weapon; }
	};
			num_icon = 4,
			num_max_stage = 3,
			type_invader = 1,
			type_weapon,
			type_icon,
			//invaders sprite definitions
			first_invaderer_sprite = 0,
			last_invaderer_sprite = first_invaderer_sprite + num_invaderers - 1,

			//weapon sprite definitions
			first_missile_sprite = 0,
			last_missile_sprite = first_missile_sprite + num_missiles - 1,

			first_bomb_sprite,
			last_bomb_sprite = first_bomb_sprite + num_bombs - 1,
			num_weapon_sprites,
			//weapon icon sprite definitions
			first_icon_sprite = 0,
			last_icon_sprite = first_icon_sprite + num_icon - 1,
			//icon definitions
			icon_ball = 0,
			icon_lightning,
			icon_slow,
			icon_star,

		int icon_disabled;
		// big array of sprites
		sprite sprites[num_sprites];
		// big array of invader sprites
		enemy invader_sprite[num_invaderers];
		// big array of weapon sprites
		weapon weapon_sprite[num_weapon_sprites];
		// big array of icon_sprites
		icon icon_sprite[num_icon];
		// player sprite
		player man;
		void generate_invaders()
		{
			GLuint invaderer = resource_dict::get_texture_handle(GL_RGBA, texture_position(stage, type_invader));;
			float w;
			float h;
			if (stage == 1)
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
			}
			else if (stage == 3)
			{
				w = 0.4f;
				h = 0.4f;
			}
			for (int j = 0; j != num_rows; ++j) {
				for (int i = 0; i != num_cols; ++i) {
					assert(first_invaderer_sprite + i + j*num_cols <= last_invaderer_sprite);
					invader_sprite[first_invaderer_sprite + i + j*num_cols].init(
						invaderer, ((float)i - num_cols * 0.5f) * 0.5f, 2.50f - ((float)j * 0.5f), w, h
					);
					invader_sprite[first_invaderer_sprite + i + j*num_cols].set_level(stage);
				}
			}
		}

		//set the new stage for the game
		void set_next_stage()
		{
			stage++;
			invader_velocity = 0.01f;
			live_invaderers = num_invaderers;
			//TODO regenerate invaders which have new level.
			generate_invaders();
		}

		//find the texture route in the asset folder
		char* texture_position(int value, int type)
		{
			char path_array[500];
			char *path = "assets/invaderers/invader1_1.gif";
			if (type == type_invader)
			{
				char num[10];
				char lives_array[10];

				sprintf(num, "%d", stage);
				sprintf(lives_array, "%d", value);
				sprintf(path_array, "assets/invaderers/invader%s_%s.gif", num, lives_array);

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
				default:
					path = "assets/invaderers/ball.gif";
					break;
				}
			}
			return path;
		}
		// called when we hit an enemy
		void on_hit_invaderer(enemy &invaderer) {
			ALuint source = get_sound_source();
			GLuint newtexture;
			icon &icon = icon_sprite[first_icon_sprite];
			alSourcei(source, AL_BUFFER, bang);
			alSourcePlay(source);
			printf("live: %d", invaderer.get_lives());
			int lives = invaderer.get_lives() - 1;
			int level = invaderer.get_level();
			if (lives == 0)
			{
				if (icon_disabled == 0)
				{
					for (int i = 0; i != num_icon; ++i) {
						icon = icon_sprite[first_icon_sprite + i];
						if (!icon.is_enabled()) {
							int random_weapon = randomizer.get(1, num_icon);
							newtexture = resource_dict::get_texture_handle(GL_RGBA, texture_position(random_weapon, type_icon));
							//newtexture = resource_dict::get_texture_handle(GL_RGBA, "assets/invaderers/ball.gif");
							icon.set_relative(invaderer, 0, 0);
							icon.get_type() = random_weapon;
							icon.is_enabled() = true;
							icon.set_texture(newtexture);
							icon_disabled = 200;
							printf("icon:%d", i);

							//ALuint source = get_sound_source();
							//alSourcei(source, AL_BUFFER, whoosh);
							//alSourcePlay(source);
							break;
						}
					}
				}

				invaderer.is_enabled() = false;
				invaderer.translate(20, 0);
				live_invaderers--;
				score = score + 10 * level*level;

			}
			else
			{
				//char *tex_pos = texture_position(lives);
				//printf(tex_pos);
				if (stage > 1)
				{
					newtexture = resource_dict::get_texture_handle(GL_RGBA, texture_position(lives, type_invader));
				}

				invaderer.set_texture(newtexture);
				invaderer.render(texture_shader_, cameraToWorld);
				invaderer.get_lives() = lives;
			}



			if (live_invaderers == 4) {
				invader_velocity *= 4;
			}
			else if (live_invaderers == 0) {
				printf("stage:%d", stage);
				if (stage<num_max_stage)
					set_next_stage();
				else
				{
					game_over = true;
					sprites[game_over_sprite].translate(-20, 0);
				}
			}
		}
		// called when we are hit
		void on_hit_ship() {
			ALuint source = get_sound_source();
			alSourcei(source, AL_BUFFER, bang);
			alSourcePlay(source);

			int cur_lives = man.get_lives();
			cur_lives--;
			man.get_lives() = cur_lives;

			if (cur_lives == 0) {
				game_over = true;
				sprites[game_over_sprite].translate(-20, 0);
			}
		}

		// use the keyboard to move the ship
		void move_ship() {
			const float ship_speed = 0.1f;

			//player &player = man;
			// left and right arrows
			if (is_key_down(key_left)) {
				man.translate(-ship_speed, 0);
				if (man.collides_with(sprites[first_border_sprite + 2])) {
					man.translate(+ship_speed, 0);
				}
			}
			else if (is_key_down(key_right)) {
				man.translate(+ship_speed, 0);
				if (man.collides_with(sprites[first_border_sprite + 3])) {
					man.translate(-ship_speed, 0);
				}
			}

			//TODO
			//move up and down
			//TODO
			//give limit of key_up
			else if (is_key_down(key_up)) {

				if (man.is_enabled() && upY<2)
				{
					man.translate(0, +ship_speed);
					if (man.collides_with(sprites[first_border_sprite + 1])) {
						man.translate(0, -ship_speed);
					}
					else
						upY += ship_speed;
				}

			}
			else if (is_key_down(key_down)) {
				man.translate(0, -ship_speed);
				if (man.collides_with(sprites[first_border_sprite])) {
					man.translate(0, +ship_speed);
				}
				else
					upY -= ship_speed;
			}
		}
		// fire button (space)
		void fire_missiles() {

			if (man.get_weapon().get_type() != icon_ball)
			{
				if (man.get_weapon().get_energy() > 0)
					man.get_weapon().get_energy()--;
				else
				{
					if (man.get_weapon().get_type() == icon_slow)
						slow_invader(false);
					change_weapon(icon_ball);
				}
			}


			if (man.get_weapon().get_type() == icon_lightning)
			{
				weapon &my_weapon = weapon_sprite[first_missile_sprite];
				if (!my_weapon.is_enabled())
				{
					if (is_key_down(' '))
					{
						my_weapon.set_relative(man, 0, 2.5f);
						my_weapon.is_enabled() = true;
						printf("D");
					}
				}
			}
			else
			{
				if (missiles_disabled) {
					--missiles_disabled;
				}
				else if (is_key_down(' ')) {
					// find a missile
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
		}
		void move_icon() {
			if (icon_disabled > 0)
				icon_disabled--;
			const float icon_speed = 0.05f;
			for (int j = 0; j != num_icon; ++j) {
				icon &icon = icon_sprite[first_icon_sprite + j];
				if (icon.is_enabled()) {
					icon.translate(0, -icon_speed);
					if (icon.collides_with(man)) {
						icon.is_enabled() = false;
						icon.translate(20, 0);
						change_weapon(icon.get_type());
						//on_hit_ship();
						goto next_icon;
					}
					if (icon.collides_with(sprites[first_border_sprite + 0])) {
						icon.is_enabled() = false;
						icon.translate(20, 0);
					}
				}
			next_icon:;
			}
		}

		// skill: can slow invaders for a while
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

		void change_weapon(int type)
		{
			if (man.get_weapon().get_type() = type)
			{
				weapon new_weapon;
				GLuint weapon_texture;
				float w = 0.4f;
				float h = 0.4f;
				switch (type) {
				case icon_lightning:
					//change weapon info to player
					new_weapon = weapon(icon_lightning, 1, 10, 300);
					w = 1.1f;
					h = 5.0f;
					printf("lightning!!!");
					break;
				case icon_slow:
					new_weapon = weapon(icon_slow, 5, 15, 300);
					slow_invader(true);
					printf("slow defender");
					break;
				case icon_star:
					new_weapon = weapon(icon_star, 5, 5, 300);
					printf("star motion");
					break;
				case icon_ball:
					new_weapon = weapon(icon_ball, 5, 15, 0);
					printf("become normal");
					break;
				default:
					break;
				}
				man.get_weapon() = new_weapon;
				missiles_disabled = man.get_weapon().get_disable_time();
				weapon_texture = resource_dict::get_texture_handle(GL_RGBA, texture_position(type, type_weapon));
				for (int i = 0; i != num_missiles; ++i) {
					weapon_sprite[first_missile_sprite + i].init(weapon_texture, 50, 0, w, h);
					weapon_sprite[first_missile_sprite + i].is_enabled() = false;
				}
			}
		}
			stage = 1;
			missiles_disabled = 15;
			bombs_disabled = 30;
			icon_disabled = 0;
			invader_velocity = 0.01f;
			bomb_speed = 0.2f;
			live_invaderers = num_invaderers;
			game_over = false;
			is_invader_slow = false;
			score = 0;
			upY = 0;
			// use the missile texture
			GLuint missile = resource_dict::get_texture_handle(GL_RGBA, "assets/invaderers/ball.gif");
			for (int i = 0; i != num_missiles; ++i) {
				// create missiles off-screen
				weapon_sprite[first_missile_sprite + i].init(missile, 50, 10, 0.4f, 0.4f);
				weapon_sprite[first_missile_sprite + i].is_enabled() = false;
			}
			// use the icon texture
			GLuint icon = resource_dict::get_texture_handle(GL_RGBA, "assets/invaderers/icon_lightning.gif");
			for (int i = 0; i != num_icon; ++i) {
				// create bombs off-screen
				icon_sprite[first_icon_sprite + i].init(icon, 20, 0, 0.25f, 0.25f);
				icon_sprite[first_icon_sprite + i].is_enabled() = false;
			}
			move_icon();
}
