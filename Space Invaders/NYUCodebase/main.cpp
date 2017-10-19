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
GLuint fontTexture;
Matrix projectionMatrix;
Matrix modelViewMatrix;
struct Entity;
float texture_coords[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f };

//GAME THINGS
enum TYPE { PLAYER, INVADER, LASER };
enum STATE { MAIN_MENU, IN_GAME, GAME_OVER};
int state;
bool gameRunning = true;
ShaderProgram* program;
float lastFrameTicks = 0.0f;
float elapsed;
float ticks;
float timeSinceLastFire = 0.0f;
float timeSinceLastInvaderFire = 0.0f;

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

//Spaceship, invader, or bullet
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
std::vector<int> playerLasersToRemove;

void drawGame() {
	player.draw();
	for (size_t i = 0; i < invaders.size(); i++) {
		invaders[i].draw();
	}
	for (size_t i = 0; i < playerLasers.size(); i++) {
		playerLasers[i].draw();
		playerLasers[i].position[1] += playerLasers[i].speed[1] * elapsed;
		playerLasers[i].boundary[0] += playerLasers[i].speed[1] * elapsed;
		playerLasers[i].boundary[1] += playerLasers[i].speed[1] * elapsed;
		//Delete lasors from vector if they fly outside screen
		if (playerLasers[i].position[1] > 2.0f) {
			playerLasers.erase(playerLasers.begin() + i);
		}
		for (size_t i = 0; i < playerLasersToRemove.size(); i++) {
			playerLasers.erase(playerLasers.begin() + playerLasersToRemove[i]);
		}
		playerLasersToRemove.clear();
	}
	for (size_t i = 0; i < invaderLasers.size(); i++) {
		invaderLasers[i].draw();
		invaderLasers[i].position[1] += invaderLasers[i].speed[1] * elapsed;
		invaderLasers[i].boundary[0] += invaderLasers[i].speed[1] * elapsed;
		invaderLasers[i].boundary[1] += invaderLasers[i].speed[1] * elapsed;
		//Delete lasors from vector if they fly outside screen
		if (invaderLasers[i].position[1] < -2.0f) {
			invaderLasers.erase(invaderLasers.begin() + i);
		}
	}
}

