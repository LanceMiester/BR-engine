#include <stdlib.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 360

struct vec4_t
{
	float x;
	float y;
	float z;
	float w;
};
struct vec3_t
{
	float x;
	float y;
	float z;
};
struct vec3i_t
{
	int x;
	int y;
	int z;
};
struct vec2_t
{
	float x;
	float y;
};
struct triangle
{
	struct vec4_t p1;
	struct vec4_t p2;
	struct vec4_t p3;
	unsigned int id;
	bool render;
};

float speed = 0.5f;
float znear = 0.1f;
float zfar = 1000.0f;
float fov = 90.0f;
float aspr = (float)SCREEN_HEIGHT / (float)SCREEN_WIDTH;
float pitch = 1.0f; // in degrees
float yaw = 1.0f;	// in degrees
struct vec4_t camera;
struct vec4_t lookdir = {0, 0, 1, 1}, upv = {0, 1, 0, 1};
int framecount = 0;

int SetPixel(SDL_Surface *surface, int x, int y, uint8_t r, uint8_t g, uint8_t b)
{
	uint8_t *pixels = (uint8_t *)surface->pixels;
	pixels[y * surface->pitch + x * surface->format->BytesPerPixel] = r;
	pixels[y * surface->pitch + x * surface->format->BytesPerPixel + 1] = g;
	pixels[y * surface->pitch + x * surface->format->BytesPerPixel + 2] = b;
	return 0;
}

float calcedge(struct vec4_t p, struct vec4_t p2, struct vec4_t p3)
{
	float edges = (p2.x - p.x) * (p3.y - p.y) - (p2.y - p.y) * (p3.x - p.x);
	return edges;
}

// should I instead pass in the triangle structure? probably.
int Rasterize_Triangle(SDL_Surface *frame, struct vec4_t p[3], int color[3], float depthbuf[SCREEN_HEIGHT][SCREEN_WIDTH])
{

	float max_X = fmaxf(p[0].x, fmaxf(p[1].x, p[2].x));
	float max_Y = fmaxf(p[0].y, fmaxf(p[1].y, p[2].y));
	float min_X = fminf(p[0].x, fminf(p[1].x, p[2].x));
	float min_Y = fminf(p[0].y, fminf(p[1].y, p[2].y));

	struct vec4_t p1;

	for (int y = min_Y; y < max_Y; y++)
	{
		for (int x = min_X; x < max_X; x++)
		{
			if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT)
			{
				p1.y = y;
				p1.x = x;
				float ABP = calcedge(p[0], p[1], p1);
				float BCP = calcedge(p[1], p[2], p1);
				float CAP = calcedge(p[2], p[0], p1);
				if (ABP >= 0 && BCP >= 0 && CAP >= 0)
				{
					SetPixel(frame, (int)x, (int)y, (uint8_t)color[0], (uint8_t)color[1], (uint8_t)color[2]);
				}
			}
		}
	}

	return 0;
}

void printvec(struct vec4_t v, char *name)
{
	printf("%s : X %f | Y %f | Z %f | W %f\n", name, v.x, v.y, v.z, v.w);
}

// function to apply an offset to a triangle
struct triangle applyoff(struct triangle tri1, float x, float y, float z)
{
	struct triangle tri;
	tri.p1.x = x + tri1.p1.x;
	tri.p1.y = y + tri1.p1.y;
	tri.p1.z = z + tri1.p1.z;
	tri.p2.x = x + tri1.p2.x;
	tri.p2.y = y + tri1.p2.y;
	tri.p2.z = z + tri1.p2.z;
	tri.p3.x = x + tri1.p3.x;
	tri.p3.y = y + tri1.p3.y;
	tri.p3.z = z + tri1.p3.z;
	return tri;
}

int calc_color(float lum, int c[3])
{

	if (lum > 0.0f)
	{
	}
	else if (lum == -0.0f)
	{
		c[0] = c[0] * 0.6f;
		c[1] = c[1] * 0.6f;
		c[2] = c[2] * 0.6f;
	}
	else
	{
		c[0] = c[0] * 0.2f;
		c[1] = c[1] * 0.2f;
		c[2] = c[2] * 0.2f;
	}

	return 0;
}

