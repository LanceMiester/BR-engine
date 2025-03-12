#include <stdlib.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <zlib.h>
#include <png.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 360

typedef struct
{
	unsigned int h;
	unsigned int w;
	unsigned char **imagedata;
} tex;
typedef struct
{
	float x;
	float y;
	float z;
	float w;
} vec4_t;
typedef struct
{
	float x;
	float y;
	float z;
} vec3_t;
typedef struct
{
	int x;
	int y;
	int z;
} vec3i_t;
typedef struct
{
	float x;
	float y;
} vec2_t;
typedef struct
{
	vec4_t p1;
	vec4_t p2;
	vec4_t p3;
	int texiter;
	vec2_t tp[3];
} triangle_t;
typedef struct
{
	triangle_t *faces;
	int ifaces;
	int *flags;
} object_t;

float speed = 0.5f;
float znear = 0.1f;
float zfar = 1000.0f;
float fov = 90.0f;
float aspr = (float)SCREEN_HEIGHT / (float)SCREEN_WIDTH;
float pitch = 0.0f; // in degrees
float yaw = 0.0f;	// in degrees
bool force_exit = false;
vec4_t camera;
vec4_t lookdir = {0, 0, 1, 1}, upv = {0, 1, 0, 1};
int framecount = 0;
int objn = 0;
int texn = 0;
// TODO: make this function faster.
int SetPixel(SDL_Surface *surface, int x, int y, uint8_t r, uint8_t g, uint8_t b)
{
	uint8_t *pixels = (uint8_t *)surface->pixels;
	pixels[y * surface->pitch + x * surface->format->BytesPerPixel] = r;
	pixels[y * surface->pitch + x * surface->format->BytesPerPixel + 1] = g;
	pixels[y * surface->pitch + x * surface->format->BytesPerPixel + 2] = b;
	return 0;
}

float calcedge(vec4_t p, vec4_t p2, vec4_t p3)
{
	float edges = (p2.x - p.x) * (p3.y - p.y) - (p2.y - p.y) * (p3.x - p.x);
	return edges;
}

int calc_color(float lum, uint8_t c[3])
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

// should I instead pass in the triangle structure? probably.
int Rasterize_Triangle(SDL_Surface *frame, vec4_t p[3], tex *texture, vec2_t texp[3], float depthbuf[SCREEN_HEIGHT][SCREEN_WIDTH], int lum)
{
	float max_X = fmaxf(p[0].x, fmaxf(p[1].x, p[2].x));
	float max_Y = fmaxf(p[0].y, fmaxf(p[1].y, p[2].y));
	float min_X = fminf(p[0].x, fminf(p[1].x, p[2].x));
	float min_Y = fminf(p[0].y, fminf(p[1].y, p[2].y));
	float maxt_X = fmaxf(texp[0].x, fmaxf(texp[1].x, texp[2].x));
	float maxt_Y = fmaxf(texp[0].y, fmaxf(texp[1].y, texp[2].y));
	vec4_t p1;
	int color[3];
	int tx = 0, ty = 0;
	for (int y = min_Y; y < max_Y; y++)
	{
		for (int x = min_X; x < max_X; x++)
		{
			if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT)
			{
				p1.y = y;
				p1.x = x;
				float ABC = calcedge(p[0], p[1], p[2]);
				float ABP = calcedge(p[0], p[1], p1);
				float BCP = calcedge(p[1], p[2], p1);
				float CAP = calcedge(p[2], p[0], p1);
				if (ABP >= 0 && BCP >= 0 && CAP >= 0)
				{

					float scalex = (maxt_X - 1) / (max_X - min_X);
					float scaley = (maxt_Y - 1) / (max_Y - min_Y);
					// the sum of these should always be 1.
					float weight1 = BCP / ABC, weight2 = CAP / ABC, weight3 = ABP / ABC;
					float depth = weight1 * p[0].z + weight2 * p[1].z + weight3 * p[2].z;
					if (depthbuf[y][x] > depth)
					{

						unsigned int proj_x = round(weight1 * texp[0].x + weight2 * texp[1].x + weight3 * texp[2].x);
						unsigned int proj_y = round(weight1 * texp[0].y + weight2 * texp[1].y + weight3 * texp[2].y);
						if (proj_x > texture->w - 1)
							proj_x = texture->w - 1;
						if (proj_y > texture->h - 1)
							proj_y = texture->h - 1;
						unsigned char *row = texture->imagedata[proj_y];
						unsigned char *pixel = &row[proj_x * 4];
						SetPixel(frame, x, y, pixel[2], pixel[1], pixel[0]);
						depthbuf[y][x] = depth;
					}
					else if (depthbuf[y][x] < 0.02f)
					{
						unsigned int proj_x = round(weight1 * texp[0].x + weight2 * texp[1].x + weight3 * texp[2].x);
						unsigned int proj_y = round(weight1 * texp[0].y + weight2 * texp[1].y + weight3 * texp[2].y);
						if (proj_x > texture->w - 1)
							proj_x = texture->w - 1;
						if (proj_y > texture->h - 1)
							proj_y = texture->h - 1;
						unsigned char *row = texture->imagedata[proj_y];
						unsigned char *pixel = &row[proj_x * 4];
						color[0] = pixel[2];
						color[1] = pixel[1];
						color[2] = pixel[0];
						SetPixel(frame, x, y, pixel[2], pixel[1], pixel[0]);
						depthbuf[y][x] = depth;
					}
				}
			}
		}
	}
	return 0;
}

