<strong>Hung-Li Chou 2016</strong>

<strong>Intro to Game Programming</strong>

<strong>Assignment 1: Basic 2D game modification from example_invaders</strong>

<strong>November 2016</strong>

Pokemon Threat
============================== 
||Demo video link||
--------------------------------------------------------------------------------------
https://www.youtube.com/watch?v=tEG8wwprpOs


||Game Description||
--------------------------------------------------------------------------------------
You will face a bunch of pokemon and you have to prevent these monsters invading your home by catching or eliminating them all to get the "high score".


||Play Machanic||
--------------------------------------------------------------------------------------
The game has 3 stages to be cleared.
You need to control Ash(character) to avoid the attack from pokemons and to eliminate them to clear stages and the get high score.

When pokemons get hit by the weapon, it will cause damage to them and the explosion sprite will be shown beneath the pokemon. 
After destroying pokemon, it has chance to drop one special item. You can get a new powerful weapon or buff condition temporarily by eating items.

When you lose all HP(health point) or pass all 3 stages, you will get the result on the screen.


||Control||
--------------------------------------------------------------------------------------
Pressing the arrow keys can move character in 4 directions.
Pressing the space key to use weapon. 


||Features||
--------------------------------------------------------------------------------------
1.Items: There are 4 items in the game so far

(1)Lighting(weapon): You will get help by pikachu using constant lighting attack.
(2)Star(weapon): Star can give strength to Ash so he can throw balls in the amazing speed.
(3)Eye of provindence(buff): Under this condition, every pokemon becomes slow.
(4)Heart(buff): You will get recovered constantly in the period of time.

2.Difficulty:

At each stage, the invaders have different abilities and offensive skills and they will be stronger with the stage progress. 
For example, pikachu is the invader at the first stage with the lightning attack. 
When you pass to the next stage, the invader will be more powerful and this makes the game more difficult.


||UI||
--------------------------------------------------------------------------------------
1.Score(text):Showing the score you get at the moment
2.Stage(text):Indicating which stage you are
3.HP(bar):health point
4.Special weapon energy(bar): The time limit for using special weapons 
5.Buff condition(image): Showing the temporary buff condition you have.


||Understanding of new things(difficult things)||
--------------------------------------------------------------------------------------
1.I can set color to each sprite by adding new uniform(ex:glUniform4fv) variables to pass value to "texture_shader.h".
EX: 1.The background of each stage has different color
    2.The player sprtie turns red to show it got hit by enimies.

2.Load data from csv. or txt. files.
EX: 1.Load enemy abilities(Sprite size, Power, Health) from the "enemy.csv" to the vectors which will be used for generating enemies for the new stage.
	2.Load weapon and buff condition data from the "icon.txt" to the vectors

3.Add and change sprites' textures.
EX: 1.Add new arrays for storing more sprites("explosion", "icon" sprites...).
	2."set_texture" function in class "sprite" can change to different textures. 

4.Create new classes derived from other class (Polymorphism)
EX: Create class "weapon", "icon", "explosion", "condition", "icon", "player" and "enemy" derived from class "sprite" 

*5.Change UV to complement animated gif(TODO thing)
   Sprite sheet contains lots of gif image, so we can keep changing values in uv array to change sprites
   EX: not using the this: static const float uvs[] = {0, 0, 1, 0, 1, 1, 0, 1,};
	   but using this:                  float uvs[] = {uv_x, uv_y, uv_x + uv_w, uv_y, uv_x + uv_w, uv_y + uv_h, uv_x, uv_y + uv_h,};
	   By changing uv_x,uv_y,uv_w,uv_h, we can change sprites from sprite sheet.

6.Transform the way of showing numbers to image(like a progress bar)
EX: Player's health and weapon energy are showing as 2 bars.
	Change bar sprite's size and position according to the calcultion of values


||Modification||
--------------------------------------------------------------------------------------
1.Change sprite texture, postion and size by calling public functions in class "sprite"

void toPos(float x, float y, float w, float h)
		{
			modelToWorld.loadIdentity();
			modelToWorld.translate(x, y, 0);
			halfWidth = w ;
			halfHeight = h;
		}

void set_texture(int _texture)
		{
			texture = _texture;
		}

2.Add up and down control of player character sprite

3.Stage-based game: all enemies in the scene then the game will go to next stage and create new(stronger) set of enemy.

4.Put all asset paths to a function("texture postion(int value,int type)") so each sprite can search its texture(path for Gluint) based on its value and type.
  (This function will return a pointer char)   

||Functions||
List of all functions in class "invaderers_app" and explaination
--------------------------------------------------------------------------------------
<<Private functions>>

[Additions]
load_data(): load game data from csv and txt files.
set_next_stage(): set the new stage for the game.
generate_invaders(): generating new set of enemies for the new stage.
texture_position(): find the texture path in the asset folder.
check_enemy_condition(): check the number of enemies to set game status.
show_game_over(): showing game is over.
move_icon(): move all item icons
slow_invader(): "Slow(condition)" can slow invaders for a while
on_icon_effect(): set received icon effect to the current weapon or condition 
checking_conditions():checking the status of all buff conditions.
checking_explosion_and_hit():let every explosion show a period of time(like a second), then close it
checking_bar(): updating health and energy bar sprites based on their actual values.

[Minor modifications]
on_hit_invaderer(): called when missile hit an enemy.
on_hit_ship(): called when players are hit.
move_player(): use the keyboard to move the ship.
move_missiles(): animate the missiles.
move_bombs(): animate the bombs.
move_invaders(): move the array of enemies.
fire_missiles(): fire button (space).
fire_bombs(): pick an enemy and fire a bomb.
invaders_collide(): check if any enemies hit the sides.

[unchanged]
draw_text(): draw text image on the screen

<<Public functions>>
[Minor modifications]
app_init(): this is called once OpenGL is initialized.
simulate(): called every frame to move things
draw_world(): this is called to draw the world