void mat4_project_matrix(float fov, float asp, float znear, float zfar, float matrix[4][4])
{

	matrix[0][0] = asp * 1.0f / tanf(fov * 0.5f / 180.0f * 3.14159f);
	matrix[1][1] = 1.0f / tanf(fov * 0.5f / 180.0f * 3.14159f);
	matrix[2][2] = zfar / (zfar - znear);
	matrix[2][3] = (-zfar * znear) / (zfar - znear);
	matrix[3][2] = 1.0f;
	return;
}

void mat4_yrot_matrix(float angle, float m[4][4])
{
	float AngleRad = angle / 180.0f * 3.14159f;
	m[0][0] = cos(AngleRad);
	m[0][2] = sin(AngleRad);
	m[2][0] = -sin(AngleRad);
	m[1][1] = 1.0f;
	m[2][2] = cos(AngleRad);
	m[3][3] = 1.0f;
}

void mat4_xrot_matrix(float angle, float m[4][4])
{
	float AngleRad = angle / 180.0f * 3.14159f;
	m[0][0] = 1.0f;
	m[1][1] = cos(AngleRad);
	m[1][2] = -sin(AngleRad);
	m[2][1] = sin(AngleRad);
	m[2][2] = cos(AngleRad);
	m[3][3] = 1.0f;
}

struct vec4_t mat4_mul_vec4(float mat4[4][4], struct vec4_t vec)
{
	struct vec4_t result = {0};

	result.x = mat4[0][0] * vec.x + mat4[0][1] * vec.y + mat4[0][2] * vec.z + mat4[0][3] * vec.w;

	result.y = mat4[1][0] * vec.x + mat4[1][1] * vec.y + mat4[1][2] * vec.z + mat4[1][3] * vec.w;

	result.z = mat4[2][0] * vec.x + mat4[2][1] * vec.y + mat4[2][2] * vec.z + mat4[2][3] * vec.w;

	result.w = mat4[3][0] * vec.x + mat4[3][1] * vec.y + mat4[3][2] * vec.z + mat4[3][3] * vec.w;
	if (result.w != 0.0)
	{
		result.x /= result.w;
		result.y /= result.w;
		result.z /= result.w;
		result.z *= 0.5 + 0.5;
	}
	return result;
}

unsigned int nlines_begin_with(char *filename, char *v)
{
	int n = 0;
	char line[500];
	FILE *file = fopen(filename, "r");
	if (file == NULL)
	{
		return 1;
	}
	while (fgets(line, sizeof(line), file))
	{
		char string[500];
		sprintf(string, "%.1s", line);
		if (strcmp(string, v) == 0)
		{
			n++;
		}
	}
	printf("%d\n", n);
	return (n);
}

float dp(struct vec4_t a, struct vec4_t b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}
struct vec4_t vecadd(struct vec4_t a, struct vec4_t b)
{
	struct vec4_t result;
	result.x = a.x + b.x;
	result.y = a.y + b.y;
	result.z = a.z + b.z;
	result.w = 1;
	return result;
}
struct vec4_t vecmul(struct vec4_t a, struct vec4_t b)
{
	struct vec4_t result;
	result.x = a.x * b.x;
	result.y = a.y * b.y;
	result.z = a.z * b.z;
	result.w = 1;
	return result;
}
struct vec4_t vecsub(struct vec4_t a, struct vec4_t b)
{
	struct vec4_t result;
	result.x = a.x - b.x;
	result.y = a.y - b.y;
	result.z = a.z - b.z;
	result.w = 1;
	return result;
}
struct vec4_t normalize(struct vec4_t a)
{
	struct vec4_t normalized;
	float l = sqrtf(a.x * a.x + a.y * a.y + a.z * a.z);
	normalized.x = a.x / l;
	normalized.y = a.y / l;
	normalized.z = a.z / l;
	return normalized;
}
struct vec4_t cross(struct vec4_t a, struct vec4_t b)
{
	struct vec4_t cp;
	cp.x = a.y * b.z - a.z * b.y;
	cp.y = a.z * b.x - a.x * b.z;
	cp.z = a.x * b.y - a.y * b.x;
	return cp;
}

struct vec4_t calc_normal(struct vec4_t points3d[3])
{
	struct vec4_t normal, line[2];
	for (unsigned int i = 0; i < 2; i++)
	{
		line[i].x = points3d[i + 1].x - points3d[0].x;
		line[i].y = points3d[i + 1].y - points3d[0].y;
		line[i].z = points3d[i + 1].z - points3d[0].z;
	}
	normal.x = line[0].y * line[1].z - line[0].z * line[1].y;
	normal.y = line[0].z * line[1].x - line[0].x * line[1].z;
	normal.z = line[0].x * line[1].y - line[0].y * line[1].x;
	return (normal);
}

