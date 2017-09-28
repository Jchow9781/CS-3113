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

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	#ifdef _WINDOWS
		glewInit();
	#endif

	//Setup
	glViewport(0, 0, 640, 360);

	ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	GLuint fireTexture = LoadTexture(RESOURCE_FOLDER"fire.png");
	GLuint tentTexture = LoadTexture(RESOURCE_FOLDER"tent.png");
	GLuint personTexture = LoadTexture(RESOURCE_FOLDER"person.png");

	glEnable(GL_BLEND);

	Matrix projectionMatrix;
	projectionMatrix.SetOrthoProjection(-3.55f, 3.55f, -2.0f, 2.0f, -1.0f, 1.0f);
	Matrix modelviewMatrix;

	SDL_Event event;
	bool done = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
		}

		glClear(GL_COLOR_BUFFER_BIT);

		glUseProgram(program.programID);

		program.SetModelviewMatrix(modelviewMatrix);
		program.SetProjectionMatrix(projectionMatrix);
		
		//Drawing person
		modelviewMatrix.Identity();
		modelviewMatrix.Translate(-1.1f, -0.5f, 0.0);
		modelviewMatrix.Scale(-1.0, 0.75f, 1.0);
		modelviewMatrix.Scale(1.1f, 1.1f, 1.1f);
		glBindTexture(GL_TEXTURE_2D, personTexture);
		program.SetModelviewMatrix(modelviewMatrix);

		float person_vertices[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, person_vertices);
		glEnableVertexAttribArray(program.positionAttribute);

		float person_tex_verticies[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, person_tex_verticies);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);

		//Drawing tent
		modelviewMatrix.Identity();
		modelviewMatrix.Translate(1.5f, 0.5f, 0.0);
		modelviewMatrix.Scale(2.0f, 2.0f, 0.0);
		program.SetModelviewMatrix(modelviewMatrix);
		glBindTexture(GL_TEXTURE_2D, tentTexture);

		float tent_vertices[] = { -1.0, -0.5, 1.0, -0.5, 1.0, 0.5, -1.0, -0.5, 1.0, 0.5, -1.0, 0.5 };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, tent_vertices);
		glEnableVertexAttribArray(program.positionAttribute);

		float tent_tex_verticies[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, tent_tex_verticies);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);

		//Drawing camp fire
		modelviewMatrix.Identity();
		modelviewMatrix.Translate(-0.0, -0.75f, 0.0);
		program.SetModelviewMatrix(modelviewMatrix);

		glBindTexture(GL_TEXTURE_2D, fireTexture);
		float fire_vertices[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, fire_vertices);
		glEnableVertexAttribArray(program.positionAttribute);

		float fire_tex_verticies[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, fire_tex_verticies);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);


		SDL_GL_SwapWindow(displayWindow);
	} 

	SDL_Quit();
	return 0;
}
