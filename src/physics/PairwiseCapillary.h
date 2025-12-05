#ifndef POLYMPS_PHYSICS_PAIRWISECAPILLARY_H_
#define POLYMPS_PHYSICS_PAIRWISECAPILLARY_H_

#include <cmath>
#include "MpsParticleSystem.h"
#include "MpsParticle.h"
#include "MpsBucket.h"

class PairwiseCapillary {
public:
	PairwiseCapillary() = default;
	~PairwiseCapillary() = default;

	/**
	 * @brief Add pairwise capillary forces (liquid-liquid and liquid-wall).
	 * @param PSystem Physical/numerical parameters
	 * @param Particles Particle storage
	 * @param Buckets Spatial bucket helper
	 */
	void compute(MpsParticleSystem *PSystem, MpsParticle *Particles, MpsBucket *Buckets);

private:
	inline double kernelWeight(const double dst, const double re) const {
		return (re > 0.0 && dst > 0.0) ? (re/dst - 1.0) : 0.0;
	}
};

#endif // POLYMPS_PHYSICS_PAIRWISECAPILLARY_H_
