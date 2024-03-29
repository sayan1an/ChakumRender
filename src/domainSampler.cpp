#include "mathPrimitives.h"
#include "random.h"
#include "domainSampler.h"
#include <cmath>
#include <cstdio>
#include <ctime>

#define MULTIPLIER 300

Vec* SphericalSampler::sampleVolume;
Vec* SphericalSampler::sampleSurface;
Vec* SphericalSampler::cosineSurfaceSamples;
double* SphericalSampler::randoms;
int SphericalSampler::__initSamples = SphericalSampler::initSamples();

int SphericalSampler::initSamples() {
    Random random(0x191123FB);
    double x, y, z;
    /* Optimize...Use aligned memory??*/
    sampleVolume = new Vec[nSAMPLES * MULTIPLIER];
    sampleSurface = new Vec[nSAMPLES * MULTIPLIER];
    randoms = new double[nSAMPLES * MULTIPLIER];
    cosineSurfaceSamples = new Vec[nSAMPLES * MULTIPLIER];
    for (int i = 0; i < nSAMPLES * MULTIPLIER;) {
      x = random.randomMTD(-1.0, 1.0);
      y = random.randomMTD(-1.0, 1.0);
      z = random.randomMTD(-1.0, 1.0);

      if ((x*x + y*y + z*z <= 1)) {
	sampleVolume[i] = Vec(x, y, z);
	sampleSurface[i] = Vec(x, y, z).norm();
	i++;
      }
    }
    for (int i = 0; i < nSAMPLES * MULTIPLIER; i++)
      randoms[i] = random.randomMTD(0, 1);

    // Cosine weighted surface samples.
    for (int i = 0; i < nSAMPLES * MULTIPLIER; i++) {
      double e1 = random.randomMTD(0, 1);
      double e2 = random.randomMTD(0, 1);
      double theta = asin(sqrt(e1));
      double phi = 2 * PI * e2;
      double x = sin(theta) * cos(phi);
      double y = sin(theta) * sin(phi);
      double z = cos(theta);

      cosineSurfaceSamples[i] = Vec(x, y, z);
    }

    return 0;
}

double SphericalSampler::getSphericalVolumeSamples(const Vec &x, const int nSamples, Vec *store) {
    Random random(clock() & 0xFFFFFFFF);
    int offset = random.randomMTD(0, (MULTIPLIER - 1) * nSAMPLES);
    // 0xFFFFFFF0 gives better alignment and performance!!
    for (int i = offset & 0xFFFFFFF0, j = 0; j < nSamples; i++, j++)
      store[j] = sampleVolume[i] + x;

    return 4.0 * PI /3.0;
}

double SphericalSampler::getSphericalSurfaceSamples(const Vec &x, const int nSamples, Vec *store) {
    Random random(clock() & 0xFFFFFFFF);
    int offset = random.randomMTD(0, (MULTIPLIER - 1) * nSAMPLES);
    // 0xFFFFFFF0 gives better alignment and performance!!
    for (int i = offset & 0xFFFFFFF0, j = 0; j < nSamples; i++, j++)
      store[j] = sampleSurface[i] + x;

    return 4 * PI;
}

// Assumes normalized n
double SphericalSampler::getHemiVolumeSamples(const Vec &n, const Vec &x, const int nSamples, Vec *store) {
    if (isNotClose(n.length(), 1, 1e-10))
      fprintf (stderr, "Direction is not normalized!!");

    Random random(clock() & 0xFFFFFFFF);
    int offset = random.randomMTD(0, (MULTIPLIER - 1) * nSAMPLES);
    // 0xFFFFFFF0 gives better alignment and performance!!
    for (int i = offset & 0xFFFFFFF0, j = 0; j < nSamples; i++, j++)
      store[j] = n.dot(sampleVolume[i]) >= 0? x+sampleVolume[i] : x-sampleSurface[i];

    return 2.0 * PI / 3.0;
}

// Assumes normalized n
double SphericalSampler::getHemiDirectionSamples(const Vec &n, const int nSamples, Vec *store) {
    if (isNotClose(n.length(), 1, 1e-10))
      fprintf (stderr, "Direction is not normalized!!");

    Random random(clock() & 0xFFFFFFFF);
    int offset = random.randomMTD(0, (MULTIPLIER - 1) * nSAMPLES);
    // 0xFFFFFFF0 gives better alignment and performance!!
    for (int i = offset & 0xFFFFFFF0, j = 0; j < nSamples; i++, j++)
      store[j] = n.dot(sampleSurface[i]) >= 0? sampleSurface[i] : sampleSurface[i] * - 1.0;

    return 2.0 * PI;
}

