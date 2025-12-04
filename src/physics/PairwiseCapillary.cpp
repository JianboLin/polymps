#include "PairwiseCapillary.h"
#include <algorithm>

void PairwiseCapillary::compute(MpsParticleSystem *PSystem, MpsParticle *Particles, MpsBucket *Buckets) {
	static bool warned = false;
	static bool sampled = false;
	static long callCount = 0;
	callCount++;
	if(PSystem == nullptr || Particles == nullptr || Buckets == nullptr) return;
	if(!PSystem->pairwiseCapillary) {
		if(!warned) { printf("[pairwise] disabled (pairwiseCapillary=false)\n"); warned = true; }
		return;
	}
	if(PSystem->pairwiseStrength <= 0.0 || PSystem->reCapillary <= PSystem->epsilonZero) {
		if(!warned) {
			printf("[pairwise] inactive: strength=%.3e reCapillary=%.3e (eps=%.3e)\n",
				PSystem->pairwiseStrength, PSystem->reCapillary, PSystem->epsilonZero);
			warned = true;
		}
		return;
	}

	const double re = PSystem->reCapillary;
	const double re2 = PSystem->reCapillary2;
	const double clipDist = std::max(PSystem->pairwiseShortClip * PSystem->partDist, PSystem->epsilonZero);
	const double strengthLL = PSystem->pairwiseStrength;
	const double strengthWL = PSystem->pairwiseStrengthWall;
	long long totalLL = 0;
	long long totalWL = 0;
	double maxW = 0.0;
	double maxAccNorm = 0.0;
	if(callCount <= 1) {
		// Bruteforce check around particle 0 to ensure geometry/threshold合理
		int i0 = -1;
		for(int i=0; i<Particles->numParticles; ++i) {
			if(Particles->particleType[i] == PSystem->fluid) { i0 = i; break; }
		}
		if(i0 >= 0) {
			int neighborBruteforce = 0;
			double minDstBF = 1e9;
			for(int j=0; j<Particles->numParticles; ++j) {
				if(j == i0 || Particles->particleType[j] != PSystem->fluid) continue;
				double rx = Particles->pos[j*3  ] - Particles->pos[i0*3  ];
				double ry = Particles->pos[j*3+1] - Particles->pos[i0*3+1];
				double rz = Particles->pos[j*3+2] - Particles->pos[i0*3+2];
				const double dst2 = rx*rx + ry*ry + rz*rz;
				if(dst2 < re2) {
					neighborBruteforce++;
					const double dst = std::sqrt(dst2);
					if(dst < minDstBF) minDstBF = dst;
				}
			}
			printf("[pairwise] bruteforce i0 neighbors within re: %d, minDst=%.3e\n", neighborBruteforce, minDstBF);
		}
	}
	if(callCount <= 2) {
		int nearWallCount = 0;
		int nonEmptyBuckets = 0;
		for(int b=0; b<PSystem->numBucketsXYZ; ++b) {
			if(Particles->firstParticleInBucket[b] != -1) nonEmptyBuckets++;
		}
		for(int i=0; i<Particles->numParticles; ++i) {
			if(Particles->particleType[i] == PSystem->fluid && Particles->particleNearWall[i]) nearWallCount++;
		}
		printf("[pairwise] call %ld iter=%d strengthLL=%.3e strengthWL=%.3e re=%.3e clip=%.3e nearWall=%d nonEmptyBuckets=%d\n",
			callCount, PSystem->numOfIterations, strengthLL, strengthWL, re, clipDist, nearWallCount, nonEmptyBuckets);
	}

#pragma omp parallel for schedule(dynamic,64)
	for(int i=0; i<Particles->numParticles; i++) {
		if(Particles->particleType[i] != PSystem->fluid) continue;

		const double massI = Particles->RHO[i] * PSystem->particleVolume;
		// epsilonZero 对 dx=1e-6 时的质量过大，这里仅防止非正质量
		if(massI <= 0.0 || !std::isfinite(massI)) continue;

		double accX = 0.0, accY = 0.0, accZ = 0.0;
		const double posXi = Particles->pos[i*3  ];
		const double posYi = Particles->pos[i*3+1];
		const double posZi = Particles->pos[i*3+2];

		int ix, iy, iz;
		Buckets->bucketCoordinates(ix, iy, iz, posXi, posYi, posZi, PSystem);
		const int minZ = (iz-1)*((int)(PSystem->dim-2.0));
		const int maxZ = (iz+1)*((int)(PSystem->dim-2.0));
		long long debugCount = 0;
		long long debugWithin = 0;
		double debugMinDst = 1e9;

		for(int jz=minZ;jz<=maxZ;jz++) {
		for(int jy=iy-1;jy<=iy+1;jy++) {
		for(int jx=ix-1;jx<=ix+1;jx++) {
			const int jb = jz*PSystem->numBucketsXY + jy*PSystem->numBucketsX + jx;
			int j = Particles->firstParticleInBucket[jb];
			if(j == -1) continue;
			double plx, ply, plz;
			Particles->getPeriodicLengths(jb, plx, ply, plz, PSystem);
			while(true) {
				double rx, ry, rz, dst2;
				Particles->sqrDistBetweenParticles(j, posXi, posYi, posZi, rx, ry, rz, dst2, plx, ply, plz);

				if(dst2 < re2 && j != i && Particles->particleType[j] == PSystem->fluid) {
					const double dst = sqrt(dst2);
					if(callCount <= 1 && i == 0) {
						debugCount++;
						if(dst < debugMinDst) debugMinDst = dst;
						if(dst2 < re2) debugWithin++;
					}
					if(dst > PSystem->epsilonZero) {
						const double w = kernelWeight(dst, re);
						if(w > 0.0) {
							#pragma omp atomic
							totalLL++;
							if(w > maxW) {
								#pragma omp critical
								if(w > maxW) maxW = w;
							}
							const double rEff = std::max(dst, clipDist);
							const double f = strengthLL * w / rEff;
							const double invMassI = 1.0/massI;
							accX += f * rx/dst * invMassI;
							accY += f * ry/dst * invMassI;
							accZ += f * rz/dst * invMassI;
						}
					}
				}
				j = Particles->nextParticleInSameBucket[j];
				if(j == -1) break;
			}
		}}}

		// Liquid-wall interaction using nearest polygon point (if available)
		if(PSystem->wallType == boundaryWallType::POLYGON && strengthWL > 0.0 && Particles->particleNearWall[i]) {
			const double dx = posXi - Particles->particleAtWallPos[i*3  ];
			const double dy = posYi - Particles->particleAtWallPos[i*3+1];
			const double dz = posZi - Particles->particleAtWallPos[i*3+2];
			const double dst2 = dx*dx + dy*dy + dz*dz;
			if(dst2 > PSystem->epsilonZero && dst2 < re2) {
				const double dst = sqrt(dst2);
				const double w = kernelWeight(dst, re);
				if(w > 0.0) {
					#pragma omp atomic
					totalWL++;
					if(w > maxW) {
						#pragma omp critical
						if(w > maxW) maxW = w;
					}
					const double rEff = std::max(dst, clipDist);
					const double f = strengthWL * w / rEff;
					const double invMassI = 1.0/massI;
					accX += f * dx/dst * invMassI;
					accY += f * dy/dst * invMassI;
					accZ += f * dz/dst * invMassI;
				}
			}
		}

		const double accNorm = std::sqrt(accX*accX + accY*accY + accZ*accZ);
		if(accNorm > maxAccNorm) {
			#pragma omp critical
			{
				if(accNorm > maxAccNorm) maxAccNorm = accNorm;
			}
		}

		Particles->acc[i*3  ] += accX;
		Particles->acc[i*3+1] += accY;
		Particles->acc[i*3+2] += accZ;

		Particles->accCapillary[i*3  ] += accX;
		Particles->accCapillary[i*3+1] += accY;
		Particles->accCapillary[i*3+2] += accZ;
	}

	if(callCount <= 2) {
		printf("[pairwise] summary call %ld: totalLL=%lld totalWL=%lld maxW=%.3e\n",
			callCount, totalLL, totalWL, maxW);
	}
	if(callCount <= 5) {
		printf("[pairwise] maxAccNorm this call = %.3e\n", maxAccNorm);
	}
}