void printvec(vec4_t v, char *name)
{
	printf("%s : X %f | Y %f | Z %f | W %f\n", name, v.x, v.y, v.z, v.w);
}

// function to apply an offset to a triangle_t
triangle_t applyoff(triangle_t tri1, float x, float y, float z)
{
	triangle_t tri;
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

vec4_t mat4_mul_vec4(float mat4[4][4], vec4_t vec)
{
	vec4_t result = {0};

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

float dot(vec4_t a, vec4_t b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}
vec4_t vecadd(vec4_t a, vec4_t b)
{
	vec4_t result;
	result.x = a.x + b.x;
	result.y = a.y + b.y;
	result.z = a.z + b.z;
	result.w = 1;
	return result;
}
vec4_t vecmul(vec4_t a, vec4_t b)
{
	vec4_t result;
	result.x = a.x * b.x;
	result.y = a.y * b.y;
	result.z = a.z * b.z;
	result.w = 1;
	return result;
}
vec4_t vecsub(vec4_t a, vec4_t b)
{
	vec4_t result;
	result.x = a.x - b.x;
	result.y = a.y - b.y;
	result.z = a.z - b.z;
	result.w = 1;
	return result;
}
vec4_t normalize(vec4_t a)
{
	vec4_t normalized;
	float l = sqrtf(a.x * a.x + a.y * a.y + a.z * a.z);
	normalized.x = a.x / l;
	normalized.y = a.y / l;
	normalized.z = a.z / l;
	return normalized;
}
vec4_t cross(vec4_t a, vec4_t b)
{
	vec4_t cp;
	cp.x = a.y * b.z - a.z * b.y;
	cp.y = a.z * b.x - a.x * b.z;
	cp.z = a.x * b.y - a.y * b.x;
	return cp;
}

vec4_t calc_normal(vec4_t points3d[3])
{
	vec4_t normal, line[2];
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

vec4_t vecmulfloat(vec4_t v, float f)
{
	vec4_t a = {v.x * f, v.y * f, v.z * f, 1.0f};
	return a;
}

// uses lookat matrix
void camerahandle(vec4_t cam, vec4_t target, vec4_t up, float mat4[4][4])
{

	up = normalize(up);

	vec4_t zax = normalize(vecsub(cam, target));

	vec4_t xax = normalize(cross(up, zax));

	vec4_t yax = cross(zax, xax);

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

	mat4[3][0] = -dot(xax, cam);
	mat4[3][1] = -dot(yax, cam);
	mat4[3][2] = -dot(zax, cam);
	mat4[3][3] = 1.0f;
}

tex *Load_png(char *path)
{
	int code;
	SDL_Surface *surface;
	int height, width, interlace, bit_depth, color_type;
	png_bytepp row_pointers = NULL;
	FILE *image = fopen(path, "rb");
	if (!image)
	{
		code = 1;
		goto fail;
	}
	png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png)
	{
		code = 2;
		goto fail;
	}
	png_infop info = png_create_info_struct(png);
	if (!info)
	{
		code = 3;
		goto fail;
	}
	png_init_io(png, image);
	png_read_info(png, info);
	png_get_IHDR(png, info, &width, &height, &bit_depth, &color_type, NULL, NULL, NULL);
	row_pointers = (png_bytepp)malloc(sizeof(png_bytep) * height);
	for (int i = 0; i < height; i++)
	{
		row_pointers[i] = (png_bytep)malloc(png_get_rowbytes(png, info));
	}
	tex *texture = (tex *)malloc(sizeof(tex));
	png_read_image(png, row_pointers);
	texture->h = height;
	texture->w = width;
	texture->imagedata = row_pointers;
	fclose(image);
	png_destroy_read_struct(&png, &info, NULL);

	for (int y = 0; y < height; y++)
	{
		png_bytep row = row_pointers[y];
		for (int x = 0; x < width; x++)
		{
			unsigned char *pixel = &row[x * 4];
			printf(" %d %d %d ", pixel[0], pixel[1], pixel[2]);
		}
		printf("\n");
	}
	return texture;
fail:
	fprintf(stderr, "Failure at loading texture code: %d", code);
	exit(-1);
}

bool checkpoints(vec4_t n[3], float projmat[4][4])
{
	for (int i = 0; i < 3; i++)
	{
		vec4_t b = mat4_mul_vec4(projmat, n[i]);
		if (b.x >= -1.0f && b.x <= 1.0f)
		{
			if (b.y >= -1.0f && b.y <= 1.0f)
			{
				if (b.z >= 0.1f)
					return true;
			}
		}
	}
	return false;
}
// Jesus christ

int handle_triangle(SDL_Surface *frame, object_t **objects, float depthbuffer[SCREEN_HEIGHT][SCREEN_WIDTH], float projmat[4][4], tex **textures)
{

	float viewmatrix[4][4];
	vec4_t target = {0, 0, 1, 1}, up = {0, 1, 0, 1};
	float yrot4[4][4] = {{{0.0f}}};
	float xrot4[4][4] = {{{0.0f}}};
	mat4_yrot_matrix(yaw, yrot4);
	mat4_xrot_matrix(pitch, xrot4);
	double ctime = time(NULL);
	lookdir = mat4_mul_vec4(yrot4, target);
	lookdir = mat4_mul_vec4(xrot4, lookdir);
	upv = mat4_mul_vec4(xrot4, up);
	target = vecadd(lookdir, camera);
	camerahandle(camera, target, upv, viewmatrix);
	vec4_t p3d[3];
	vec4_t p3do[3];
	for (unsigned int obj = 0; obj < objn; obj++)
	{
		for (int face = 0; face < objects[obj]->ifaces; face++)
		{
			// instead of rewriting the entire program to run with the new rendering method,
			// i'll just convert the triangle_ts points into individual 4D vectors to use with the older functions.
			p3d[0] = vecadd(objects[obj]->faces[face].p1, camera);

			p3d[0] = mat4_mul_vec4(viewmatrix, p3d[0]);
			p3d[1] = vecadd(objects[obj]->faces[face].p2, camera);
			p3d[1] = mat4_mul_vec4(viewmatrix, p3d[1]);
			p3d[2] = vecadd(objects[obj]->faces[face].p3, camera);
			p3d[2] = mat4_mul_vec4(viewmatrix, p3d[2]);

			p3do[0] = objects[obj]->faces[face].p1;
			p3do[1] = objects[obj]->faces[face].p2;
			p3do[2] = objects[obj]->faces[face].p3;

			vec4_t normal = calc_normal(p3do);

			if (checkpoints(p3d, projmat) == true && dot(normal, vecadd(camera, p3do[0])) < 0.0f)
			{
				double ptime = time(NULL);
				vec4_t ld = {0.0f, 5.0f, 0.0f};
				ld = normalize(ld);
				float lum = normal.x * ld.x + normal.y * ld.y + normal.z * ld.z;

				uint8_t cl[3] = {255, 255, 255};
				calc_color(lum, cl);
				for (unsigned int i = 0; i < 3; i++)
				{
					p3d[i] = mat4_mul_vec4(projmat, p3d[i]);
					p3d[i].x = p3d[i].x + 1.0f;
					p3d[i].y = p3d[i].y + 1.0f;
					p3d[i].x *= 0.5f * SCREEN_WIDTH;
					p3d[i].y *= 0.5f * SCREEN_HEIGHT;
				}
				Rasterize_Triangle(frame, p3d, textures[objects[obj]->faces[face].texiter], objects[obj]->faces[face].tp, depthbuffer, lum);
			}
		}
	}
	return;
}

object_t *obj_from_objfil(char *path)
{
	int vi = 0, fi = 0;

	vec4_t *vec = (vec4_t *)malloc(8 * sizeof(vec4_t));
	vec3i_t *face = (vec3i_t *)malloc(12 * sizeof(vec3i_t));
	char line[500];
	int recs = 0;
	FILE *objfile = fopen(path, "r");
	if (!objfile)
	{
		fprintf(stderr, "Error occured with reading object file \"%s\"\n This Error happens usually when the file could not be found", path);
		force_exit = true;
		return;
	}
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
					if (vi > 8 + recs)
					{
						printf("vec reallocate\nVec iter %d\nVec size %d\n", vi, vi * sizeof(vec4_t));
						vec = (vec4_t *)realloc(vec, vi++ * sizeof(vec4_t));
						recs++;
					}
					vvi++;
				}
				token = strtok(NULL, delim);
			}
		}
		else if (strcmp(str, "f") == 0)
		{
			recs = 0;
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
					if (fi > 12 + recs)
					{
						printf("face reallocate\n");
						face = (vec3i_t *)realloc(face, fi * sizeof(vec3i_t));
						recs++;
					}
					ffi++;
				}
				token = strtok(NULL, delim);
			}
		}
	}
	object_t *obj = malloc(sizeof(object_t));
	obj->faces = (triangle_t *)malloc(fi * sizeof(triangle_t));

	for (unsigned int i = 0; i < fi; i++)
	{
		obj->faces[i].p1.x = vec[face[i].x].x;
		obj->faces[i].p1.y = vec[face[i].x].y;
		obj->faces[i].p1.z = vec[face[i].x].z;
		obj->faces[i].p2.x = vec[face[i].y].x;
		obj->faces[i].p2.y = vec[face[i].y].y;
		obj->faces[i].p2.z = vec[face[i].y].z;
		obj->faces[i].p3.x = vec[face[i].z].x;
		obj->faces[i].p3.y = vec[face[i].z].y;
		obj->faces[i].p3.z = vec[face[i].z].z;
	}
	obj->ifaces = fi;
	free(vec);
	free(face);
	printf("obj pointer %p\n", obj);
	return obj;
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

		if (line[0] == v[0])
		{
			n++;
		}
	}
	printf("%d\n", n);
	return n;
}