struct vec4_t vecmulfloat(struct vec4_t v, float f)
{
	struct vec4_t a = {v.x * f, v.y * f, v.z * f, 1.0f};
	return a;
}

// uses lookat matrix
void camerahandle(struct vec4_t cam, struct vec4_t target, struct vec4_t up, float mat4[4][4])
{

	up = normalize(up);

	struct vec4_t zax = normalize(vecsub(cam, target));

	struct vec4_t xax = normalize(cross(up, zax));

	struct vec4_t yax = cross(zax, xax);

	mat4[0][0] = xax.x;
	mat4[0][1] = yax.x;
	mat4[0][2] = zax.x;
	mat4[0][3] = 0;

	mat4[1][0] = xax.y;
	mat4[1][1] = yax.y;
	mat4[1][2] = zax.y;
	mat4[1][3] = 0;

	mat4[2][0] = xax.z;
	mat4[2][1] = yax.z;
	mat4[2][2] = zax.z;
	mat4[2][3] = 0;

	mat4[3][0] = -dp(xax, cam);
	mat4[3][1] = -dp(yax, cam);
	mat4[3][2] = -dp(zax, cam);
	mat4[3][3] = 1.0f;
}

// what's a for loop?
void mat4xmat4(float a[4][4], float b[4][4], float out[4][4])
{
	out[0][0] = a[0][0] * b[0][0] + a[1][0] * b[0][1] + a[2][0] * b[0][2] + a[3][0] * b[0][3];
	out[0][1] = a[0][0] * b[1][0] + a[1][0] * b[1][1] + a[2][0] * b[1][2] + a[3][0] * b[1][3];
	out[0][2] = a[0][0] * b[2][0] + a[1][0] * b[2][1] + a[2][0] * b[2][2] + a[3][0] * b[2][3];
	out[0][3] = a[0][0] * b[3][0] + a[1][0] * b[3][1] + a[2][0] * b[3][2] + a[3][0] * b[3][3];
	out[1][0] = a[0][1] * b[0][0] + a[1][1] * b[0][1] + a[2][1] * b[0][2] + a[3][1] * b[0][3];
	out[1][1] = a[0][1] * b[1][0] + a[1][1] * b[1][1] + a[2][1] * b[1][2] + a[3][1] * b[1][3];
	out[1][2] = a[0][1] * b[2][0] + a[1][1] * b[2][1] + a[2][1] * b[2][2] + a[3][1] * b[2][3];
	out[1][3] = a[0][1] * b[3][0] + a[1][1] * b[3][1] + a[2][1] * b[3][2] + a[3][1] * b[3][3];
	out[2][0] = a[0][2] * b[0][0] + a[1][2] * b[0][1] + a[2][2] * b[0][2] + a[3][2] * b[0][3];
	out[2][1] = a[0][2] * b[1][0] + a[1][2] * b[1][1] + a[2][2] * b[1][2] + a[3][2] * b[1][3];
	out[2][2] = a[0][2] * b[2][0] + a[1][2] * b[2][1] + a[2][2] * b[2][2] + a[3][2] * b[2][3];
	out[2][3] = a[0][2] * b[3][0] + a[1][2] * b[3][1] + a[2][2] * b[3][2] + a[3][2] * b[3][3];
	out[3][0] = a[0][3] * b[0][0] + a[1][3] * b[0][1] + a[2][3] * b[0][2] + a[3][0] * b[0][3];
	out[3][1] = a[0][3] * b[1][0] + a[1][3] * b[1][1] + a[2][3] * b[1][2] + a[3][0] * b[1][3];
	out[3][2] = a[0][3] * b[2][0] + a[1][3] * b[2][1] + a[2][3] * b[2][2] + a[3][0] * b[2][3];
	out[3][3] = a[0][3] * b[3][0] + a[1][3] * b[3][1] + a[2][3] * b[3][2] + a[3][0] * b[3][3];
}

bool checkpoints(struct vec4_t n[3], float projmat[4][4])
{

	for (int i = 0; i < 3; i++)
	{
		struct vec4_t b = mat4_mul_vec4(projmat, n[i]);
		if (b.x >= -1.5f && b.x <= 1.5f)
		{
			if (b.y >= -1.5f && b.y <= 1.5f)
			{
				return true;
			}
		}
	}
	return false;
}
// Jesus christ

