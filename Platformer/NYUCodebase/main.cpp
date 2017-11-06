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

enum Type {PLAYER, ENEMY, BLOCK};

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
		/*boundaries[0] = y + 0.05f * size[1] * 2;
		boundaries[1] = y - 0.05f * size[1] * 2;
		boundaries[2] = x - 0.05f * size[0] * 2;
		boundaries[3] = x + 0.05f * size[0] * 2;*/
		u = sprite_u;
		v = sprite_v;
		width = sprite_width;
		height = sprite_height;
		type = obj_type;
 	}

	float position[2]; //x,y
	float boundaries[4];	//up, down, left, right
	float size[2];	//length, width
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

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	#ifdef _WINDOWS
		glewInit();
	#endif

	SDL_Event event;
	bool done = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
		}
		glClear(GL_COLOR_BUFFER_BIT);
		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}