void moveobj(object_t *obj, vec4_t offset)
{
	for (int face = 0; face < obj->ifaces; face++)
	{
		obj->faces[face].p1 = vecadd(obj->faces[face].p1, offset);
		obj->faces[face].p2 = vecadd(obj->faces[face].p2, offset);
		obj->faces[face].p3 = vecadd(obj->faces[face].p3, offset);
	}
	return;
}
vec4_t inverse(vec4_t v)
{
	vec4_t inversed;
	inversed.x = -v.x;
	inversed.y = -v.y;
	inversed.z = -v.z;
	return inversed;
}

tex *load_tex_file(char *path, object_t *object)
{
	FILE *f = fopen(path, "r");
	if (!f)
	{
		printf("failed %s\n", path);
		exit(-1);
	}
	char line[500];
	fgets(line, sizeof(line), f);
	char pathf[500];
	sscanf(line, "%s", pathf);
	int face_iter = 0;
	while (fgets(line, sizeof(line), f))
	{
		char *linecpy = line;
		unsigned int p1 = 1, p2 = 1, p3 = 1, p4 = 1, p5 = 1, p6 = 1;
		sscanf(linecpy, "f %d %d %d %d %d %d", &p1, &p2, &p3, &p4, &p5, &p6);
		object->faces[face_iter].tp[0].x = p1;
		object->faces[face_iter].tp[0].y = p2;
		object->faces[face_iter].tp[1].x = p3;
		object->faces[face_iter].tp[1].y = p4;
		object->faces[face_iter].tp[2].x = p5;
		object->faces[face_iter].tp[2].y = p6;
		object->faces[face_iter].texiter = texn;
		face_iter++;
	}
	tex *texture = Load_png(pathf);
	return texture;
}