// Assumes normalized n
double SphericalSampler::getHemiDirectionSamplesTrue(const Vec &n, const int nSamples, Vec *store) {
    if (isNotClose(n.length(), 1, 1e-10))
      fprintf (stderr, "Direction is not normalized!!");

    Random random(clock() & 0xFFFFFFFF);

    double sx, sy, sz;
    Vec s;
    for (int i = 0; i < nSAMPLES;) {
      sx = random.randomMTD(-1.0, 1.0);
      sy = random.randomMTD(-1.0, 1.0);
      sz = random.randomMTD(-1.0, 1.0);

      if ((sx*sx + sy*sy + sz*sz <= 1)) {
	s = Vec(sx, sy, sz).norm();
	store[i] = n.dot(s) >= 0? s : s * -1.0;
	i++;
      }
    }

    return 2.0 * PI;
}

// Importance sampling as per visible surface of light from a point x.
double SphericalSampler::getLightDirectionSamples(const Vec &c, const double r, const Vec &x, const int nSamples, Vec *store) {
    Random random(clock() & 0xFFFFFFFF);
    int offset = random.randomMTD(0, (MULTIPLIER - 1) * nSAMPLES);
    Vec dir = (x - c).norm();
    // 0xFFFFFFF0 gives better alignment and performance!!
    for (int i = offset & 0xFFFFFFF0, j = 0; j < nSamples; i++, j++)
      store[j] = dir.dot(sampleSurface[i]) >= 0? (c + sampleSurface[i] * r - x).norm(): (c - sampleSurface[i] * r - x).norm();

    return 2 * PI * r * r;
}

void SphericalSampler::getTriLightSurfaceSamples(const Vec &p1, const Vec &p2, const Vec &p3, const uint32_t nSamples, Vec *store) {
    Random random(clock() & 0xFFFFFFFF);
    uint32_t offset = random.randomMTD(0, (MULTIPLIER - 2) * nSAMPLES); // We'll use two random number on every iteration.

    for (uint32_t i = 0, j = offset & 0xFFFFFFF0; i < nSamples; i++, j += 2) {
      double e1 = sqrt(randoms[j]);
      double e2 = randoms[j + 1];

      store[i] = p1 * (1.0 - e1) +
		 p2 * e1 * (1.0 - e2) +
		 p3 * e2 * e1;
    }
}

// Cosine weighted surface sampling.
// Assumes normalized n.
double SphericalSampler::getCosineDirectionSamples(const Vec &n, const int nSamples, Vec *store) {
    if (isNotClose(n.length(), 1, 1e-10)) // Remove this to caller function
      fprintf (stderr, "Direction is not normalized!!");

    Random random(clock() & 0xFFFFFFFF);
    int offset = random.randomMTD(0, (MULTIPLIER - 1) * nSAMPLES); // We'll use one sample on every iteration.

    //Z - axis is transfomed to normal direction.

    Vec newX; // X - axis transfomed to this axis.
    // Let's find a Vector perpendicular to n.
    if (n.x != 0)
      newX = Vec((-n.y-n.z)/n.x, 1.0, 1.0);
    else if (n.y != 0)
      newX = Vec(1.0,  -n.z/n.y, 1.0); // since n.x is zero, we can simplify 2nd dimension
    else if (n.z != 0)
      newX = Vec(1.0, 1.0, 0); // Since both n.x and n.y are zero.
    else
      fprintf (stderr, "WTF is n??\n");

    newX.norm();

    Vec newY = (n%newX).norm();

    for (int i = offset & 0xFFFFFFF0, j = 0; j < nSamples; i++, j++) {
      double x = cosineSurfaceSamples[i].x;
      double y = cosineSurfaceSamples[i].y;
      double z = cosineSurfaceSamples[i].z;

      store[j] = Vec(x * newX.x + y * newY.x + z * n.x,
		     x * newX.y + y * newY.y + z * n.y,
		     x * newX.z + y * newY.z + z * n.z);
    }

    return PI;
}

