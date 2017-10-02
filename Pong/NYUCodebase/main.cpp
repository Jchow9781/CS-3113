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

struct Paddle {
	Paddle(float left, float right, float top, float bottom) : left(left), right(right), top(top), bottom(bottom) {}

	float left;
	float right;
	float top;
	float bottom;
};

struct Ball {
	Ball(float pos_x, float pos_y, float speed, float dir_x, float dir_y) : pos_x(pos_x), pos_y(pos_y), speed(speed), dir_x(dir_x), dir_y(dir_y) {}
	float pos_x = 0.0f;
	float pos_y = 0.0f;
	float speed = 0.1f;
	float dir_x = (float)rand();
	float dir_y = (float)rand();

	void reset() {
		float pos_x = 0.0f;
		float pos_y = 0.0f;
		float speed = 0.1f;
		float dir_x = (float)rand();
		float dir_y = (float)rand();
	}

	void move(float elapsed) {
		pos_x += (speed * dir_x * elapsed);
		pos_y += (speed * dir_y * elapsed);
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

	//Can't make an object have a solid white color so using white texture instead
	GLuint whiteTexture = LoadTexture(RESOURCE_FOLDER"white.png");

	glEnable(GL_BLEND);

	Matrix projectionMatrix;
	projectionMatrix.SetOrthoProjection(-3.55f, 3.55f, -2.0f, 2.0f, -1.0f, 1.0f);

	Matrix left_paddle_Matrix;
	Matrix right_paddle_Matrix;
	Matrix ball_Matrix;
	Matrix modelviewMatrix;

	float last_frame_ticks = 0.0f;

	Paddle left_paddle(-3.5f, -3.4f, 0.5f, -0.5f);
	Paddle right_paddle(3.4f, 3.5f, 0.5f, -0.5f);
	Ball ball(0.0f, 0.0f, 0.0001f, (float)rand(), (float)rand());

	SDL_Event event;
	bool done = false;
	bool game_running = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
			if (event.type == SDL_KEYDOWN) {
				//Start Game
				if (event.key.keysym.scancode == SDL_SCANCODE_SPACE && !game_running) {
					game_running = true;
				}
				//Left paddle 
				if (event.key.keysym.scancode == SDL_SCANCODE_W && left_paddle.top < 2.0f) {
					left_paddle.top += 0.3f;
					left_paddle.bottom += 0.3f;
					left_paddle_Matrix.Translate(0.0f, 0.3f, 0.0f);
				}
				if (event.key.keysym.scancode == SDL_SCANCODE_S && left_paddle.bottom > -2.0f) {
					left_paddle.top -= 0.3f;
					left_paddle.bottom -= 0.3f;
					left_paddle_Matrix.Translate(0.0f, -0.3f, 0.0f);
				}
				//Right paddle
				if (event.key.keysym.scancode == SDL_SCANCODE_UP && right_paddle.top < 2.0f) {
					right_paddle.top += 0.3f;
					right_paddle.bottom += 0.3f;
					right_paddle_Matrix.Translate(0.0f, 0.3f, 0.0f);
				}
				if (event.key.keysym.scancode == SDL_SCANCODE_DOWN && right_paddle.bottom > -2.0f) {
					right_paddle.top -= 0.3f;
					right_paddle.bottom -= 0.3f;
					right_paddle_Matrix.Translate(0.0f, -0.3f, 0.0f);
				}
			}
		}

		glClear(GL_COLOR_BUFFER_BIT);

		glUseProgram(program.programID);

		program.SetModelviewMatrix(left_paddle_Matrix);
		program.SetProjectionMatrix(projectionMatrix);

		//Drawing left paddle
		float left_paddle_vertices[] = { -3.4f, -0.5f, -3.2f, -0.5f, -3.2f, 0.5f, -3.2f, 0.5f, -3.4f, 0.5f, -3.4f, -0.5f };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, left_paddle_vertices);
		glEnableVertexAttribArray(program.positionAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);


		//Draw right paddle
		program.SetModelviewMatrix(right_paddle_Matrix);

		float right_paddle_vertices[] = { 3.4f, -0.5f, 3.2f, -0.5f, 3.2f, 0.5f, 3.2f, 0.5f, 3.4f, 0.5f, 3.4f, -0.5f };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, right_paddle_vertices);
		glEnableVertexAttribArray(program.positionAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);

		//Draw ball
		program.SetModelviewMatrix(ball_Matrix);
		float ball_vertices[] = { -0.1f, -0.1f, 0.1f, -0.1f, 0.1f, 0.1f, -0.1f, -0.1f, 0.1f, 0.1f, -0.1f, 0.1f };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, ball_vertices);
		glEnableVertexAttribArray(program.positionAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);


		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - last_frame_ticks;
		last_frame_ticks = ticks;

		//Game logic
		if (game_running) {
			//Ball hits top/bottom wall -> reverse y magnitude
			if (ball.pos_y + 0.1f >= 1.0f || ball.pos_y - 0.1f <= -1.0f) {
				ball.dir_y *= -1;
				ball.move(elapsed);
				ball_Matrix.Translate(ball.speed * ball.dir_x * elapsed, ball.speed * ball.dir_y * elapsed, 0.0f);
			}
			//Ball hits either paddle -> reverse x magnitude
			else if (ball.pos_x - 0.2f <= left_paddle.right && ball.pos_y <= left_paddle.top && ball.pos_y >= left_paddle.bottom ||
				ball.pos_x + 0.2f >= right_paddle.left && ball.pos_y <= right_paddle.top && ball.pos_y >= right_paddle.bottom) {
				ball.dir_x *= -1;
				ball.move(elapsed);
				ball_Matrix.Translate(ball.speed * ball.dir_x * elapsed, ball.speed * ball.dir_y * elapsed, 0.0f);
			}
			//Left side wins
			else if (ball.pos_x > right_paddle.left) {
				game_running = false;
				ball_Matrix.Translate(-ball.pos_x, -ball.pos_y, 0.0f);
				ball.reset();
				std::cout << "LEFT WINS!\n";
			}
			//Right side wins
			else if (ball.pos_x < left_paddle.right) {
				game_running = false;
				ball_Matrix.Translate(-ball.pos_x, -ball.pos_y, 0.0f);
				ball.reset();
				std::cout << "Right WINS!\n";
			}
			//Ball moves regularly
			else {
				ball.move(elapsed);
				ball_Matrix.Translate((ball.speed * ball.dir_x * elapsed), (ball.speed * ball.dir_y * elapsed), 0.0f);
			}
		}

		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}

