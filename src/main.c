#include <stdlib.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
// the current screens width
#define SCREEN_WIDTH 640
// the current screens height
#define SCREEN_HEIGHT 360

// making 2d, 3d and 4d vectors
struct vec4_t
{
	float x;
	float y;
	float z;
	float w;
}; // 4D float vector.
struct vec3_t
{
	float x;
	float y;
	float z;
}; // 3D float vector for specific use cases, If not used will be removed
struct vec3i_t
{
	int x;
	int y;
	int z;
}; // 3D integer version for specfic use cases
struct vec2_t
{
	float x;
	float y;
}; // 2D float vector.
struct triangle
{
	struct vec4_t p1;
	struct vec4_t p2;
	struct vec4_t p3;
	int id;
}; // triangle structure to differentiate triangles with ID

// defining some variables
float s = 0.5f;											  // speed scalar
float znear = 0.1f;										  // z near
float zfar = 1000.0f;									  // z far
float fov = 90.0f;										  // field of view
float aspr = (float)SCREEN_HEIGHT / (float)SCREEN_WIDTH;  // aspect ratio
float pitch = 1.0f;										  // pitch in degrees
float yaw = 1.0f;										  // yaw in degrees
struct vec4_t camera;									  // for calculating the camera position.
struct vec4_t lookdir = {0, 0, 1, 1}, upv = {0, 1, 0, 1}; // to calculate where the camera is looking
SDL_Renderer *rndr;										  // renderer
int framecount = 0;										  // variable to get current fps

// function to print 4D vectors for debugging
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

// calculate the color based on the luminosity
int calc_color(float lum, int c[3])
{
	// lit
	if (lum > 0.0f)
	{
		// no processing needed because lighting system is simple
	}
	// some weird middle ground?
	else if (lum == -0.0f)
	{
		// darken colors by 40%
		for (unsigned int i = 0; i < 3; i++)
		{
			c[i] = c[i] * 0.6f;
		}
	}
	// unlit
	else
	{
		// darken colors by 80%
		for (unsigned int i = 0; i < 3; i++)
		{
			c[i] = c[i] * 0.2f;
		}
	}

	return 0;
}

// projection matrix function
void mat4_project_matrix(float fov, float asp, float znear, float zfar, float matrix[4][4])
{

	matrix[0][0] = asp * 1.0f / tanf(fov * 0.5f / 180.0f * 3.14159f);
	matrix[1][1] = 1.0f / tanf(fov * 0.5f / 180.0f * 3.14159f);
	matrix[2][2] = zfar / (zfar - znear);
	matrix[2][3] = (-zfar * znear) / (zfar - znear);
	matrix[3][2] = 1.0f;
	return;
}

// creates a y rotation matrix based on a yaw value in degrees
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
// creates a x rotation matrix based on a pitch value in degrees
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

