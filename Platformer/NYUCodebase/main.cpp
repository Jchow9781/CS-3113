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
#define SPRITE_COUNT_X 16
#define SPRITE_COUNT_Y 8
#define TILE_SIZE 0.4f
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

struct Entity;
float texture_coords[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f };

//GAME THINGS
enum Type {PLAYER, COIN};
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

struct SpriteSheet {
	SpriteSheet() {};

	SpriteSheet(unsigned int textureID, float u, float v, float width, float height, float size)
		: textureID(textureID), u(u), v(v), width(width), height(height), size(size) {};

	void Draw(ShaderProgram* program) {
		glBindTexture(GL_TEXTURE_2D, textureID);
		GLfloat texCoords[] = {
			u, v + height,
			u + width, v,
			u, v,
			u + width, v,
			u, v + height,
			u + width, v + height
		};

		float aspect = width / height;
		float vertices[] = {
			-0.5f * size * aspect, -0.5f * size,
			0.5f * size * aspect, 0.5f * size,
			-0.5f * size * aspect, 0.5f * size,
			0.5f * size * aspect, 0.5f * size,
			-0.5f * size * aspect, -0.5f * size,
			0.5f * size * aspect, -0.5f * size
		};

		glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program->positionAttribute);

		glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program->texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program->positionAttribute);
		glDisableVertexAttribArray(program->texCoordAttribute);
	}

	float u;
	float v;
	float width;
	float height;
	float size;
	unsigned int textureID;
};

float lerp(float v0, float v1, float t) {
	return (1.0 - t) * v0 + t * v1;
}

struct Entity {
	Entity() {}

	/*Entity(float x, float y, float sprite_u, float sprite_v, float sprite_width, float sprite_height, float dir_x, float dir_y, GLuint sprite_texture, Type obj_type) {
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
	*/
	
	void update(float elapsed) {
		if (!is_static) {
			acceleration[1] = -1.0f;
			speed[0] = lerp(speed[0], 0.0f, elapsed * friction[0]);
			speed[1] = lerp(speed[1], 0.0f, elapsed * friction[1]);
			speed[0] += acceleration[0] * elapsed;
			speed[1] += acceleration[1] * elapsed;
			position[0] += speed[0] * elapsed;
			position[1] += speed[1] * elapsed;			
		}
	}

	void draw(ShaderProgram* program, Matrix& modelMatrix) {
		modelMatrix.Identity();
		modelMatrix.Translate(position[0], position[1], 0);
		modelMatrix.Scale(1.5f, 1.0f, 1.0f);
		program->SetModelviewMatrix(modelMatrix);
		this->sprite.Draw(program);
	}
	/*
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
	}*/

	float position[2]; //x,y
	float boundaries[4];	//up, down, left, right
	float size[2];	//length, width
	float boundary[4];	//up, down, left, right
	float speed[2];	//x, y
	float acceleration[2];	//x, y
	float friction[2];	//x, y
	float u;
	float v;
	float width;
	float height;
	bool is_static;
	GLuint texture;
	Matrix entity_matrix;
	Type type;
	SpriteSheet sprite;
};

//Creating Entities vector
vector<Entity*> entities;

void worldToTileCoordinates(float worldX, float worldY, int *gridX, int *gridY) {
	*gridX = (int)(worldX / TILE_SIZE);
	*gridY = (int)(-worldY / TILE_SIZE);
}

