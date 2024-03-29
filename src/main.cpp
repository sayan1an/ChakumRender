#include <iostream>
#include <cstdio>
#include "domainSampler.h"
#include "mathPrimitives.h"
#include "materialTypes.h"
#include "geometryPrimitives.h"
#include "random.h"
#include "objects.h"
#include "shader.h"
#include "bvhAccel.h"
#include "lightSources.h"
#include "mis.h"
#include "ppm.h"
#include "prepareScene.h"
#include <stdint.h>
#include <omp.h>
#include <cmath>
#include <ctime>

#define ANTIALIASING 1
#define MOTIONBLUR   1

double x_alias[] = {0.34, 0.86, 0.20, 0.86, 0.66, 0.34, 0.20, 0.66};
double y_alias[] = {0.20, 0.66, 0.66, 0.34, 0.86, 0.86, 0.34, 0.20};

// Clamp pixel values between 0 to 255. 1/2.2 is gamma correction factor!
inline int toDisplayValue(double x){ return int( pow( clamp(x, 0.0, 1.0), 1.0/2.2 ) * 255 + .5); }

int main(int argc, char *argv[]) {
  if (argc != 2)
    return 0;
  
  if (!initSceneParser(argv[1]))
    return 0;
  
  uint32_t w, h;
  Vec camPos, camDir, cx, cy;
  if (!prepareScreenCamera(w, h, camPos, camDir, cx, cy, true))
    return 0;
  
  if (!prepareObjects(true))
    return 0;
  
  if (!prepareLightSources(true))
    return 0;
  
  loadAccels();
  loadLightSampler();
  
  //Ray camera( Vec(50, 50, 275.0), Vec(0, -0.05, -1).norm());
  //Vec cx = Vec( w * 0.57735 / h, 0., 0.1); // hint : tan( 30 / 180.0 * M_PI ) == 0.57735
  //Ray camera( Vec(50, 40, 275.0 * 2.0), Vec(0, 0, -1).norm());
  //Vec cx = Vec( w * 0.57735 / h, 0., 0.0); // hint : tan( 30 / 180.0 * M_PI ) == 0.57735
  //Veach
  // camera location and direction of looking. Imagine right direction is x, up is y, and z is out of screen. Camera is mostly looking towards -z direction!!
  //Ray camera( Vec(50 * 0, 50 * 2, 275.0 * 2), Vec(0, -0.05 * 3, -1).norm());
  //Define pixel pitch along width of the screen. Field of view is 30 + 30 or 60 degrees.
  //Vec cx = Vec(1.0, 0.0, 0).norm() * ((double)w/h) * 0.57735; // hint : tan( 30 / 180.0 * M_PI ) == 0.57735
  //Define pixel pitch along height of the screen.
  //Vec cy = (cx % camera.d).norm() * 0.57735;
  // 2D Array of pixels
  Vec pixelValue, *pixelColors = new Vec[w * h];

  int motionBlur = 0;
    do {
    #pragma omp parallel for private(pixelValue)
    for(uint32_t y = 0; y < h; y++) {
      // Percentage completion!!
      fprintf(stderr,"\r%5.2f%%",100.*y/(h-1));
      for(uint32_t x = 0; x < w; x++ ) {
	// Start from top left to bottm right of the screen.
	uint32_t idx = (h - y - 1) * w + x;
	pixelValue = Vec();
	for (uint32_t k = 0; k < ANTIALIASING; k++) {
	  int depth = 2;
	  // Shoot ray from camera thur each pixel: Computed as: camera direction +/- deviation from camera direction in terms of pixel pitch.
	  Vec cameraRayDir = cx * ( (double(x) + ((k<8)? x_alias[k] : 0))/w - .5) + cy * ((double(y) + ((k < 8)? y_alias[k] : 0))/h - .5) + camDir;
	  // Find color of intersection. In case no intersection is found color the pixel black.
	  pixelValue = pixelValue + shade(Ray(camPos, cameraRayDir.norm()), depth);
	}
	pixelValue = pixelValue / ANTIALIASING;
	// Clamp the rgb values.
	pixelColors[idx] = pixelColors[idx] + Vec(pixelValue.x, pixelValue.y, pixelValue.z);
      }
    }

    for (uint32_t loop = 0; loop < nTriangles; loop++)
      triangleList[loop].updatePos(Vec(5.0, 0, 5.0));
    delete bvhAccelT;
    bvhAccelT = new BvhAccel((uint8_t *)triangleList, nTriangles, (std::size_t)sizeof(Triangle));
    bvhAccelT -> initAccel();

    motionBlur++;
  } while (motionBlur < MOTIONBLUR);

  fprintf(stderr,"\n");

  for(uint32_t y = 0; y < h; y++) {
    for(uint32_t x = 0; x < w; x++ ) {
      uint32_t idx = (h - y - 1) * w + x;
      pixelColors[idx] = pixelColors[idx] / (double) MOTIONBLUR;
      pixelColors[idx].x = clamp(pixelColors[idx].x, 0.0, 1.0);
      pixelColors[idx].y = clamp(pixelColors[idx].y, 0.0, 1.0);
      pixelColors[idx].z = clamp(pixelColors[idx].z, 0.0, 1.0);
    }
  }

  // Save pixelColors in ppm format.
  // hint: Google the PPM image format
  FILE *f = fopen("image.ppm", "w");
  // Format header.
  fprintf(f, "P3\n%d %d\n%d\n", w, h, 255);
  for (uint32_t p = 0; p < w * h; p++) {
    //Save each pixel as rgb triplet.
    fprintf(f,"%d %d %d ", toDisplayValue(pixelColors[p].x), toDisplayValue(pixelColors[p].y), toDisplayValue(pixelColors[p].z));
  }
  fclose(f);

  delete []pixelColors;

  return 0;
}
/*
int main() {
  uint32_t w, h;
  Vec *image;
  readImage(image, w, h);
  EnvSource s1(image, w, h, Vec(10,10,10), 1);
  Vec *lightMap = new Vec[360 * 180];

  for (uint32_t theta = 0; theta < 180; theta++)
    for (uint32_t phi = 0; phi < 360; phi++) {
      double x = sin(theta * PI / 180.0) * cos(phi * PI / 180.0);
      double y = cos(theta * PI / 180.0);
      double z = sin(theta * PI / 180.0) * sin(phi * PI / 180.0);
      std::cout<<theta * PI /180.0<<" "<<phi * PI / 180.0<<" ";
      lightMap[theta * 360 + phi] = s1.getRadiance(Vec(x, y, z)) / 10;
    }

  writeImage(lightMap, 360, 180);
}*/