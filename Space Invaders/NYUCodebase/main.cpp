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

//GAME VALUES
enum TYPE {PLAYER, ALIEN};
enum STATE {MAIN_MENU, IN_GAME};
ShaderProgram* program;

//Spaceship or alien
struct Entity {
	float position[2];
	float speed[2];
	float boundary[4];
	float u;
	float v;
	float width;
	float height;
	float size = 1.0f;
	Matrix entityMatrix;
	TYPE type;

	Entity() {}

	Entity(float x, float y, float spriteU, float spriteV, float spriteWidth, float spriteHeight, float dx, float dy) {
		position[0] = x;
		position[1] = y;
		speed[0] = dx;
		speed[1] = dy;
		entityMatrix.Identity();
		entityMatrix.Translate(x, y, 0);
		boundary[0] = y + 0.05f * size;
		boundary[1] = y - 0.05f * size;
		boundary[2] = x - 0.05f * size;
		boundary[3] = x + 0.05f * size;

		u = spriteU;
		v = spriteV;
		width = spriteWidth;
		height = spriteHeight;
	}

	void draw() {
		entityMatrix.Identity();
		entityMatrix.Translate(position[0], position[1], 0);
		program->SetModelviewMatrix(entityMatrix);
		//Make separate header folder for shader program and pass through pointer
		

		std::vector<float> vertexData;
		std::vector<float> texCoordData;
		float texture_x = u;
		float texture_y = v;
		vertexData.insert(vertexData.end(), {
			(-0.1f * size), 0.1f * size,
			(-0.1f * size), -0.1f * size,
			(0.1f * size), 0.1f * size,
			(0.1f * size), -0.1f * size,
			(0.1f * size), 0.1f * size,
			(-0.1f * size), -0.1f * size,
		});
		texCoordData.insert(texCoordData.end(), {
			texture_x, texture_y,
			texture_x, texture_y + height,
			texture_x + width, texture_y,
			texture_x + width, texture_y + height,
			texture_x + width, texture_y,
			texture_x, texture_y + height,
		});

		glUseProgram(program->programID);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
		glEnableVertexAttribArray(program->positionAttribute);
		glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
		glEnableVertexAttribArray(program->texCoordAttribute);
		glBindTexture(GL_TEXTURE_2D, spriteSheetTexture);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program->positionAttribute);
		glDisableVertexAttribArray(program->texCoordAttribute);
	}

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
	
	//Setup
	glViewport(0, 0, 1280, 720);
	ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	GLuint mainTexture = LoadTexture(RESOURCE_FOLDER"sheet.png");


	Matrix projectionMatrix;
	projectionMatrix.SetOrthoProjection(-3.55f, 3.55f, -2.0f, 2.0f, -1.0f, 1.0f);

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

		glUseProgram(program.programID);

		//Draw spaceship
		program.SetModelviewMatrix(spaceship_Matrix);
		program.SetProjectionMatrix(projectionMatrix);

		spaceship_Matrix.Identity();
		//glBindTexture(GL_TEXTURE_2D, whiteTexture);
		spaceship_Matrix.Translate(0, -1.75f, 0);
		//float spaceship_vertices[] = { -0.125f, -1.75f, 0.125f, -1.75f, 0.125f, -1.5f, -0.125f, -1.75f, 0.125f, -1.5f, -0.125f, -1.5f };
		float spaceship_vertices[] = { -0.125f, -0.125f, 0.125f, -0.125f, 0.125f, 0.125f, -0.125f, -0.125f, 0.125f, 0.125f, -0.125f, 0.125f };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, spaceship_vertices);
		glEnableVertexAttribArray(program.positionAttribute);

		float texCoords[] = { 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 1.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);

		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}
