#ifdef _WINDOWS
	#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <random>
#include "ShaderProgram.h"
#include "Matrix.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <vector>
#include <string>
#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;
ShaderProgram* program;
Matrix modelViewMatrix;
Matrix projectionMatrix;

//Game things
enum GameState { STATE_MAIN_MENU, STATE_LEVEL_ONE, STATE_LEVEL_TWO, STATE_LEVEL_THREE, STATE_GAME_WON, STATE_GAME_LOST};
enum TYPE {PLAYER, ASTEROID, LASER};
int state;
bool gameRunning = true;
float lastFrameTicks = 0.0f;
float elapsed;
float ticks;
float timeSinceLastFire;
float timeSinceLastAsteroid;
float screenShakeValue;
int asteroidsKilled = 0;
int health;
float fireDelay = 0.2f;

//Texture things
GLuint mainTexture;
GLuint fontTexture;
GLuint backgroundTexture;
GLuint coinTexture;
//float texture_coords[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f };

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
		entityMatrix.Rotate(270.0f * (3.1415926f / 180.0f));
		program->SetModelviewMatrix(entityMatrix);

		std::vector<float> vertices;
		std::vector<float> texCoords;
		float texture_x = u;
		float texture_y = v;
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

	void drawCoin() {
		entityMatrix.Identity();
		entityMatrix.Translate(position[0], position[1], 0);
		program->SetModelviewMatrix(entityMatrix);

		std::vector<float> vertices;
		std::vector<float> texCoords;
		float texture_x = u;
		float texture_y = v;
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
		glBindTexture(GL_TEXTURE_2D, coinTexture);
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
	float size = 1.5f;
	Matrix entityMatrix;
	TYPE type;
};

struct Particle {
	float position[2];
	float speed[2];
	float lifeTime;
	float sizeDeviation;
	float size;
};

struct ParticleEmitter {
	ParticleEmitter(unsigned int particleCount);

	ParticleEmitter();
	~ParticleEmitter();

	void Update(float elapsed);
	void draw() {
		std::vector<float> vertices;
		std::vector<float> texCoords;
		for (int i = 0; i < particles.size(); i++) {
			//float m = (particles[i].lifeTime / maxLifeTime);
			//float size = lerp(startSize, endSize, m) + particles[i].sizeDeviation;
			vertices.push_back(particles[i].position[0]);
			vertices.push_back(particles[i].position[1]);

			vertices.insert(vertices.end(), {
				particles[i].position[0] - size, particles[i].position[1] + size,
				particles[i].position[0] - size, particles[i].position[1] - size,
				particles[i].position[0] + size, particles[i].position[1] + size,
				particles[i].position[0] + size, particles[i].position[1] + size,
				particles[i].position[0] - size, particles[i].position[1] - size,
				particles[i].position[0] + size, particles[i].position[1] - size
			});

			texCoords.insert(texCoords.end(), {
				0.0f, 0.0f,
				0.0f, 1.0f,
				1.0f, 0.0f,
				1.0f, 0.0f,
				0.0f, 1.0f,
				1.0f, 1.0f
			});

			glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices.data());
			glEnableVertexAttribArray(program->positionAttribute);
			glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords.data());
			glEnableVertexAttribArray(program->texCoordAttribute);
			glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 2);
			
		}

		glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices.data());
		glEnableVertexAttribArray(program->positionAttribute);
		glDrawArrays(GL_POINTS, 0, vertices.size() / 2);
	};

	float position[2];
	float speed[2];
	float maxLifeTime;
	float startSize;
	float endSize;
	float sizeDeviation;
	float size = 1.0f;
	std::vector<Particle> particles;
};

Entity player;
Entity asteroid1;
std::vector<Entity> asteroids;
std::vector<Entity> playerLasers;
std::vector<Entity> coins;
std::vector<int> playerLasersToRemove;

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

