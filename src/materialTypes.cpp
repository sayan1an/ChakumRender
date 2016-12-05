#include "mathPrimitives.h"
#include "materialTypes.h"
#include "domainSampler.h"
#include <cmath>
#include <iostream>

// Assumes wo.dot(n) and wi.dot(n) are positive
double MaterialType::brdf(Vec n, Vec wo_ref, Vec wo, Vec wi) {
  if (b == CLASSIC_PHONG) {
    double cosine = wo_ref.dot(wi);
    //cosine > 0?cosine:0
    return (specularCoef * (phongExp + 1) * pow (cosine > 0?cosine:0, phongExp) / (2 * PI)) + (1 - specularCoef) / PI;
  }
  else if (b == GGX) {
      Vec h = (wo + wi).norm();

      double F = fresnel(h.dot(wi));
      double D = ggxDist(h, n);
      double G = ggxG1(wo, h, n) * ggxG1(wi, h, n);

      return (specularCoef * F * D * G) / (4.0 * wi.dot(n) * wo.dot(n)) + (1 - specularCoef) / PI;
  }
  else if (b == BECKMANN) {
      Vec h = (wo + wi).norm();

      double F = fresnel(h.dot(wi));
      double D = beckmannDist(h, n);
      double G = beckmannG1(wo, h, n) * beckmannG1(wi, h, n);

      return (specularCoef * F * D * G) / (4.0 * wi.dot(n) * wo.dot(n)) + (1 - specularCoef) / PI;
  }
  else if (b == PHONG) {
      Vec h = (wo + wi).norm();

      double F = fresnel(h.dot(wi));
      double D = (alpha + 2) * pow (h.dot(n), alpha) / (2 * PI);
      double G = phongG1(wo, h, n) * phongG1(wi, h, n);

      return (specularCoef * F * D * G) / (4.0 * wi.dot(n) * wo.dot(n)) + (1 - specularCoef) / PI;
  }
  else
    std::cerr<<"Undefined BRDF!!\n";

  return 0.0;
}

double MaterialType::pdfEval(Vec n, Vec wo_ref, Vec wo, Vec wi) {

  // Assumes Cosine weighted sampling
  double diffuse = (1.0 - specularCoef) * wi.dot(n) / PI;
  double spec = specularCoef;

  if (b == CLASSIC_PHONG) {
      spec *= ((phongExp + 1)  * pow(wi.dot(wo_ref), phongExp)) / (2 * PI);
  }
  else if (b == GGX || b == BECKMANN || b == PHONG) {
      Vec h = (wo + wi).norm();
      double D = 0;
      if (b == GGX)
	D = ggxDist(h, n);
      else if (b == BECKMANN)
	D = beckmannDist(h, n);
      else if (b == PHONG)
	D = (alpha + 2) * pow (h.dot(n), alpha) / (2 * PI);

      spec *= D * h.dot(n) / (4.0 * h.dot(wo));
  }
  else
    std::cerr<<"Undefined BRDF sampling!!\n";

  return spec + diffuse;

}

void MaterialType::getBrdfDirectionSamples(Vec n, Vec wo_ref, Vec wo, Vec *samples, double *weights, uint32_t nSamples) {
  uint32_t nBrdfSamples = nSamples * specularCoef;
  const static double divideByZero = 1e-10;

  if (b == CLASSIC_PHONG)
    SphericalSampler::getClassicPhongDirectionSamples(n, wo_ref, phongExp, nBrdfSamples, samples);
  else if (b == GGX || b == BECKMANN || b == PHONG)
    SphericalSampler::getBrdfDirectionSamples(n, wo, alpha, nBrdfSamples, samples, b);
  else
    std::cerr<<"Undefined BRDF sampling!!\n";

  // If you replace diffuse sampling with any other scehem then change the diffuse pdf in pdfEval
  SphericalSampler::getCosineDirectionSamples(n, nSamples - nBrdfSamples, samples + nBrdfSamples);

  for (uint32_t i = 0; i < nSamples && weights != 0; i++)
    weights[i] = 1.0 / (pdfEval(n, wo_ref, wo, samples[i]) + divideByZero);
}

//See Microfacet BSDF paper, Bruce Walter
double inline MaterialType::fresnel(double c) const {
  double g = intIOR/extIOR;
  g *= g;
  g += c*c - 1.0;
  g = sqrt(g);

  double t1 = g - c;
  double t2 = g + c;

  double temp = c * t2 - 1.0;
  double F = temp * temp;

  temp = c * t1 + 1.0;
  F /= (temp * temp);
  F += 1.0;

  temp = t1 / t2;
  temp *= temp;

  F *= 0.5 * temp;

  return F;
}

double MaterialType::ggxDist(const Vec &h,const Vec &n) const {
  double cosine_sq = h.dot(n);
  cosine_sq *= cosine_sq;
  double tan_sq = 1.0 / cosine_sq - 1.0;
  double alpha_sq = alpha * alpha;

  tan_sq += alpha_sq;
  tan_sq *= tan_sq;
  double D = alpha_sq / (PI * cosine_sq * cosine_sq * tan_sq);

  return D;
}

double MaterialType::ggxG1(const Vec &v, const Vec &h, const Vec &n) const {
  double cosv_sq = v.dot(n);
  cosv_sq *= cosv_sq;
  double tanv_sq = 1.0 / cosv_sq - 1.0;

  if (tanv_sq <= 1e-15)
    return 1.0;
  else if (h.dot(v) <= 1e-15)
    return 0;

  double alpha_sq = alpha * alpha;

  tanv_sq *= alpha_sq;
  tanv_sq += 1.0;
  tanv_sq = 1.0 + sqrt(tanv_sq);

  return 2.0/tanv_sq;
}

double MaterialType::beckmannDist(const Vec &h, const Vec &n) const {
  double cosine_sq = h.dot(n);
  cosine_sq *= cosine_sq;
  double tan_sq = 1.0 / cosine_sq - 1.0;
  double alpha_sq = alpha * alpha;

  tan_sq /= alpha_sq;
  tan_sq *= -1.0;

  return exp(tan_sq) / (PI * cosine_sq * cosine_sq * alpha_sq);
}

double MaterialType::beckmannG1(const Vec &v, const Vec &h, const Vec &n) const {
  double cosv_sq = v.dot(n);
  cosv_sq *= cosv_sq;
  double a = 1.0 / (alpha * sqrt(1.0 / cosv_sq - 1.0));

  if (cosv_sq >= 0.999999)
    return 1.0;
  else if (h.dot(v) <= 1e-15)
    return 0;

  if (a < 1.6) {
    return (3.535 * a + 2.181 * a * a) / (1.0 + 2.276 * a + 2.577 * a * a);
  }

  return 1;
}

double MaterialType::phongG1(const Vec &v, const Vec &h, const Vec &n) const {
  double cosv_sq = v.dot(n);
  cosv_sq *= cosv_sq;
  double a = 0.5 * alpha + 1;
  a /= (1.0 / cosv_sq - 1.0);
  a = sqrt(a);

  if (cosv_sq >= 0.999999)
    return 1.0;
  else if (h.dot(v) <= 1e-15)
    return 0;

  if (a < 1.6) {
    return (3.535 * a + 2.181 * a * a) / (1.0 + 2.276 * a + 2.577 * a * a);
  }

  return 1;
}