// Function to do 4x4 * 1x4 matrix multiplication
struct vec4_t mat4_mul_vec4(float mat4[4][4], struct vec4_t vec)
{
	struct vec4_t result = {0};
	// first row
	result.x = mat4[0][0] * vec.x + mat4[0][1] * vec.y + mat4[0][2] * vec.z + mat4[0][3] * vec.w;
	// second row
	result.y = mat4[1][0] * vec.x + mat4[1][1] * vec.y + mat4[1][2] * vec.z + mat4[1][3] * vec.w;
	// third row
	result.z = mat4[2][0] * vec.x + mat4[2][1] * vec.y + mat4[2][2] * vec.z + mat4[2][3] * vec.w;
	// fourth row
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

// check how many lines a file has that starts with a character
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

// math functions
// returns a dot product of two vectors
float dp(struct vec4_t a, struct vec4_t b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}
// adds two vectors together
struct vec4_t vecadd(struct vec4_t a, struct vec4_t b)
{
	struct vec4_t result;
	result.x = a.x + b.x;
	result.y = a.y + b.y;
	result.z = a.z + b.z;
	result.w = 1;
	return result;
}
// multiplies two vectors
struct vec4_t vecmul(struct vec4_t a, struct vec4_t b)
{
	struct vec4_t result;
	result.x = a.x * b.x;
	result.y = a.y * b.y;
	result.z = a.z * b.z;
	result.w = 1;
	return result;
}
// subtracts two vectors
struct vec4_t vecsub(struct vec4_t a, struct vec4_t b)
{
	struct vec4_t result;
	result.x = a.x - b.x;
	result.y = a.y - b.y;
	result.z = a.z - b.z;
	result.w = 1;
	return result;
}
// normalizes a vector
struct vec4_t normalize(struct vec4_t a)
{
	struct vec4_t normalized;
	float l = sqrtf(a.x * a.x + a.y * a.y + a.z * a.z);
	normalized.x = a.x / l;
	normalized.y = a.y / l;
	normalized.z = a.z / l;
	return normalized;
}
// gets the cross product of 2 vecors
struct vec4_t cross(struct vec4_t a, struct vec4_t b)
{
	struct vec4_t cp;
	cp.x = a.y * b.z - a.z * b.y;
	cp.y = a.z * b.x - a.x * b.z;
	cp.z = a.x * b.y - a.y * b.x;
	return cp;
}
// Calculate normal using Cross Product and line data
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
// scales a vector with a float value
struct vec4_t vecmulfloat(struct vec4_t v, float f)
{
	struct vec4_t a = {v.x * f, v.y * f, v.z * f, 1.0f};
	return a;
}

// handle the camera movement
void camerahandle(struct vec4_t cam, struct vec4_t target, struct vec4_t up, float mat4[4][4])
{

	// Look at matrix creation
	// normalize up vector
	up = normalize(up);

	// get the forward vector
	struct vec4_t zax = normalize(vecsub(cam, target));

	// get the right vector
	struct vec4_t xax = normalize(cross(up, zax));

	// Get the up vector
	struct vec4_t yax = cross(zax, xax);

	// Create view matrix

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

// for loop whats that again?
void mat4xmat4(float a[4][4], float b[4][4], float out[4][4])
{
	// first row
	out[0][0] = a[0][0] * b[0][0] + a[1][0] * b[0][1] + a[2][0] * b[0][2] + a[3][0] * b[0][3];
	out[0][1] = a[0][0] * b[1][0] + a[1][0] * b[1][1] + a[2][0] * b[1][2] + a[3][0] * b[1][3];
	out[0][2] = a[0][0] * b[2][0] + a[1][0] * b[2][1] + a[2][0] * b[2][2] + a[3][0] * b[2][3];
	out[0][3] = a[0][0] * b[3][0] + a[1][0] * b[3][1] + a[2][0] * b[3][2] + a[3][0] * b[3][3];
	// second row
	out[1][0] = a[0][1] * b[0][0] + a[1][1] * b[0][1] + a[2][1] * b[0][2] + a[3][1] * b[0][3];
	out[1][1] = a[0][1] * b[1][0] + a[1][1] * b[1][1] + a[2][1] * b[1][2] + a[3][1] * b[1][3];
	out[1][2] = a[0][1] * b[2][0] + a[1][1] * b[2][1] + a[2][1] * b[2][2] + a[3][1] * b[2][3];
	out[1][3] = a[0][1] * b[3][0] + a[1][1] * b[3][1] + a[2][1] * b[3][2] + a[3][1] * b[3][3];
	// third row
	out[2][0] = a[0][2] * b[0][0] + a[1][2] * b[0][1] + a[2][2] * b[0][2] + a[3][2] * b[0][3];
	out[2][1] = a[0][2] * b[1][0] + a[1][2] * b[1][1] + a[2][2] * b[1][2] + a[3][2] * b[1][3];
	out[2][2] = a[0][2] * b[2][0] + a[1][2] * b[2][1] + a[2][2] * b[2][2] + a[3][2] * b[2][3];
	out[2][3] = a[0][2] * b[3][0] + a[1][2] * b[3][1] + a[2][2] * b[3][2] + a[3][2] * b[3][3];
	// fourth row
	out[3][0] = a[0][3] * b[0][0] + a[1][3] * b[0][1] + a[2][3] * b[0][2] + a[3][0] * b[0][3];
	out[3][1] = a[0][3] * b[1][0] + a[1][3] * b[1][1] + a[2][3] * b[1][2] + a[3][0] * b[1][3];
	out[3][2] = a[0][3] * b[2][0] + a[1][3] * b[2][1] + a[2][3] * b[2][2] + a[3][0] * b[2][3];
	out[3][3] = a[0][3] * b[3][0] + a[1][3] * b[3][1] + a[2][3] * b[3][2] + a[3][0] * b[3][3];
}

// checks the points after they go through a projection matrix to see if they are visible to the user
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

// draw triangle function
int draw_triangle(struct vec2_t p1, struct vec2_t p2, struct vec2_t p3, SDL_Renderer *rnd)
{
	SDL_RenderDrawLine(rnd, p1.x, p1.y, p2.x, p2.y);
	SDL_RenderDrawLine(rnd, p2.x, p2.y, p3.x, p3.y);
	SDL_RenderDrawLine(rnd, p3.x, p3.y, p1.x, p1.y);
}
// draw filled triangle function
int draw_fill_triangle(struct vec2_t p1, struct vec2_t p2, struct vec2_t p3, SDL_Renderer *rnd, SDL_Color scolor, int dbg)
{
	// Define our variables
	SDL_Vertex vertices[3] =
		{
			{{p1.x, p1.y}, {scolor.r, scolor.g, scolor.b, scolor.a}},
			{{p2.x, p2.y}, {scolor.r, scolor.g, scolor.b, scolor.a}},
			{{p3.x, p3.y}, {scolor.r, scolor.g, scolor.b, scolor.a}}};
	// draw the finished result
	// TODO: add make a different rasterization method
	if (dbg == 0)
	{
		if (SDL_RenderGeometry(rnd, NULL, vertices, 3, NULL, 0) != 0)
		{
			printf("Error occured with drawing face");
			return -1;
		}
	}
	// if debug is on then draw debug triangles
	if (dbg == 1 || dbg == 2)
	{
		SDL_SetRenderDrawColor(rnd, 160, 32, 240, 1);
		draw_triangle(p1, p2, p3, rnd);
	}
	return 1;
}

struct vec4_t inverse_vector(struct vec4_t a)
{
	a.x = -a.x;
	a.y = -a.y;
	a.z = -a.z;
	return a;
}

// handle the triangles
int handle_triangles(unsigned int faces, SDL_Renderer *rnd, struct triangle tri[faces], float projmat[4][4])
{
	// handle the camera movement
	float viewmatrix[4][4];
	struct vec4_t target = {0, 0, 1, 1}, up = {0, 1, 0, 1};
	float yrot4[4][4] = {{{0.0f}}};
	float xrot4[4][4] = {{{0.0f}}};
	mat4_yrot_matrix(yaw, yrot4);
	mat4_xrot_matrix(pitch, xrot4);
	// handle yaw and pitch
	lookdir = mat4_mul_vec4(yrot4, target);
	lookdir = mat4_mul_vec4(xrot4, lookdir);
	// fix for camera tilt
	upv = mat4_mul_vec4(xrot4, up);
	target = vecadd(lookdir, camera);
	camerahandle(camera, target, upv, viewmatrix);
	struct vec4_t p3d[3];
	struct vec4_t p3do[3];
	struct vec2_t p2d[3];
	// handle triangles
	for (unsigned int face = 0; face < faces; face++)
	{
		// instead of rewriting the entire program to run with the new rendering method, i'll just convert the triangles points into individual 4D vectors to use with the older functions.
		p3d[0] = vecadd(tri[face].p1, camera);
		p3d[0] = mat4_mul_vec4(viewmatrix, p3d[0]);
		p3d[1] = vecadd(tri[face].p2, camera);
		p3d[1] = mat4_mul_vec4(viewmatrix, p3d[1]);
		p3d[2] = vecadd(tri[face].p3, camera);
		p3d[2] = mat4_mul_vec4(viewmatrix, p3d[2]);
		// original points for calculating the normals and doing lighting calculations
		p3do[0] = tri[face].p1;
		p3do[1] = tri[face].p2;
		p3do[2] = tri[face].p3;

		// Get normal data
		struct vec4_t normal = calc_normal(p3do);
		// check if user can see triangle if true then render triangle
		if (checkpoints(p3d, projmat) == true && dp(normal, vecadd(target, p3do[0])) <= 0.0f)
		{
			// basic shading
			// calculate the luminosity of triangle
			struct vec4_t ld = {0.0f, 5.0f, 0.0f};
			ld = normalize(ld);
			float lum = normal.x * ld.x + normal.y * ld.y + normal.z * ld.z;
			// printf("Luminosity: %f\n", lum);
			int cl[3] = {255, 255, 255};
			calc_color(lum, cl);
			for (unsigned int i = 0; i < 3; i++)
			{
				// Convert that to 2D space with the projection matrix
				p3d[i] = mat4_mul_vec4(projmat, p3d[i]);
				// Scale to monitor so end user can see the rendered object
				p2d[i].x = p3d[i].x + 1.0f;
				p2d[i].y = p3d[i].y + 1.0f;
				p2d[i].x *= 0.5f * SCREEN_WIDTH;
				p2d[i].y *= 0.5f * SCREEN_HEIGHT;
			}
			SDL_Color color = {cl[0], cl[1], cl[2], 0};
			draw_fill_triangle(p2d[0], p2d[1], p2d[2], rnd, color, 0);
		}
	}
}

// returns the integer value of how many triangles are in a map file
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
		// finding the object path
		char *string = line;
		char *token;
		char objectpath[512];
		// get the first token of the string expected to be "o <objectpath>"
		token = strtok(string, "|");
		// format the object path string to remove the "o " from the string
		sprintf(objectpath, "%s", token + 2);
		// read how many faces are in that file
		n += nlines_begin_with(objectpath, "f");
	}
	printf("Faces %d\n", n);
	return n;
}