// GGX, BECKMANN, PHONG sampling.
// Assumes normalized n, wo
double SphericalSampler::getBrdfDirectionSamples(const Vec &n, const Vec &wo, const double alpha, const int nSamples, Vec *store, const BRDFType b) {
    if (isNotClose(n.length(), 1, 1e-10)) // Remove this to caller function
      fprintf (stderr, "Direction is not normalized!!\n");
    if (isNotClose(wo.length(), 1, 1e-10))
      fprintf (stderr, "Direction is not normalized!!\n");

    Random random(clock() & 0xFFFFFFFF);
    int offset = random.randomMTD(0, (MULTIPLIER - 2) * nSAMPLES); // We'll use two random number on every iteration.

    //Z - axis is transfomed to normal direction.

    Vec newX; // X - axis transfomed to this axis.
    // Let's find a Vector perpendicular to n.
    if (n.x != 0)
      newX = Vec((-n.y-n.z)/n.x, 1.0, 1.0);
    else if (n.y != 0)
      newX = Vec(1.0,  -n.z/n.y, 1.0); // since n.x is zero, we can simplify 2nd dimension
    else if (n.z != 0)
      newX = Vec(1.0, 1.0, 0); // Since both n.x and n.y are zero.
    else
      fprintf (stderr, "WTF is n??\n");

    newX.norm();

    Vec newY = (n%newX).norm();

    for (int i = 0, j = offset & 0xFFFFFFF0; i < nSamples; i++, j += 2) {
      double e1 = randoms[j];
      double e2 = randoms[j+1];
      double costheta = 0;
      double sintheta = 0;
      if (b == GGX) {
	double tantheta = alpha * sqrt(e1) / sqrt(1 - e1 + 1e-10);
	costheta = 1.0 / sqrt(1 + tantheta * tantheta);
	sintheta = tantheta * costheta;
      }
      else if (b == BECKMANN) {
	double tantheta = sqrt(alpha * alpha * log (1 - e1 + 1e-20) * -1.0);
	costheta = 1.0 / sqrt(1 + tantheta * tantheta);
	sintheta = tantheta * costheta;
      }
      else if (b == PHONG) {
	costheta = pow(e1, 1.0 / (alpha + 2.0));
	sintheta = sqrt(1.0 - costheta * costheta);
      }
      else
	fprintf (stderr, "Cannot Identify BRDF Type.\n");

      double phi = 2 * PI * e2;

      double cosphi = cos(phi);
      double sinphi = sin(phi);

      // Compute half vector.
      double xh = sintheta * cosphi;
      double yh = sintheta * sinphi;
      double zh = costheta;

      //Apply transformation, rotate
      Vec h = Vec(xh * newX.x + yh * newY.x + zh * n.x,
		  xh * newX.y + yh * newY.y + zh * n.y,
		  xh * newX.z + yh * newY.z + zh * n.z);

      store[i] = (h * 2.0 * (h.dot(wo)) - wo).norm();
    }

    return 1.0;
}

// Get phong cosine lobe around w, with exponent e.
// Assumes normalized n, w
double SphericalSampler::getClassicPhongDirectionSamples(const Vec &n, const Vec &w, const double e, const int nSamples, Vec *store) {
    if (isNotClose(n.length(), 1, 1e-10)) // Remove this to caller function
      fprintf (stderr, "Direction is not normalized!!");
    if (isNotClose(w.length(), 1, 1e-10))
      fprintf (stderr, "Direction is not normalized!!");

    Random random(clock() & 0xFFFFFFFF);
    int offset = random.randomMTD(0, (MULTIPLIER - 2) * nSAMPLES); // We'll use two random number on every iteration.

    // Z - axis is transfomed to w.

    Vec newX; // X - axis transfomed to this axis.
    // Let's find a Vector perpendicular to w.
    if (w.x != 0)
      newX = Vec((-w.y-w.z)/w.x, 1.0, 1.0);
    else if (w.y != 0)
      newX = Vec(1.0,  -w.z/w.y, 1.0); // since w.x is zero, we can simplify 2nd dimension
    else if (w.z != 0)
      newX = Vec(1.0, 1.0, 0); // Since both w.x and w.y are zero.
    else
      fprintf (stderr, "WTF is w??\n");

    newX.norm();

    Vec newY = (w%newX).norm();

    //Our transformation matrix is [newX, newY, w]
    for (int i = 0, j = offset & 0xFFFFFFF0; i < nSamples; i++, j += 2) {
      double e1 = randoms[j];
      double e2 = randoms[j+1];
      double costheta = pow((1.0 - e1), 1.0/(e + 1));
      double theta = acos(costheta);
      double phi = 2 * PI * e2;

      double sintheta = sin(theta);
      double cosphi = cos(phi);
      double sinphi = sin(phi);
      double x = sintheta * cosphi;
      double y = sintheta * sinphi;
      double z = costheta;

      //Apply transformation, rotate
      store[i] = Vec(x * newX.x + y * newY.x + z * w.x,
		       x * newX.y + y * newY.y + z * w.y,
		       x * newX.z + y * newY.z + z * w.z);


    }

    return 1; // For normalized phong.
}