void drawText(ShaderProgram* program, int fontTexture, std::string text, float size, float spacing) {
	float texture_size = 1.0 / 16.0f;
	std::vector<float> vertexData;
	std::vector<float> texCoordData;

	for (size_t i = 0; i < text.size(); i++) {
		float texture_x = (float)(((int)text[i]) % 16) / 16.0f;
		float texture_y = (float)(((int)text[i]) / 16) / 16.0f;
		vertexData.insert(vertexData.end(), {
			((size + spacing) * i) + (-0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
		});
		texCoordData.insert(texCoordData.end(), {
			texture_x, texture_y,
			texture_x, texture_y + texture_size,
			texture_x + texture_size, texture_y,
			texture_x + texture_size, texture_y + texture_size,
			texture_x + texture_size, texture_y,
			texture_x, texture_y + texture_size,
		});
	}
	glUseProgram(program->programID);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
	glEnableVertexAttribArray(program->positionAttribute);
	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
	glEnableVertexAttribArray(program->texCoordAttribute);
	glBindTexture(GL_TEXTURE_2D, fontTexture);
	glDrawArrays(GL_TRIANGLES, 0, text.size() * 6);

	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
}

void drawMenu() {
	modelViewMatrix.Identity();
	modelViewMatrix.Translate(-1.5f, 1.5f, 0.0f);
	program->SetModelviewMatrix(modelViewMatrix);
	drawText(program, fontTexture, "SPACE", 0.8f, 0.0001f);
	
	modelViewMatrix.Identity();
	modelViewMatrix.Translate(-2.8f, 0.8f, 0.0f);
	program->SetModelviewMatrix(modelViewMatrix);
	drawText(program, fontTexture, "INVADERS", 0.8f, 0.0001f);

	modelViewMatrix.Identity();
	modelViewMatrix.Translate(-2.87f, -0.5f, 0.0f);
	program->SetModelviewMatrix(modelViewMatrix);
	drawText(program, fontTexture, "A and S TO MOVE, SPACE TO FIRE", 0.2f, 0.000001f);

	modelViewMatrix.Identity();
	modelViewMatrix.Translate(-1.85f, -1.4f, 0.0f);
	program->SetModelviewMatrix(modelViewMatrix);
	drawText(program, fontTexture, "PRESS SPACE TO BEGIN", 0.2f, 0.0001f);
}

void updateGame(){
	//Make each invader move left and right
	for (size_t i = 0; i < invaders.size(); i++) {
		invaders[i].position[0] += invaders[i].speed[0] * elapsed;
		invaders[i].boundary[2] += invaders[i].speed[0] * elapsed;
		invaders[i].boundary[3] += invaders[i].speed[0] * elapsed;

		if (invaders[i].boundary[2] < -3 || invaders[i].boundary[3] > 3) {
			invaders[i].speed[0] *= -1;
		}
	}

	//Check to make sure that dead invaders are not firing and add delay between firing
	if (timeSinceLastInvaderFire > 0.4f) {
		invaderLasers.push_back(Entity(invaders[rand() % invaders.size()].position[0], invaders[rand() % invaders.size()].position[1], 602.0f / 1024.0f, 600.0f / 1024.0f, 48.0f / 1024.0f, 46.0f / 1024.0f, 0, -1.0f));
		timeSinceLastInvaderFire = 0;
	}

	//Collision detection between lasers and objects
	//player laser and invader
	for (size_t i = 0; i < playerLasers.size(); i++) {
		for (size_t j = 0; j < invaders.size(); j++) {
			if (invaders[j].boundary[0] > playerLasers[i].boundary[1] &&
				invaders[j].boundary[1] < playerLasers[i].boundary[0] &&
				invaders[j].boundary[2] < playerLasers[i].boundary[3] &&
				invaders[j].boundary[3] > playerLasers[i].boundary[2]) {
				playerLasersToRemove.push_back(i);
				invaders.erase(invaders.begin() + j);
			}
		}
	}
	//Invader laser and player
	for (size_t i = 0; i < invaderLasers.size(); i++) {
		if (player.boundary[0] > invaderLasers[i].boundary[1] &&
			player.boundary[1] < invaderLasers[i].boundary[0] &&
			player.boundary[2] < invaderLasers[i].boundary[3] &&
			player.boundary[3] > invaderLasers[i].boundary[2]) {
			
			state = MAIN_MENU;
		}
	}
	if (invaders.size() == 0) {
		//gameRunning = false;
		state = MAIN_MENU;
	}
}

void updateMainMenu() {

}

void render() {
	glClear(GL_COLOR_BUFFER_BIT);
	switch (state) {
	case MAIN_MENU:
		drawMenu();
		break;
	case IN_GAME:
		drawGame();
		break;
	}
	SDL_GL_SwapWindow(displayWindow);
}

void update() {
	switch (state) {
	case MAIN_MENU:
		updateMainMenu();
		break;
	case IN_GAME:
		updateGame();
		break;
	}
}

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
	fontTexture = LoadTexture("font1.png");

	const Uint8 *keys = SDL_GetKeyboardState(NULL);

	//Creating objects for each entity
	player = Entity(0.0f, -1.8f, (346.0f / 1024.0f), (75.0f / 1024.0f), (98.0f / 1024.0f), (75.0f / 1024.0f), 4.0f, 0);
	for (int i = 0; i < 50; i++) {
		invaders.push_back(Entity(-2.4 + (i % 10) * 0.5, 1.8 - (i / 10 * 0.5), 120.0f / 1024.0f, 604.0f / 1024.0f, 104.0f / 1024.0f, 84.0f / 1024.0f, 1.25f, 0.0f));
	}

	SDL_Event event;
	bool done = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
		}
		if (keys[SDL_SCANCODE_SPACE]) {
			if (state == MAIN_MENU) {
				state = IN_GAME;
			} else if (timeSinceLastFire > 0.15f){
				playerLasers.push_back(Entity(player.position[0], player.position[1], 843.0f / 1024.0f, 426.0f / 1024.0f, 13.0f / 1024.0f, 54.0f / 1024.0f, 0, 4.0f));
				timeSinceLastFire = 0;
			}
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

		//Dealing with time
		ticks = (float)SDL_GetTicks() / 1000.0f;
		elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;
		timeSinceLastFire += elapsed;
		timeSinceLastInvaderFire += elapsed;

		if (gameRunning) {
			update();
			render();
		}
	}

	SDL_Quit();
	return 0;
}