object_t **addobj(char *path, object_t **objs)
{
	objn++;
	objs = (object_t **)realloc(objs, objn * sizeof(object_t));
	objs[objn - 1] = obj_from_objfil(path);
	;
	moveobj(objs[objn - 1], inverse(camera));
	return objs;
}

void readmap(char *path, object_t **objects, tex **Textures)
{

	// this expects the map to be in
	// o <objectfile>|g x y z t <UV_file>  format
	char line[500];
	FILE *file = fopen(path, "r");

	int line_iter = 0;
	while (fgets(line, sizeof(line), file))
	{

		char objectpath[512];
		char *token;
		char *linecpy = line;
		token = strtok(linecpy, "|");
		sprintf(objectpath, "%s", token + 2);
		token = strtok(NULL, "|");
		vec4_t goff;
		char *goffbuf;
		goffbuf = strtok(token, " ");
		goffbuf = strtok(NULL, " ");

		goff.x = atof(goffbuf);
		goffbuf = strtok(NULL, " ");

		goff.y = atof(goffbuf);
		goffbuf = strtok(NULL, " ");

		goff.z = atof(goffbuf);
		goffbuf = strtok(NULL, " ");

		goffbuf = strtok(NULL, " ");
		printf("%s\n", goffbuf);
		objects[line_iter] = obj_from_objfil(objectpath);
		Textures[line_iter] = load_tex_file(goffbuf, objects[line_iter]);
		printf("object out %p tex out %p\nvalue of i %d\n", objects[line_iter], Textures[line_iter], line_iter);
		moveobj(objects[line_iter], goff);
		if (line_iter >= objn)
		{
			break;
		}
		line_iter++;
	}
	return;
}

