#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <vector>
#include "ShaderProgram.h"
#include "Matrix.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;

//TEXTURE THINGS
GLuint mainTexture;
GLuint fontTexture;
Matrix projectionMatrix;
Matrix modelViewMatrix;
struct Entity;
float texture_coords[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f };

//GAME THINGS
enum Type {PLAYER, COIN, BLOCK};
bool game_running = true;
float lastFrameTicks = 0.0f;
float elapsed;
float ticks;
ShaderProgram* program;
unsigned char** levelData;
int mapWidth;
int mapHeight;

GLuint LoadTexture(const char *filePath) {
	int w, h, comp;
	unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);
	if (image == NULL) {
		std::cout << "Unable to load image. Make sure the path is correct\n";
		assert(false);
	}
	GLuint retTexture;
	glGenTextures(1, &retTexture);
	glBindTexture(GL_TEXTURE_2D, retTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	stbi_image_free(image);
	return retTexture;
}

struct Entity {
	Entity() {}

	Entity(float x, float y, float sprite_u, float sprite_v, float sprite_width, float sprite_height, float dir_x, float dir_y, GLuint sprite_texture, Type obj_type) {
		position[0] = x;
		position[1] = y;
		speed[0] = dir_x;
		speed[1] = dir_y;
		entity_matrix.Identity();
		entity_matrix.Translate(x, y, 0);
		size[0] = 1.0f;
		size[1] = 1.0f;
		boundary[0] = y + 0.05f * size[1] * 2;
		boundary[1] = y - 0.05f * size[1] * 2;
		boundary[2] = x - 0.05f * size[0] * 2;
		boundary[3] = x + 0.05f * size[0] * 2;
		u = sprite_u;
		v = sprite_v;
		width = sprite_width;
		height = sprite_height;
		type = obj_type;
 	}

	Entity(float x, float y, float sprite_u, float sprite_v, float sprite_width, float sprite_height, float dir_x, float dir_y, GLuint sprite_texture, float size_x, float size_y, Type obj_type) {
		position[0] = x;
		position[1] = y;
		speed[0] = dir_x;
		speed[1] = dir_y;
		entity_matrix.Identity();
		entity_matrix.Translate(x, y, 0);
		size[0] = size_x;
		size[1] = size_y;
		boundary[0] = y + 0.05f * size[1] * 2;
		boundary[1] = y - 0.05f * size[1] * 2;
		boundary[2] = x - 0.05f * size[0] * 2;
		boundary[3] = x + 0.05f * size[0] * 2;
		u = sprite_u;
		v = sprite_v;
		width = sprite_width;
		height = sprite_height;
		type = obj_type;
	}

	void draw() {
		entity_matrix.Identity();
		entity_matrix.Translate(position[0], position[1], 0);
		program->SetModelviewMatrix(entity_matrix);

		vector<float> vertex_data;
		vector<float> tex_coord_data;
		float texture_x = u;
		float texture_y = v;
		vertex_data.insert(vertex_data.end(), {
			(-0.1f * size[0]), 0.1f * size[1],
			(-0.1f * size[0]), -0.1f * size[1],
			(0.1f * size[0]), 0.1f * size[1],
			(0.1f * size[0]), -0.1f * size[1],
			(0.1f * size[0]), 0.1f * size[1],
			(-0.1f * size[0]), -0.1f * size[1],
		});
		tex_coord_data.insert(tex_coord_data.end(), {
			texture_x, texture_y + height,
			texture_x + width, texture_y,
			texture_x, texture_y,
			texture_x + width, texture_y,
			texture_x, texture_y + height,
			texture_x + width, texture_y + height,
		});

		glUseProgram(program->programID);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertex_data.data());
		glEnableVertexAttribArray(program->positionAttribute);
		glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, tex_coord_data.data());
		glEnableVertexAttribArray(program->texCoordAttribute);
		glBindTexture(GL_TEXTURE_2D, texture);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program->positionAttribute);
		glDisableVertexAttribArray(program->texCoordAttribute);
	}

	float position[2]; //x,y
	float boundaries[4];	//up, down, left, right
	float size[2];	//length, width
	float boundary[4];	//up, down, left, right
	float speed[2];	//x, y
	float acceleration[2];	//x, y
	float u;
	float v;
	float width;
	float height;
	GLuint texture;
	Matrix entity_matrix;
	Type type;

};

//Creating Entities
Entity player;
vector<Entity> coins;
vector<int> coins_to_remove;

