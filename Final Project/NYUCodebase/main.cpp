#ifdef _WINDOWS
	#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "ShaderProgram.h"
#include "Matrix.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <vector>
#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;
ShaderProgram* program;
Matrix modelViewMatrix;
Matrix projectionMatrix;

GLuint fontTexture;

enum GameState { STATE_MAIN_MENU, STATE_LEVEL_ONE, STATE_LEVEL_TWO, STATE_LEVEL_THREE, STATE_GAME_WON, STATE_GAME_LOST};
int state;
bool gameRunning = true;

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
	program->SetModelviewMatrix(modelViewMatrix);
	drawText(program, fontTexture, "MAIN MENU", 0.2f, 0.000001f);
}

void renderLevelOne() {
	modelViewMatrix.Identity();
	modelViewMatrix.Translate(-1.5f, 1.5f, 0.0f);
	program->SetModelviewMatrix(modelViewMatrix);
	drawText(program, fontTexture, "LEVEL ONE", 0.2f, 0.000001f);
}

void renderLevelTwo() {
	modelViewMatrix.Identity();
	modelViewMatrix.Translate(-1.5f, 1.5f, 0.0f);
	program->SetModelviewMatrix(modelViewMatrix);
	drawText(program, fontTexture, "LEVEL TWO", 0.2f, 0.000001f);
}

void renderLevelThree() {
	modelViewMatrix.Identity();
	modelViewMatrix.Translate(-1.5f, 1.5f, 0.0f);
	program->SetModelviewMatrix(modelViewMatrix);
	drawText(program, fontTexture, "LEVEL THREE", 0.2f, 0.000001f);
}

void renderGameWon() {
	modelViewMatrix.Identity();
	modelViewMatrix.Translate(-1.5f, 1.5f, 0.0f);
	program->SetModelviewMatrix(modelViewMatrix);
	drawText(program, fontTexture, "YOU HAVE DIED :(", 0.2f, 0.000001f);
}

void renderGameLost() {
	modelViewMatrix.Identity();
	modelViewMatrix.Translate(-1.5f, 1.5f, 0.0f);
	program->SetModelviewMatrix(modelViewMatrix);
	drawText(program, fontTexture, "YOU WIN!", 0.2f, 0.000001f);
}

void render() {
	glClear(GL_COLOR_BUFFER_BIT);
	switch (state) {
	case STATE_MAIN_MENU:
		renderMainMenu();
		break;
	case STATE_LEVEL_ONE:
		renderLevelOne();
		break;
	case STATE_LEVEL_TWO:
		renderLevelTwo();
		break;
	case STATE_LEVEL_THREE:
		renderLevelThree();
		break;
	case STATE_GAME_WON:
		renderGameWon();
		break;
	case STATE_GAME_LOST:
		renderGameLost();
		break;
	default:
		break;
	}
	SDL_GL_SwapWindow(displayWindow);
}

class Entity {

};

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

	SDL_Event event;
	bool done = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
			switch (event.type) {
				case SDL_KEYDOWN:
					if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
						if (state == STATE_MAIN_MENU) {
							state = STATE_LEVEL_ONE;
						}
					}
			}
		}

		if (gameRunning) {
			render();
		}
	}

	SDL_Quit();
	return 0;
}
