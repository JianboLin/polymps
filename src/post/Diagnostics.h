#ifndef POLYMPS_POST_DIAGNOSTICS_H_
#define POLYMPS_POST_DIAGNOSTICS_H_

#include <fstream>
#include <string>
#include "MpsParticleSystem.h"
#include "MpsParticle.h"

class Diagnostics {
public:
	Diagnostics();
	~Diagnostics();

	void initialize(MpsParticleSystem *PSystem, MpsParticle *Particles);
	void sample(MpsParticleSystem *PSystem, MpsParticle *Particles);
	void finalize();

private:
	bool initialized;
	double cumulativeDissipation;
	double cumulativeCapillaryWork;
	double cachedInitialRadius;
	std::string historyFilepath;
	std::string capillaryDebugFilepath;
	std::ofstream history;
	std::ofstream capillaryDebug;

	void ensureInitialRadius(MpsParticleSystem *PSystem, MpsParticle *Particles);
	void computeGeometry(MpsParticleSystem *PSystem, MpsParticle *Particles, double &R, double &H, double &beta);
	void computeEnergy(MpsParticleSystem *PSystem, MpsParticle *Particles, double &KE, double &dissPower, double &capPower);
	double computeContactAngleDeg(double R, double H) const;
	inline bool isFinite(double v) const { return std::isfinite(v); }
};

#endif // POLYMPS_POST_DIAGNOSTICS_H_