//Solid angle imortance sampling!!
//Sample around w as axis, centered at x with sample points making maximum angle of theta_max with w.
// This implementation is slower but numerically more stable.
// Assume normalized w
double SphericalSampler::getSolidDirectionSamples(const Vec &w, const double theta_max, const int nSamples, Vec *store) {
  if (isNotClose(w.length(), 1, 1e-10))
      fprintf (stderr, "Direction is not normalized!!");

  // calculate area of cap using cap-hat theorem
  double A = 2.0 * PI * (1 - cos(theta_max));

  Random random(clock() & 0xFFFFFFFF);
  int offset = random.randomMTD(0, (MULTIPLIER - 2) * nSAMPLES); // We'll use two random number on every iteration.

  // Z - axis is transfomed to w axis.

  Vec newX; // X - axis transfomed to this axis.
  // Let's find a Vector perpendicular to w.
  if (w.x != 0)
    newX = Vec((-w.y-w.z)/w.x, 1.0, 1.0);
  else if (w.y != 0)
    newX = Vec(1.0,  -w.z/w.y, 1.0); // since w.x is zero, we can simplify 2nd dimension
  else if (w.z != 0)
    newX = Vec(1.0, 1.0, 0); // Since both w.x and w.y are zero.
  else
    fprintf (stderr, "WTF is w??\n");

  newX.norm();

  Vec newY = (w%newX).norm();

  //Our transformation matrix is [newX, newY, w]
  for (int i = 0, j = offset & 0xFFFFFFF0; i < nSamples; i++, j += 2) {
    double e1 = randoms[j];
    double e2 = randoms[j+1];
    double costheta = 1.0 - (A * e1) / (2.0 * PI);
    double theta = acos(costheta);
    double phi = 2 * PI * e2;

    double sintheta = sin(theta);
    double cosphi = cos(phi);
    double sinphi = sin(phi);
    double x = sintheta * cosphi;
    double y = sintheta * sinphi;
    double z = costheta;

    //Apply transformation, rotate
    store[i] = Vec(x * newX.x + y * newY.x + z * w.x,
		     x * newX.y + y * newY.y + z * w.y,
		     x * newX.z + y * newY.z + z * w.z);
  }

  return A;
}