void renderMainMenu() {
	modelViewMatrix.Identity();
	modelViewMatrix.Translate(-1.5f, 1.5f, 0.0f);
	//modelViewMatrix.Translate(0.0f, sin(screenShakeValue * 10) * 0.1f, 0.0f);
	program->SetModelviewMatrix(modelViewMatrix);
	drawText(program, fontTexture, "MAIN MENU", 0.2f, 0);	
}

void renderHUD() {
	//Render Health
	modelViewMatrix.Identity();
	modelViewMatrix.Translate(-3.5f, 1.9f, 0.0f);
	program->SetModelviewMatrix(modelViewMatrix);
	drawText(program, fontTexture, "REMAINING LIVES: ", 0.15f, -0.065f);
	modelViewMatrix.Identity();
	modelViewMatrix.Translate(-2.15f, 1.9f, 0.0f);
	program->SetModelviewMatrix(modelViewMatrix);
	drawText(program, fontTexture, std::to_string(health), 0.15f, -0.065f);

	//Render Asteroids to kill
	modelViewMatrix.Identity();
	modelViewMatrix.Translate(-3.5f, 1.75f, 0.0f);
	program->SetModelviewMatrix(modelViewMatrix);
	drawText(program, fontTexture, "ASTEROIDS LEFT: ", 0.15f, -0.065f);
	modelViewMatrix.Identity();
	modelViewMatrix.Translate(-2.2f, 1.75f, 0.0f);
	program->SetModelviewMatrix(modelViewMatrix);
	drawText(program, fontTexture, std::to_string(10 - asteroidsKilled), 0.15f, -0.065f);
	
	//Render Asteroids to kill
	modelViewMatrix.Identity();
	modelViewMatrix.Translate(-3.5f, 1.6f, 0.0f);
	program->SetModelviewMatrix(modelViewMatrix);
	drawText(program, fontTexture, "FIRE RATE DELAY: ", 0.15f, -0.065f);
	modelViewMatrix.Identity();
	modelViewMatrix.Translate(-2.1f, 1.6f, 0.0f);
	program->SetModelviewMatrix(modelViewMatrix);
	drawText(program, fontTexture, std::to_string(fireDelay ), 0.15f, -0.065f);
}

void renderBackground() {
	modelViewMatrix.Identity();
	modelViewMatrix.Scale(3.8f, 4.0f, 0);
	program->SetModelviewMatrix(modelViewMatrix);
	glBindTexture(GL_TEXTURE_2D, backgroundTexture);
	float background_vertices[] = { -1.0, -0.5, 1.0, -0.5, 1.0, 0.5, -1.0, -0.5, 1.0, 0.5, -1.0, 0.5 };
	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, background_vertices);
	glEnableVertexAttribArray(program->positionAttribute);

	float background_tex_verticies[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, background_tex_verticies);
	glEnableVertexAttribArray(program->texCoordAttribute);

	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
}

void renderGame() {
	player.draw();

	//Drawing asteroids
	for (size_t i = 0; i < asteroids.size(); i++) {
		asteroids[i].draw();
		asteroids[i].position[0] += asteroids[i].speed[0] * elapsed;
		asteroids[i].boundary[2] += asteroids[i].speed[0] * elapsed;
		asteroids[i].boundary[3] += asteroids[i].speed[0] * elapsed;
	}

	for (size_t i = 0; i < coins.size(); i++) {
		coins[i].drawCoin();
	}

	//Drawing player lasers
	for (size_t i = 0; i < playerLasers.size(); i++) {
		playerLasers[i].draw();
		playerLasers[i].position[0] += playerLasers[i].speed[0] * elapsed;
		playerLasers[i].boundary[2] += playerLasers[i].speed[0] * elapsed;
		playerLasers[i].boundary[3] += playerLasers[i].speed[0] * elapsed;

		//Delete lasors from vector if they fly outside screen
		if (playerLasers[i].boundary[3] > 4.0f) {
			playerLasers.erase(playerLasers.begin() + i);
		}
		for (size_t i = 0; i < playerLasersToRemove.size(); i++) {
			playerLasers.erase(playerLasers.begin() + playerLasersToRemove[i]);
		}
		playerLasersToRemove.clear();
	}
}