void drawGame() {
	player.draw();
	for (size_t i = 0; i < coins.size(); i++){
		coins[i].draw();
	}
	modelViewMatrix.Identity();
	modelViewMatrix.Translate(-player.position[0], -player.position[1], 0.0f);
	program->SetModelviewMatrix(modelViewMatrix);
}

void updateGame() {
	//Collision detection between player and coins
	for (size_t i = 0; i < coins.size(); i++) {
		if (player.boundary[0] > coins[i].boundary[1] &&
			player.boundary[1] < coins[i].boundary[0] &&
			player.boundary[2] < coins[i].boundary[3] &&
			player.boundary[3] > coins[i].boundary[2]) {
			coins.erase(coins.begin() + i);
		}
	}
}

bool readHeader(ifstream &stream) {
	string line;
	mapWidth = -1;
	mapHeight = -1;
	while (getline(stream, line)) {
		if (line == "") { break; }
		istringstream sStream(line);
		string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);
		if (key == "width") {
			mapWidth = atoi(value.c_str());
		}
		else if (key == "height") {
			mapHeight = atoi(value.c_str());
		}
	}
	if (mapWidth == -1 || mapHeight == -1) {
		return false;
	}
	else {
		levelData = new unsigned char*[mapHeight];
		for (int i = 0; i < mapHeight; ++i) {
			levelData[i] = new unsigned char[mapWidth];
		}
		return true;
	}
}

bool readLayerData(ifstream &stream) {
	string line;
	while (getline(stream, line)) {
		if (line == "") { break; }
		istringstream sStream(line);
		string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);
		if (key == "data") {
			for (int y = 0; y < mapHeight; y++) {
				getline(stream, line);
				istringstream lineStream(line);
				string tile;
				for (int x = 0; x < mapWidth; x++) {
					getline(lineStream, tile, ',');
					unsigned char val = (unsigned char)atoi(tile.c_str());
					if (val > 0) {
						levelData[y][x] = val - 1;
					}
					else {
						levelData[y][x] = 0;
					}
				}
			}
		}
	}
	return true;
}

bool readEntityData(ifstream &stream) {
	string line;
	string type;
	while (getline(stream, line)) {
		if (line == "") { break; }
		istringstream sStream(line);
		string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);
		if (key == "type") {
			type = value;
		}
		else if (key == "location") {
			istringstream lineStream(value);
			string xPosition, yPosition;
			getline(lineStream, xPosition, ',');
			getline(lineStream, yPosition, ',');
			float placeX = atoi(xPosition.c_str())*TILE_SIZE;
			float placeY = atoi(yPosition.c_str())*-TILE_SIZE;
			placeEntity(type, placeX, placeY);
		}
	}
	return true;
}

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	#ifdef _WINDOWS
		glewInit();
	#endif

	//Setup
	glViewport(0, 0, 1280, 720);
	program = new ShaderProgram(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	projectionMatrix.SetOrthoProjection(-3.55f, 3.55f, -2.0f, 2.0f, -1.0f, 1.0f);
	program->SetModelviewMatrix(modelViewMatrix);
	program->SetProjectionMatrix(projectionMatrix);

	mainTexture = LoadTexture(RESOURCE_FOLDER"sheet.png");
	fontTexture = LoadTexture("font1.png");

	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	//Textures
	GLuint player_texture = LoadTexture("abc.png");

	//Initializing entities
	player = Entity(0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0, 0, player_texture, 1.0f, 1.4f, PLAYER);
	player.acceleration[1] = -0.01f;

	SDL_Event event;
	bool done = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
		}
		if (keys[SDL_SCANCODE_A] && player.boundary[2] > -3.4f) {
			player.position[0] -= player.speed[0] * elapsed;
			player.position[1] -= player.speed[0] * elapsed;
			player.boundary[2] -= player.speed[0] * elapsed;
			player.boundary[3] -= player.speed[0] * elapsed;
		}
		if (keys[SDL_SCANCODE_D] && player.boundary[3] < 3.4f) {
			player.position[0] += player.speed[0] * elapsed;
			player.position[1] += player.speed[0] * elapsed;
			player.boundary[2] += player.speed[0] * elapsed;
			player.boundary[3] += player.speed[0] * elapsed;
		}

		//Ticks
		ticks = (float)SDL_GetTicks() / 1000.0f;
		elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;
	}

	SDL_Quit();
	return 0;
}