int handle_triangles(unsigned int faces, SDL_Surface *frame, struct triangle tri[faces], float depthbuffer[SCREEN_HEIGHT][SCREEN_WIDTH], float projmat[4][4])
{

	float viewmatrix[4][4];
	struct vec4_t target = {0, 0, 1, 1}, up = {0, 1, 0, 1};
	float yrot4[4][4] = {{{0.0f}}};
	float xrot4[4][4] = {{{0.0f}}};
	mat4_yrot_matrix(yaw, yrot4);
	mat4_xrot_matrix(pitch, xrot4);

	lookdir = mat4_mul_vec4(yrot4, target);
	lookdir = mat4_mul_vec4(xrot4, lookdir);

	upv = mat4_mul_vec4(xrot4, up);
	target = vecadd(lookdir, camera);
	camerahandle(camera, target, upv, viewmatrix);
	struct vec4_t p3d[3];
	struct vec4_t p3do[3];

	for (unsigned int face = 0; face < faces; face++)
	{
		// instead of rewriting the entire program to run with the new rendering method, i'll just convert the triangles points into individual 4D vectors to use with the older functions.
		p3d[0] = vecadd(tri[face].p1, camera);
		p3d[0] = mat4_mul_vec4(viewmatrix, p3d[0]);
		p3d[1] = vecadd(tri[face].p2, camera);
		p3d[1] = mat4_mul_vec4(viewmatrix, p3d[1]);
		p3d[2] = vecadd(tri[face].p3, camera);
		p3d[2] = mat4_mul_vec4(viewmatrix, p3d[2]);

		p3do[0] = tri[face].p1;
		p3do[1] = tri[face].p2;
		p3do[2] = tri[face].p3;

		struct vec4_t normal = calc_normal(p3do);

		if (checkpoints(p3d, projmat) == true && dp(normal, vecadd(camera, p3do[0])) <= 0.0f)
		{
			struct vec4_t ld = {0.0f, 5.0f, 0.0f};
			ld = normalize(ld);
			float lum = normal.x * ld.x + normal.y * ld.y + normal.z * ld.z;

			int cl[3] = {255, 255, 255};
			calc_color(lum, cl);
			for (unsigned int i = 0; i < 3; i++)
			{

				p3d[i] = mat4_mul_vec4(projmat, p3d[i]);

				p3d[i].x = p3d[i].x + 1.0f;
				p3d[i].y = p3d[i].y + 1.0f;
				p3d[i].x *= 0.5f * SCREEN_WIDTH;
				p3d[i].y *= 0.5f * SCREEN_HEIGHT;
			}
			Rasterize_Triangle(frame, p3d, cl, depthbuffer);
		}
	}
}

unsigned int mapfaces(char *path)
{
	unsigned int n = 0;
	char line[500];
	FILE *file = fopen(path, "r");
	if (file == NULL)
	{
		return 1;
	}
	while (fgets(line, sizeof(line), file))
	{
		char *string = line;
		char *token;
		char objectpath[512];
		token = strtok(string, "|");

		sprintf(objectpath, "%s", token + 2);

		n += nlines_begin_with(objectpath, "f");
	}
	return n;
}