void renderLevelOne() {
	modelViewMatrix.Identity();
	modelViewMatrix.Translate(-1.5f, 1.5f, 0.0f);
	program->SetModelviewMatrix(modelViewMatrix);
	drawText(program, fontTexture, "LEVEL 1", 0.2f, 0.000001f);
}

void renderLevelTwo() {
	modelViewMatrix.Identity();
	modelViewMatrix.Translate(-1.5f, 1.5f, 0.0f);
	program->SetModelviewMatrix(modelViewMatrix);
	drawText(program, fontTexture, "LEVEL 2", 0.2f, 0.000001f);
}

void renderLevelThree() {
	modelViewMatrix.Identity();
	modelViewMatrix.Translate(-1.5f, 1.5f, 0.0f);
	program->SetModelviewMatrix(modelViewMatrix);
	drawText(program, fontTexture, "LEVEL THREE", 0.2f, 0.000001f);
}

void renderGameLost() {
	modelViewMatrix.Identity();
	modelViewMatrix.Translate(-1.5f, 1.5f, 0.0f);
	program->SetModelviewMatrix(modelViewMatrix);
	drawText(program, fontTexture, "YOU HAVE DIED :(", 0.2f, 0.000001f);
}

void renderGameWon() {
	modelViewMatrix.Identity();
	modelViewMatrix.Translate(-1.5f, 1.5f, 0.0f);
	program->SetModelviewMatrix(modelViewMatrix);
	drawText(program, fontTexture, "YOU WIN!", 0.2f, 0.000001f);
}

void reset() {
	health = 3;
	asteroidsKilled = 0;
	//delete every entity 
	asteroids.clear();
	playerLasers.clear();
}

void updateGame() {
	//Asteroid spawning
	if (timeSinceLastAsteroid > 1.5f) {
		asteroids.push_back(
			Entity(4.0f, -1.8f + (3.6f * rand()) / ((float)RAND_MAX + 1.0f), (224.0f / 1024.0f), (748.0f / 1024.0f), (101.0f / 1024.0f), (84.0f / 1024.0f), -(3.0f * rand()) / ((float)RAND_MAX + 1.0f), 0)
		);
		timeSinceLastAsteroid = 0;
	}

	//if (timeSinceLastAsteroid > 1.0f) {
		coins.push_back(
			Entity(0, 0, 0, 0, (40.0f / 1024.0f), (44.5f / 1024.0f), 0, 0)
		);
	

	//Laser-asteroid collision testing
	for (size_t i = 0; i < playerLasers.size(); i++) {
		for (size_t j = 0; j < asteroids.size(); j++) {
			if (playerLasers[i].boundary[3] > asteroids[j].boundary[2] &&
				playerLasers[i].boundary[0] > asteroids[j].boundary[1] &&
				playerLasers[i].boundary[1] < asteroids[j].boundary[0] /*&&
				playerLasers[i].boundary[2] < asteroids[j].boundary[3]*/) {
				playerLasersToRemove.push_back(i);
				asteroids.erase(asteroids.begin() + j);
				asteroidsKilled++;
			}
		}
	}

	for (size_t i = 0; i < asteroids.size(); i++) {
		if (asteroids[i].boundary[2] < -3.5f) {
			asteroids.erase(asteroids.begin() + i);
			--health;
		}
	}

	if (health < 0) {
		state = STATE_GAME_LOST;
	}

	if (asteroidsKilled >= 10) {
		asteroidsKilled = 0;
		switch (state) {
		case STATE_LEVEL_ONE:
			fireDelay = 0.8f;
			state = STATE_LEVEL_TWO;
			break;
		case STATE_LEVEL_TWO:
			fireDelay = 1.2f;
			state = STATE_LEVEL_THREE;
			break;
		case STATE_LEVEL_THREE:
			state = STATE_GAME_WON;
			break;
		}
	}
	for (size_t i = 0; i < asteroids.size(); i++) {
		if (asteroids[i].boundary[2] < -4.0f) {
			asteroids.erase(asteroids.begin() + i);
		}
	}
}