void buildLevel(ShaderProgram *program, GLuint tilesTexture) {
	vector<float> vertexData;
	vector<float> texCoordData;
	int count = 0;
	for (int y = 0; y < mapHeight; y++) {
		for (int x = 0; x < mapWidth; x++) {
			if (levelData[y][x] != 0) {
				float u = (float)(((int)levelData[y][x]) % SPRITE_COUNT_X) / (float)SPRITE_COUNT_X;
				float v = (float)(((int)levelData[y][x]) / SPRITE_COUNT_X) / (float)SPRITE_COUNT_Y;
				float spriteWidth = 1.0f / (float)SPRITE_COUNT_X;
				float spriteHeight = 1.0f / (float)SPRITE_COUNT_Y;
				count += 1;
				vertexData.insert(vertexData.end(), {
					TILE_SIZE * x, -TILE_SIZE * y,
					TILE_SIZE * x, (-TILE_SIZE * y) - TILE_SIZE,
					(TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,
					TILE_SIZE * x, -TILE_SIZE * y,
					(TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,
					(TILE_SIZE * x) + TILE_SIZE, -TILE_SIZE * y
				});
				texCoordData.insert(texCoordData.end(), {
					u, v,
					u, v + (spriteHeight),
					u + spriteWidth, v + (spriteHeight),
					u, v,
					u + spriteWidth, v + (spriteHeight),
					u + spriteWidth, v
				});
			}
		}
	}

	glUseProgram(program->programID);

	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
	glEnableVertexAttribArray(program->positionAttribute);

	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
	glEnableVertexAttribArray(program->texCoordAttribute);

	glBindTexture(GL_TEXTURE_2D, tilesTexture);
	glDrawArrays(GL_TRIANGLES, 0, count * 6);

	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
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

void placeEntity(string& type, float x, float y) {
	Entity* entity = new Entity();
	if (type == "Player") {
		entity->type = PLAYER;
		entity->is_static = false;
	}
	if (type == "Coin") {
		entity->type = COIN;
		entity->is_static = true;
	}
	entity->position[0] = x;
	entity->position[1] = y;
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

	Matrix projectionMatrix;
	Matrix modelMatrix;
	Matrix viewMatrix;

	projectionMatrix.SetOrthoProjection(-3.55f, 3.55f, -2.0f, 2.0f, -1.0f, 1.0f);

	GLuint tile_texture = LoadTexture("arne_sprites.png");
	SpriteSheet playerSprite(tile_texture, 0.0f / 256.0f, 80.0f / 128.0f, 16.0f / 256.0f, 16.0f / 128.0f, 0.4f);
	SpriteSheet coinSprite(tile_texture, 64.0f / 256.0f, 48.0f / 128.0f, 16.0f / 256.0f, 16.0f / 128.0f, 0.4f);

	//Reading into map file
	ifstream infile("MyTileMap.txt");
	string line;
	while (getline(infile, line)) {
		if (line == "[header]") {
			if (!readHeader(infile)) {
				return 0;
			}
		}
		else if (line == "[layer]") {
			readLayerData(infile);
		}
		else if (line == "[object]") {
			readEntityData(infile);
		}
	}
	infile.close();

	//Creating Entities
	for (size_t i = 0; i < entities.size(); i++) {
		if (entities[i]->type == PLAYER) {
			entities[i]->sprite = playerSprite;
			entities[i]->width = playerSprite.width;
			entities[i]->height = playerSprite.height;
			entities[i]->speed[0] = 0;
			entities[i]->speed[1] = 0;
			entities[i]->acceleration[0] = 0;
			entities[i]->acceleration[1] = 0;
			entities[i]->friction[0] = 3;
			entities[i]->friction[1] = 1;
		}
		else if (entities[i]->type == COIN) {
			entities[i]->sprite = coinSprite;
			entities[i]->width = coinSprite.width;
			entities[i]->height = coinSprite.height;
		}
	};

	const Uint8 *keys = SDL_GetKeyboardState(NULL);

	SDL_Event event;
	bool done = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
		}
		if (keys[SDL_SCANCODE_A]) {
			entities[0]->speed[0] = -3.0f;
		}
		if (keys[SDL_SCANCODE_D]) {
			entities[0]->speed[0] = 3.0f;
		}
		//Ticks
		ticks = (float)SDL_GetTicks() / 1000.0f;
		elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;

		//Moving screen according to player movement
		viewMatrix.Identity();
		viewMatrix.Translate(-entities[0]->position[0], -entities[0]->position[1], 0);
		program->SetModelviewMatrix(modelMatrix);
		program->SetProjectionMatrix(projectionMatrix);
	}

	SDL_Quit();
	return 0;
}