// function to read object file and extract the triangles
// computation needed to calculate how many faces there are inside the object file, suggested function: nlines_begin_with()
void tri_from_objfile(unsigned int faces, char *path, struct triangle tri[faces])
{
	// value of how many vertices there are
	unsigned int nvec = nlines_begin_with(path, "v");
	// iterators
	int vi = 0, fi = 0, vni = 0;

	// Struct arrays for vertices and faces
	struct vec4_t vec[nvec];
	struct vec3i_t face[faces];
	// line buffer
	char line[500];

	FILE *objfile = fopen(path, "r");
	while (fgets(line, sizeof(line), objfile))
	{
		char *str[2];
		sprintf(str, "%.1s", line);

		// Vertices
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
		// Faces
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
	// populate the triangles with the object data
	for (unsigned int i = 0; i < faces; i++)
	{
		// point one
		tri[i].p1.x = vec[face[i].x].x;
		tri[i].p1.y = vec[face[i].x].y;
		tri[i].p1.z = vec[face[i].x].z;
		// point two
		tri[i].p2.x = vec[face[i].y].x;
		tri[i].p2.y = vec[face[i].y].y;
		tri[i].p2.z = vec[face[i].y].z;
		// point three
		tri[i].p3.x = vec[face[i].z].x;
		tri[i].p3.y = vec[face[i].z].y;
		tri[i].p3.z = vec[face[i].z].z;
	}
}

// function to read a map file and extract all triangle data from each object
// computation needed for knowing how many faces are in the map, suggested function: mapfaces()
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
			// getting line information
			char *token;
			char *linecpy = line;	   // copy of line
			int facet;				   // how many faces the object has
			float x = 0, y = 0, z = 0; // triangles global offset
			// get the first token of the string expected to be "o <objectpath>"
			token = strtok(linecpy, "|");
			// format the object path string to remove the "o " from the string
			sprintf(objectpath, "%s", token + 2);
			// get the faces
			facet = nlines_begin_with(objectpath, "f");
			struct triangle tribuf[facet];
			// get the second token from the token expected to be "g <x> <y> <z>"
			token = strtok(NULL, "|");
			char *goffbuf;
			goffbuf = strtok(token, " ");
			// get the second token to remove the "g "
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
			// get the objects triangles triangles
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

// move towards the look direction.
struct vec4_t MoveTowards(struct vec4_t target)
{
	struct vec4_t direction = {0.0f, 0.0f, 0.0f, 1};
	// calculate the direction
	direction = vecsub(camera, target);
	direction = normalize(direction);
	// Speed modifier
	direction = vecmulfloat(direction, s);

	// move object towards the direction
	camera.x -= direction.x;
	camera.z += direction.z;
}
// move away from the look direction.
struct vec4_t MoveAway(struct vec4_t target)
{
	struct vec4_t direction = {0.0f, 0.0f, 0.0f, 1};
	// calculate the direction
	direction = vecsub(camera, target);
	// Speed modifier
	direction = vecmulfloat(direction, s);
	// move object away from the direction
	camera.x += direction.x;
	camera.z -= direction.z;
}
// Move to the left or right based on where the user is looking.
struct vec4_t moveside(struct vec4_t target, int d)
{
	struct vec4_t direction = {0.0f, 0.0f, 0.0f, 1};
	direction = vecsub(camera, target);
	direction = normalize(direction);

	struct vec4_t xax = normalize(cross(upv, direction));
	// Speed modifier
	direction = vecmulfloat(xax, s);
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
	bool forward, back, left, right, up, down;
	bool lleft, lright, lup, ldown;
	camera.x = 0.0f;
	camera.y = 0.0f;
	camera.z = 0.0f;
	unsigned int faces = mapfaces("./map_files/map1.map");
	struct triangle triangles[faces];
	readmap(faces, "./map_files/map1.map", triangles);
	// Initialize all of SDLs Modules and create the Renderer
	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_Window *wnd;
	SDL_Event evn;
	SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, &wnd, &rndr);
	// Capture the mouse
	int x, y;
	SDL_GetKeyboardFocus();
	// create black bg
	SDL_SetRenderDrawColor(rndr, 0, 0, 0, 0);
	SDL_RenderClear(rndr);
	// define projection matrix
	float *projmat[4][4] = {{{0}}};
	mat4_project_matrix(fov, aspr, znear, zfar, projmat);
	// hide the cursor
	SDL_ShowCursor(SDL_DISABLE);
	SDL_WarpMouseGlobal(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
	// TODO add delta-time
	//  Frame and input handler
	double start_time = time(NULL);
	for (;;)
	{
		mat4_project_matrix(fov, aspr, znear, zfar, projmat);
		if (SDL_PollEvent(&evn))
		{
			if (evn.type == SDL_QUIT)
				break;
			// mouse handling
			if (evn.type == SDL_MOUSEMOTION)
			{
				SDL_GetMouseState(&x, &y);
				if (x > SCREEN_WIDTH / 2)
					yaw -= 0.1f;
				if (x < SCREEN_WIDTH / 2)
					yaw += 0.1f;
				if (y > SCREEN_HEIGHT / 2)
					pitch -= 0.1f;
				if (y < SCREEN_HEIGHT / 2)
					pitch += 0.1f;

				SDL_WarpMouseGlobal(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
			}
			// TODO: add better keyboard handling
			// Keyboard handling
			if (evn.type == SDL_KEYDOWN)
			{
				// get the key being pressed
				SDL_Keycode KEY = evn.key.keysym.sym;
				// fix to handle multiple being pressed
				if (KEY == SDLK_w)
				{
					forward = true;
				}
				// Backwards
				if (KEY == SDLK_s)
				{
					back = true;
				}
				// Left
				if (KEY == SDLK_d)
				{
					left = true;
				}
				// Right
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
				// Left
				if (KEY == SDLK_LEFT)
				{
					lleft = true;
				}
				// Right
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

				// move forward
				if (forward)
				{
					MoveTowards(vecadd(lookdir, camera));
				}
				// Backwards
				if (back)
				{
					MoveAway(vecadd(lookdir, camera));
				}

				// Left
				if (left)
				{
					moveside(vecadd(lookdir, camera), 0);
				}
				// Right
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
				// Left
				if (lleft)
				{
					yaw += 2.0f;
				}
				// Right
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
				// get the key being pressed
				SDL_Keycode KEY = evn.key.keysym.sym;
				if (KEY == SDLK_w)
				{
					forward = false;
				}
				// Backwards
				if (KEY == SDLK_s)
				{
					back = false;
				}
				// Left
				if (KEY == SDLK_d)
				{
					left = false;
				}
				// Right
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
				// Left
				if (KEY == SDLK_LEFT)
				{
					lleft = false;
				}
				// Right
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
		// clear frame
		SDL_SetRenderDrawColor(rndr, 0, 0, 0, 1);
		SDL_RenderClear(rndr);
		// draw new frame
		// draw_object_mat(rows, cols, matrix, rndr, projmat, 1.0f, 1.0f, 3.0f);
		handle_triangles(faces, rndr, triangles, projmat);
		SDL_RenderPresent(rndr);
		framecount++;
		double curtime = time(NULL);
		if (curtime - start_time >= 1.000000)
		{
			printf("%d fps \n", framecount);
			start_time = curtime;
			framecount = 0;
		}
	}
	SDL_DestroyRenderer(rndr);
	SDL_DestroyWindow(wnd);
	SDL_Quit();
	return 0;
}
