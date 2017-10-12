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

#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;

//TEXTURE THINGS
GLuint mainTexture;
Matrix projectionMatrix;
Matrix modelViewMatrix;
struct Entity;
float texture_coords[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f };

//GAME VALUES
enum TYPE { PLAYER, INVADER, LASER };
enum STATE { MAIN_MENU, IN_GAME };
ShaderProgram* program;
float lastFrameTicks = 0.0f;
float elapsed;
float timeSinceLastFire = 0.0f;
float timeSinceLastEnemyFire = 0.0f;

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

//Spaceship or invader
struct Entity {
	Entity() {}

	Entity(float pos_x, float pos_y, float spriteU, float spriteV, float spriteWidth, float spriteHeight, float dir_x, float dir_y) {
		position[0] = pos_x;
		position[1] = pos_y;
		speed[0] = dir_x;
		speed[1] = dir_y;
		entityMatrix.Identity();
		entityMatrix.Translate(pos_x, pos_y, 0);
		boundary[0] = pos_y + 0.05f * size;
		boundary[1] = pos_y - 0.05f * size;
		boundary[2] = pos_x - 0.05f * size;
		boundary[3] = pos_x + 0.05f * size;
		u = spriteU;
		v = spriteV;
		width = spriteWidth;
		height = spriteHeight;
	}

	void draw() {
		entityMatrix.Identity();
		entityMatrix.Translate(position[0], position[1], 0);
		program->SetModelviewMatrix(entityMatrix);		

		std::vector<float> vertices;
		std::vector<float> texCoords;
		float texture_x = u;
		float texture_y = v;
		//Aspect ratio of img is 1024/1024 = 1 so I don't need to include it
		vertices.insert(vertices.end(), {
			-0.10f * size, -0.10f * size,
			0.10f * size, 0.10f * size,
			-0.10f * size, 0.10f * size,
			0.10f * size, 0.10f * size,
			-0.10f * size, -0.10f * size ,
			0.10f * size, -0.10f * size
		});
		texCoords.insert(texCoords.end(), {
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

		glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices.data());
		glEnableVertexAttribArray(program->positionAttribute);
		glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords.data());
		glEnableVertexAttribArray(program->texCoordAttribute);
		glBindTexture(GL_TEXTURE_2D, mainTexture);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program->positionAttribute);
		glDisableVertexAttribArray(program->texCoordAttribute);
	}

	float position[2]; //dir_x, dir_y]
	float speed[2]; //[dir_x, dir_y]
	float boundary[4]; //[up, down, left, right]
	float u;
	float v;
	float width;
	float height;
	float size = 1.0f;
	Matrix entityMatrix;
	TYPE type;
};

//Create all Entities
Entity player;
std::vector<Entity> invaders;
std::vector<Entity> playerLasers;
std::vector<Entity> invaderLasers;


int main(int argc, char *argv[]) {
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
	const Uint8 *keys = SDL_GetKeyboardState(NULL);

	player = Entity(0.0f, -1.8f, (346.0f / 1024.0f), (75.0f / 1024.0f), (98.0f / 1024.0f), (75.0f / 1024.0f), 4.0f, 0);
	for (int i = 0; i < 50; i++) {
		invaders.push_back(Entity(-2.4 + (i % 10) * 0.5, 1.8 - (i / 10 * 0.5), 120.0f / 1024.0f, 604.0f / 1024.0f, 104.0f / 1024.0f, 84.0f / 1024.0f, 0.0f, 0.0f));
	}
	

	SDL_Event event;
	bool done = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
			
			
		}
		if (keys[SDL_SCANCODE_SPACE] && timeSinceLastFire > 0.15f) {
			playerLasers.push_back(Entity(player.position[0], player.position[1], 843.0f / 1024.0f, 426.0f / 1024.0f, 13.0f / 1024.0f, 54.0f / 1024.0f, 0, 4.0f));
			timeSinceLastFire = 0;
		}
		if (keys[SDL_SCANCODE_A] && player.boundary[2] > -3.4f) {
			player.position[0] -= player.speed[0] * elapsed;
			player.boundary[2] -= player.speed[0] * elapsed;
			player.boundary[3] -= player.speed[0] * elapsed;
		}
		if (keys[SDL_SCANCODE_D] && player.boundary[3] < 3.4f) {
			player.position[0] += player.speed[0] * elapsed;
			player.boundary[2] += player.speed[0] * elapsed;
			player.boundary[3] += player.speed[0] * elapsed;
		}
		

		float ticks = (float)SDL_GetTicks() / 1000.0f;
		elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;
		timeSinceLastFire += elapsed;
		timeSinceLastEnemyFire += elapsed;

		glClear(GL_COLOR_BUFFER_BIT);

		player.draw();
		for (int i = 0; i < invaders.size(); i++) {
			invaders[i].draw();
		}
		for (int i = 0; i < playerLasers.size(); i++) {
			playerLasers[i].draw();
			playerLasers[i].position[1] += playerLasers[i].speed[1] * elapsed;
			playerLasers[i].boundary[0] += playerLasers[i].speed[1] * elapsed;
			playerLasers[i].boundary[1] += playerLasers[i].speed[1] * elapsed;
			if (playerLasers[i].position[1] > 2.0f) {
				playerLasers.erase(playerLasers.begin() + i);
			}
		}
		SDL_GL_SwapWindow(displayWindow);

		glUseProgram(program->programID);
	}

	SDL_Quit();
	return 0;
}