vec4_t MoveTowards(vec4_t target)
{
	vec4_t direction = {0.0f, 0.0f, 1.0f, 1};

	direction = vecsub(camera, target);
	direction = vecmulfloat(direction, speed);

	camera.x -= direction.x;
	camera.z += direction.z;
}
vec4_t MoveAway(vec4_t target)
{
	vec4_t direction = {0.0f, 0.0f, 1.0f, 1};

	direction = vecsub(camera, target);
	direction = vecmulfloat(direction, speed);
	camera.x += direction.x;
	camera.z -= direction.z;
	return;
}
vec4_t moveside(vec4_t target, int d)
{
	vec4_t direction = {0.0f, 0.0f, 0.0f, 1};
	direction = vecsub(camera, target);
	direction = normalize(direction);

	vec4_t xax = normalize(cross(upv, direction));
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
	return;
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

		objn = nlines_begin_with(argv[1], "o");
		object_t **objects = (object_t **)malloc(objn * sizeof(object_t));
		tex **textures = (SDL_Surface **)malloc(objn * sizeof(tex));
		readmap(argv[1], objects, textures);
		// printf("arg 1 %s\n", argv[1]);
		SDL_Init(SDL_INIT_EVERYTHING);
		SDL_Window *wnd;
		SDL_Event evn;
		wnd = SDL_CreateWindow("BR_Engine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
		SDL_Surface *frame = SDL_GetWindowSurface(wnd);
		int x, y;
		SDL_GetKeyboardFocus();
		float *projmat[4][4] = {{{0}}};
		mat4_project_matrix(fov, aspr, znear, zfar, projmat);
		// TODO add delta-time
		//  Frame and input handler
		printf("Objects %d\n", objn);
		// SDL_BlitSurface(Load_png("./textures/Brick.png"), 0, frame, 0);
		double start_time = time(NULL);
		for (;;)
		{
			float depthbuf[SCREEN_HEIGHT][SCREEN_WIDTH] = {5.0};
			// mat4_project_matrix(fov, aspr, znear, zfar, projmat);
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
					if (KEY == SDLK_c)
						objects = addobj("./test_objects/cube.obj", objects);
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
			handle_triangle(frame, objects, depthbuf, projmat, textures);

			SDL_UnlockSurface(frame);
			SDL_UpdateWindowSurface(wnd);

			framecount++;
			double curtime = time(NULL);
			if (curtime - start_time >= 1.000000)
			{
				printf("%d fps\n", framecount);
				start_time = curtime;
				framecount = 0;
			}
		}
		for(int i = 0; i < objn; i++)
		{
			free(objects[i]);
		}	
		for(int i = 0; i < texn; i++)
		{
			free(textures[i]);
		}
		free(objects);
		free(textures);
		SDL_DestroyWindow(wnd);
		SDL_Quit();

		
	}
	else
	{
		printf("Please include a map file to load\n");
	}
	return 0;
}