// This implementation is slightly faster but less numerically satble. You will see artifacts when size of light source gets smaller.
//#define DEBUG_ARCSS 0
/*double SphericalSampler::getSolidSurfaceSamples(const Vec &w, Vec x, double theta_max, int nSamples, Vec *store) {
  if (isNotClose(w.length(), 1, 1e-10))
      fprintf (stderr, "Direction is not normalized!!");

  Random random(clock() & 0xFFFFFFFF);
  double eps = 1e-14;

  double eita = w.x * w.x + w.y * w.y;
  double alpha = w.z * w.z + eita; // Parabola opens upwards since alpha >= 0.

  for (int i = 0; i < nSamples;) {
    double theta = random.randomMTD(theta_max/1000.0, theta_max);
    double delta1 = cos(theta);

    double beta = -2.0 * delta1 * w.z;
    double gamma = delta1 * delta1 - eita;

    double disc = beta * beta - 4 * alpha * gamma;

    if (disc * disc < eps)  {// discriminant almost zero, degenerate case
      double o = -beta / (2 * alpha);

      double r = sqrt(1 - o*o);

      double m = random.randomMTD(0.0, r);
      double n1 = sqrt (r * r - m * m);
      double n2 = -n1;

#if DEBUG_ARCSS
      fprintf(stdout, "C1:%f %f %f %f %f %f\n", m , n1, o, (Vec(m, n1, o).norm()).dot(w), delta1, theta);
      fprintf(stdout, "C2:%f %f %f %f %f %f\n", m , n2, o, (Vec(m, n2, o).norm()).dot(w), delta1, theta);
#endif
      store[i] = x + Vec(m, n1, o).norm();
      store[i + 1] = x + Vec(m, n2, o).norm();

      i += 2;
    }
    else if (disc > 0) {
      double o1, o2;
      if (beta >= 0) { // This is more numerically stable than directly computing two roots.
        double temp = -(beta + sqrt(disc));
        o1 = temp / (2.0 * alpha);
        o2 = (2 * gamma) / temp;
      }
      else {
        double temp = (-beta) + sqrt(disc);
        o1 = (2 * gamma) / temp;
        o2 = temp / (2 * alpha);
      }

      double eps2 = (o2 - o1) / 1000.0;
      double o = randomMTD(o1 + eps2, o2 - eps2); //if o = o1 or o2 then disc1 is zero. In this case the line is tangential to the circle.

      double sdelta1 = 1 - o * o;
      double sdelta2 = delta1 - o * w.z;
      double alpha1 = w.x * w.x + w.y * w.y;
      double beta1 = -2.0 * w.y * sdelta2;
      double gamma1 = sdelta2 * sdelta2 - w.x * w.x *sdelta1;

      double disc1 = beta1 * beta1 - 4 * alpha1 * gamma1;
      double n1, n2;
      if (disc1 * disc1 < eps) {
        n1 = n2 = -beta1 / (2 * alpha1);
        //if (w.x * w.x < eps * 100000) { // Plane parallel to X axis, i.e if w.x = 0, then disc1 is 0.
	  double m1, m2;
	  m1 = sqrt(sdelta1 - n1 * n1);
	  m2 = -m1;
#if DEBUG_ARCSS
	  fprintf(stdout, "D1:%f %f %f %f %f %f\n", m1 , n1, o, (Vec(m1, n1, o).norm()).dot(w), delta1, theta);
          fprintf(stdout, "D2:%f %f %f %f %f %f\n", m2 , n2, o, (Vec(m2, n2, o).norm()).dot(w), delta1, theta);
#endif
	  store[i] = x + Vec(m1, n1, o).norm();
	  store[i + 1] = x + Vec(m2, n2, o).norm();
	  i += 2;
        //}
        // Other reason for disc1 is 0 because o = o1 or o = o2. But the way o is generated, such condition will never happen!!
      }
      else if (beta1 >= 0) {
        double temp = -(beta1 + sqrt(disc1));
        double m1, m2;
        n1 = temp / (2.0 * alpha1);
        n2 = (2 * gamma1) / temp;

        m1 = (sdelta2 - n1 * w.y) / w.x;
        m2 = (sdelta2 - n2 * w.y) / w.x;
#if DEBUG_ARCSS
        fprintf(stdout, "A1:%f %f %f %f %f %f\n", m1 , n1, o, (Vec(m1, n1, o).norm()).dot(w), delta1, theta);
        fprintf(stdout, "A2:%f %f %f %f %f %f\n", m2 , n2, o, (Vec(m2, n2, o).norm()).dot(w), delta1, theta);
#endif
	store[i] = x + Vec(m1, n1, o).norm();
	store[i + 1] = x + Vec(m2, n2, o).norm();

	i += 2;
      }
      else {
        double temp = (-beta1) + sqrt(disc1);
        double m1, m2;
        n1 = (2 * gamma1) / temp;
        n2 = temp / (2 * alpha1);

        m1 = (sdelta2 - n1 * w.y) / w.x;
        m2 = (sdelta2 - n2 * w.y) / w.x;
#if DEBUG_ARCSS
        fprintf(stdout, "B1:%f %f %f %f %f %f\n", m1, n1, o, (Vec(m1, n1, o).norm()).dot(w), delta1, theta);
        fprintf(stdout, "B2:%f %f %f %f %f %f\n", m2, n2, o, (Vec(m2, n2, o).norm()).dot(w), delta1, theta);
#endif
	store[i] = x + Vec(m1, n1, o).norm();
	store[i + 1] = x + Vec(m2, n2, o).norm();

	i += 2;
      }
    }
#if DEBUG_ARCSS
    fprintf(stdout, "%f %f %f %f\n", disc, w.x, w.y, w.z);
#endif
  }

  // Use Cap-hat theorem to return 1/pdf which is essentially the area of spherical cap subtended by the light.
  return 2.0 * PI * (1 - cos(theta_max));

}*/