void tri_from_objfile(unsigned int faces, char *path, struct triangle tri[faces])
{
	unsigned int nvec = nlines_begin_with(path, "v");
	int vi = 0, fi = 0, vni = 0;

	struct vec4_t vec[nvec];
	struct vec3i_t face[faces];
	char line[500];

	FILE *objfile = fopen(path, "r");
	while (fgets(line, sizeof(line), objfile))
	{
		char *str[2];
		sprintf(str, "%.1s", line);

		if (strcmp(str, "v") == 0)
		{
			char *delim = " ";
			int vvi = 0;
			char *token = strtok(line, delim);

			while (token != NULL)
			{
				if (strcmp(token, "v") != 0)
				{
					if (vvi == 0)
					{
						vec[vi].x = atof(token);
					}
					else if (vvi == 1)
					{
						vec[vi].y = atof(token);
					}
					else if (vvi == 2)
					{
						vec[vi].z = atof(token);
						vi++;
					}
					vvi++;
				}
				token = strtok(NULL, delim);
			}
		}
		else if (strcmp(str, "f") == 0)
		{
			char *delim = " ";
			int ffi = 0;
			char *token = strtok(line, delim);

			while (token != NULL)
			{
				if (strcmp(token, "f") != 0)
				{
					int n1 = atoi(token) - 1;

					if (ffi == 0)
					{
						face[fi].x = n1;
					}
					else if (ffi == 1)
					{
						face[fi].y = n1;
					}
					else if (ffi == 2)
					{
						face[fi].z = n1;
						fi++;
					}
					ffi++;
				}
				token = strtok(NULL, delim);
			}
		}
	}

	for (unsigned int i = 0; i < faces; i++)
	{
		tri[i].p1.x = vec[face[i].x].x;
		tri[i].p1.y = vec[face[i].x].y;
		tri[i].p1.z = vec[face[i].x].z;
		tri[i].p2.x = vec[face[i].y].x;
		tri[i].p2.y = vec[face[i].y].y;
		tri[i].p2.z = vec[face[i].y].z;
		tri[i].p3.x = vec[face[i].z].x;
		tri[i].p3.y = vec[face[i].z].y;
		tri[i].p3.z = vec[face[i].z].z;
		tri[i].render = true;
	}
}

void readmap(unsigned int faces, char *path, struct triangle tri[faces])
{
	// line buffer
	char line[500];
	unsigned int iter = 0;
	FILE *file = fopen(path, "r");
	while (fgets(line, sizeof(line), file))
	{
		{
			char objectpath[512];
			char *token;
			char *linecpy = line;
			int facet;
			float x = 0, y = 0, z = 0;
			token = strtok(linecpy, "|");
			sprintf(objectpath, "%s", token + 2);
			facet = nlines_begin_with(objectpath, "f");
			struct triangle tribuf[facet];
			token = strtok(NULL, "|");
			char *goffbuf;
			goffbuf = strtok(token, " ");
			goffbuf = strtok(NULL, " ");
			for (unsigned int i = 0; i < 3; i++)
			{
				if (i == 0)
				{
					x = atof(goffbuf);
				}
				else if (i == 1)
				{
					y = atof(goffbuf);
				}
				else if (i == 2)
				{
					z = atof(goffbuf);
				}
				goffbuf = strtok(NULL, " ");
			}
			tri_from_objfile(facet, objectpath, tribuf);
			for (unsigned int i = 0; i < facet; i++)
			{
				printf("iter %d | i %d\n", iter, i);
				tri[iter] = applyoff(tribuf[i], x, y, z);
				iter++;
			}
		}
	}
}

struct vec4_t MoveTowards(struct vec4_t target)
{
	struct vec4_t direction = {0.0f, 0.0f, 1.0f, 1};

	direction = vecsub(camera, target);
	direction = vecmulfloat(direction, speed);

	camera.x -= direction.x;
	camera.z += direction.z;
}
struct vec4_t MoveAway(struct vec4_t target)
{
	struct vec4_t direction = {0.0f, 0.0f, 1.0f, 1};

	direction = vecsub(camera, target);
	direction = vecmulfloat(direction, speed);
	camera.x += direction.x;
	camera.z -= direction.z;
}
struct vec4_t moveside(struct vec4_t target, int d)
{
	struct vec4_t direction = {0.0f, 0.0f, 0.0f, 1};
	direction = vecsub(camera, target);
	direction = normalize(direction);

	struct vec4_t xax = normalize(cross(upv, direction));
	// Speed modifier
	direction = vecmulfloat(xax, speed);
	if (d == 1)
	{
		camera.x -= direction.x;
		camera.z += direction.z;
	}
	else
	{
		camera.x += direction.x;
		camera.z -= direction.z;
	}
}

