#if !defined(materialTypes_h__)
#define materialTypes_h__

enum LightType {NONE = 0, POINT, VOLUME, PLANAR, MESH};
enum BRDFType {BRDF_NONE = 0, CLASSIC_PHONG, GGX};
class MaterialType {
  public:
    double phongExp; //Classical phong BRDF exponent
    double specularCoef;

    double intIOR; //interior IOR(Index of Refraction)
    double extIOR; //exterior IOR

    double ggxAlpha; // GGX BRDF roughness

    LightType l;
    Vec radiance;

    // Assumes w and n is normalized!!
    Vec getRadiance(const Vec &n, const Vec &w) {
      // This is true only for sphere volume sources.
      return radiance;
    }
    //Dummy Material
    MaterialType() {
      phongExp = 0;
      specularCoef = 0;

      ggxAlpha = 0;
      intIOR = 0;
      extIOR = 0;

      l = NONE;
      radiance = Vec(0, 0, 0);

      b = BRDF_NONE;
    }

    //Light Material
    MaterialType(LightType lt, const Vec &rad): l(lt), radiance(rad) {
      phongExp = 0;
      specularCoef = 0;

      ggxAlpha = 0;
      intIOR = 0;
      extIOR = 0;

      b = BRDF_NONE;
    }

    //For CLASSIC_PHONG Material
    MaterialType(double pE, double sC) {
      phongExp = (pE >= 0) ? pE : -pE;
      specularCoef = (sC * ((sC < 0)? -1.0: 1.0));
      specularCoef = specularCoef <= 1 ? specularCoef : 1;

      ggxAlpha = 0;
      intIOR = 0;
      extIOR = 0;

      l = NONE;
      radiance = Vec(0.0, 0.0, 0.0);

      b = CLASSIC_PHONG;
    }

    //For GGX Material
    MaterialType(double alpha, double intIOR, double extIOR, double sC) {
      ggxAlpha = alpha;
      this->intIOR = intIOR;
      this->extIOR = extIOR;

      specularCoef = (sC * ((sC < 0)? -1.0: 1.0));
      specularCoef = specularCoef <= 1 ? specularCoef : 1;

      // Convert GGX to phong instead of setting phongExp to 0.
      phongExp = 0;

      l = NONE;
      radiance = Vec(0, 0, 0);

      b = GGX;
    }

    double brdf(Vec n, Vec wo_ref, Vec wo, Vec wi);
  private:
    BRDFType b;
    // Dot product product between Half angle and wi or wo.
    double fresnel(double c) const;
    double ggxDistribution(const Vec &h, const Vec &n) const;
    double ggxG1(const Vec &v, const Vec &h, const Vec &n) const;
};

#endif