void SphericalSampler::getDistribution(const Vec &n, const Vec &x, const int nSamples, Vec *samples) {
    int *histogramElevation = new int[181]; // 180 degrees
    int *histogramAzmuth = new int[361]; // 360 degrees
    int *histogramRadii = new int[101]; // 100 uniform samples along radius length.

    double meanElevation = 0, meanAzmuth = 0, meanRadii = 0;

    Vec t = Vec(1,1,1);
    Vec nOrtho = (n%t).norm(); // Vector orthogonal to n. This is our reference for calculating azmuth.

    int i;
    for (i = 0; i < 181; i++)
      histogramElevation[i] = 0;

    for (i = 0; i < 361; i++)
      histogramAzmuth[i] = 0;

    for (i = 0; i < 101; i++)
      histogramRadii[i] = 0;

    for (int i = 0; i < nSamples; i++) {
	Vec dir = (samples[i] - x);
	double length = dir.length();
	dir.norm();
	double magnitudeProjN = n.dot(dir);

	Vec projN = n * magnitudeProjN;
	Vec projAzmuth = (dir - projN).norm(); // Projection of dir on plane defined by normal n.
	double elevationAngle = acos(magnitudeProjN) * 180.0 / PI;
	double azmuthAngle = acos(projAzmuth.dot(nOrtho)) * 180.0 / PI;
	if ((projAzmuth%nOrtho).dot(n) < 0)
	    azmuthAngle = 360 - azmuthAngle;

	histogramElevation[(int)elevationAngle]++;
	histogramAzmuth[(int)azmuthAngle]++;
	if (length > 1.01)
	  fprintf(stderr, "Not a unit sphere.\n");
	else
	  histogramRadii[int(length * 100.0)]++;

	meanElevation += ((int)elevationAngle) + 0.5;
	meanAzmuth += ((int)azmuthAngle) + 0.5;
	meanRadii += ((int)(length * 100)) + 0.5;
    }

    meanAzmuth /= nSamples;
    meanElevation /= nSamples;
    meanRadii /= nSamples;

    double stdElevation = 0, stdAzmuth = 0, stdRadii = 0;
    //fprintf(stderr, "Elevation Distribution.\n");
    for (i = 0; i < 181; i++) {
	//fprintf(stderr, "%d %d %d\n", i, i+1, histogramElevation[i]);
	stdElevation += (i + 0.5 - meanElevation) * (i + 0.5 - meanElevation) * histogramElevation[i];
    }

    //fprintf(stderr, "Azmuth Distribution.\n");
    for (i = 0; i < 361; i++) {
	//fprintf(stderr, "%d %d %d\n", i, i+1, histogramAzmuth[i]);
	stdAzmuth += (i + 0.5 - meanAzmuth) * (i + 0.5 - meanAzmuth) * histogramAzmuth[i];
    }

    //fprintf(stderr, "Radii Distribution.\n");
    for (i = 0; i < 101; i++) {
	//fprintf(stderr, "%d %d %d\n", i, i+1, histogramRadii[i]);
	stdRadii += (i + 0.5 - meanRadii) * (i + 0.5 - meanRadii) * histogramRadii[i] / 10000.0;
    }

    stdAzmuth = sqrt(stdAzmuth/nSamples);
    stdElevation = sqrt(stdElevation/nSamples);
    stdRadii = sqrt(stdRadii/nSamples);

    fprintf(stderr, "Mean Elevation=%f, Mean Azmuth=%f, Mean Radii=%f\n", meanElevation, meanAzmuth, meanRadii / 100.0);
    fprintf(stderr, "StdDev Elevation=%f, StdDev Azmuth=%f, StdDev Radii=%f\n", stdElevation, stdAzmuth, stdRadii);
}