void render() {
	glClear(GL_COLOR_BUFFER_BIT);
	switch (state) {
	case STATE_MAIN_MENU:
		renderBackground();
		renderMainMenu();
		break;
	case STATE_LEVEL_ONE:
		renderBackground();
		renderLevelOne();
		renderGame();
		renderHUD();
		break;
	case STATE_LEVEL_TWO:
		renderBackground();
		renderLevelTwo();
		renderGame();
		renderHUD();
		break;
	case STATE_LEVEL_THREE:
		renderBackground();
		renderLevelThree();
		renderHUD();
		renderGame();
		break;
	case STATE_GAME_WON:
		renderBackground();
		renderGameWon();
		break;
	case STATE_GAME_LOST:
		renderBackground();
		renderGameLost();
		reset();
		break;
	default:
		break;
	}
	SDL_GL_SwapWindow(displayWindow);
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

	glViewport(0, 0, 1280, 720);
	program = new ShaderProgram(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
	projectionMatrix.SetOrthoProjection(-3.55f, 3.55f, -2.0f, 2.0f, -1.0f, 1.0f);
	program->SetModelviewMatrix(modelViewMatrix);
	program->SetProjectionMatrix(projectionMatrix);

	fontTexture = LoadTexture("font1.png");
	mainTexture = LoadTexture("sheet.png");
	coinTexture = LoadTexture("coins.png");
	backgroundTexture = LoadTexture("background.jpg");

	player = Entity(-3.4f, 0, (346.0f / 1024.0f), (75.0f / 1024.0f), (98.0f / 1024.0f), (75.0f / 1024.0f), 3.0f, 3.0f);
	health = 3;
	
	//asteroid1 = Entity(2.0f, -1.8f + (3.6f * rand())/((float)RAND_MAX + 1.0f) , (224.0f / 1024.0f), (748.0f / 1024.0f), (101.0f / 1024.0f), (84.0f / 1024.0f), -3.0f, 0);
	//asteroids.push_back(
	//	Entity(2.0f, -1.8f + (3.6f * rand()) / ((float)RAND_MAX + 1.0f), (224.0f / 1024.0f), (748.0f / 1024.0f), (101.0f / 1024.0f), (84.0f / 1024.0f), -2.0f, 0)
	//);
	const Uint8 *keys = SDL_GetKeyboardState(NULL);

	SDL_Event event;
	bool done = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
		}
		if (keys[SDL_SCANCODE_RETURN]) {
			if (state == STATE_GAME_LOST) {
				state = STATE_MAIN_MENU;
			}
		}
		if (keys[SDL_SCANCODE_SPACE]) {
			if (state == STATE_MAIN_MENU) {
				state = STATE_LEVEL_ONE;
			} else if (timeSinceLastFire > fireDelay) {
				playerLasers.push_back(Entity(player.position[0], player.position[1], 843.0f / 1024.0f, 426.0f / 1024.0f, 13.0f / 1024.0f, 54.0f / 1024.0f, 4.0f, 0));
				timeSinceLastFire = 0;
			}
		}
		if (keys[SDL_SCANCODE_S] && player.boundary[1] > -1.9f) {
			player.position[1] -= player.speed[1] * elapsed;
			player.boundary[0] -= player.speed[1] * elapsed;
			player.boundary[1] -= player.speed[1] * elapsed;
			player.boundary[2] -= player.speed[1] * elapsed;
			player.boundary[3] -= player.speed[1] * elapsed;
		}
		if (keys[SDL_SCANCODE_W] && player.boundary[0] < 1.9f) {
			player.position[1] += player.speed[1] * elapsed;
			player.boundary[0] += player.speed[1] * elapsed;
			player.boundary[1] += player.speed[1] * elapsed;
			player.boundary[2] += player.speed[1] * elapsed;
			player.boundary[3] += player.speed[1] * elapsed;
		}

		render();
		updateGame();

		ticks = (float)SDL_GetTicks() / 1000.0f;
		elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;
		timeSinceLastFire += elapsed;
		timeSinceLastAsteroid += elapsed;
		screenShakeValue += elapsed;
	}

	SDL_Quit();
	return 0;
}