int main(int argc, char *argv[])
{
	if (argc != 1)
	{
		bool forward, back, left, right, up, down;
		bool lleft, lright, lup, ldown;
		camera.x = 0.0f;
		camera.y = 0.0f;
		camera.z = 0.0f;
		unsigned int faces = mapfaces(argv[1]);
		struct triangle triangles[faces];
		readmap(faces, argv[1], triangles);
		SDL_Init(SDL_INIT_EVERYTHING);
		SDL_Window *wnd;
		SDL_Event evn;
		wnd = SDL_CreateWindow("BR_Engine tests", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
		SDL_Surface *frame = SDL_GetWindowSurface(wnd);
		int x, y;
		SDL_GetKeyboardFocus();
		float *projmat[4][4] = {{{0}}};
		mat4_project_matrix(fov, aspr, znear, zfar, projmat);
		// TODO add delta-time
		//  Frame and input handler
		double start_time = time(NULL);
		for (;;)
		{
			float depthbuf[SCREEN_HEIGHT][SCREEN_WIDTH] = {{{-1.0f}}};
			mat4_project_matrix(fov, aspr, znear, zfar, projmat);
			if (SDL_PollEvent(&evn))
			{
				if (evn.type == SDL_QUIT)
					break;
				// TODO: add better keyboard handling
				// Keyboard handling
				if (evn.type == SDL_KEYDOWN)
				{
					SDL_Keycode KEY = evn.key.keysym.sym;
					// fix to handle multiple being pressed
					if (KEY == SDLK_w)
					{
						forward = true;
					}
					if (KEY == SDLK_s)
					{
						back = true;
					}
					if (KEY == SDLK_d)
					{
						left = true;
					}
					if (KEY == SDLK_a)
					{
						right = true;
					}
					if (KEY == SDLK_SPACE)
					{
						up = true;
					}
					if (KEY == SDLK_LCTRL)
					{
						down = true;
					}
					if (KEY == SDLK_LCTRL)
					{
						down = true;
					}

					if (KEY == SDLK_LCTRL)
					{
						down = true;
					}
					if (KEY == SDLK_LEFT)
					{
						lleft = true;
					}
					if (KEY == SDLK_RIGHT)
					{
						lright = true;
					}
					if (KEY == SDLK_UP)
					{
						lup = true;
					}
					if (KEY == SDLK_DOWN)
					{
						ldown = true;
					}

					if (pitch < -90)
						pitch = -89.9f;
					if (pitch > 90)
						pitch = 89.9f;
					if (yaw < 0.0f)
						yaw = 360.0f;
					if (yaw > 360.0f)
						yaw = 0.0f;

					if (KEY == SDLK_q)
					{
						fov -= 1.0f;
					}
					if (KEY == SDLK_e)
					{
						fov += 1.0f;
					}

					if (forward)
					{
						MoveTowards(vecadd(lookdir, camera));
					}
					if (back)
					{
						MoveAway(vecadd(lookdir, camera));
					}

					if (left)
					{
						moveside(vecadd(lookdir, camera), 0);
					}

					if (right)
					{
						moveside(vecadd(lookdir, camera), 1);
					}
					if (up)
					{
						camera.y += 0.1f;
					}
					if (down)
					{
						camera.y -= 0.1f;
					}

					if (lleft)
					{
						yaw += 2.0f;
					}

					if (lright)
					{
						yaw -= 2.0f;
					}
					if (lup)
					{
						pitch -= 2.0f;
					}
					if (ldown)
					{
						pitch += 1.0f;
					}
				}
				if (evn.type == SDL_KEYUP)
				{

					SDL_Keycode KEY = evn.key.keysym.sym;
					if (KEY == SDLK_w)
					{
						forward = false;
					}

					if (KEY == SDLK_s)
					{
						back = false;
					}

					if (KEY == SDLK_d)
					{
						left = false;
					}

					if (KEY == SDLK_a)
					{
						right = false;
					}
					if (KEY == SDLK_SPACE)
					{
						up = false;
					}
					if (KEY == SDLK_LCTRL)
					{
						down = false;
					}

					if (KEY == SDLK_LEFT)
					{
						lleft = false;
					}

					if (KEY == SDLK_RIGHT)
					{
						lright = false;
					}
					if (KEY == SDLK_UP)
					{
						lup = false;
					}
					if (KEY == SDLK_DOWN)
					{
						ldown = false;
					}
				}
			}

			SDL_FillRect(frame, NULL, SDL_MapRGB(frame->format, 0, 0, 0));

			SDL_LockSurface(frame);

			handle_triangles(faces, frame, triangles, depthbuf, projmat);

			SDL_UnlockSurface(frame);

			SDL_UpdateWindowSurface(wnd);

			framecount++;
			double curtime = time(NULL);
			if (curtime - start_time >= 1.000000)
			{
				printf("%d fps \n", framecount);
				start_time = curtime;
				framecount = 0;
			}
		}

		SDL_DestroyWindow(wnd);
		SDL_Quit();
	}
	else
	{
		printf("Please include a map file to load\n");
	}
	return 0;
}
