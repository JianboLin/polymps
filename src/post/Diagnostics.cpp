#include "Diagnostics.h"
#include <algorithm>
#include <cmath>
#include <iomanip>

Diagnostics::Diagnostics()
	: initialized(false)
	, cumulativeDissipation(0.0)
	, cumulativeCapillaryWork(0.0)
	, cachedInitialRadius(0.0) {
}

Diagnostics::~Diagnostics() {
	finalize();
}

void Diagnostics::initialize(MpsParticleSystem *PSystem, MpsParticle *Particles) {
	if(PSystem == nullptr || Particles == nullptr) return;

	cumulativeDissipation = 0.0;
	cumulativeCapillaryWork = 0.0;
	cachedInitialRadius = PSystem->initialRadius;

	std::string outFolder = PSystem->outputDir.empty() ? std::string("output") : PSystem->outputDir;
	historyFilepath = outFolder + "/history.csv";
	capillaryDebugFilepath = outFolder + "/capillary_debug.csv";
	history.open(historyFilepath, std::ios::out);
	capillaryDebug.open(capillaryDebugFilepath, std::ios::out);
	if(history.is_open()) {
		history << "t,R,H,beta,theta_deg,KE,Diss,Wcap\n";
		history << std::scientific << std::setprecision(12);
	}
	if(capillaryDebug.is_open()) {
		capillaryDebug << "t,capPower,maxAccCap,nearWallFluid,strengthLL,strengthWL,reCapillary,clipDist\n";
		capillaryDebug << std::scientific << std::setprecision(12);
	}
	ensureInitialRadius(PSystem, Particles);
	initialized = history.is_open();
}

void Diagnostics::sample(MpsParticleSystem *PSystem, MpsParticle *Particles) {
	if(!initialized || PSystem == nullptr || Particles == nullptr) return;
	if(PSystem->historyStep <= 0) return;
	if(PSystem->numOfIterations % PSystem->historyStep != 0) return;

	ensureInitialRadius(PSystem, Particles);

	double R = 0.0, H = 0.0, beta = 0.0;
	computeGeometry(PSystem, Particles, R, H, beta);
	double thetaDeg = computeContactAngleDeg(R, H);

	double KE = 0.0, dissPower = 0.0, capPower = 0.0;
	computeEnergy(PSystem, Particles, KE, dissPower, capPower);

	cumulativeDissipation += -dissPower * PSystem->timeStep;
	if(cumulativeDissipation < 0.0) cumulativeDissipation = 0.0;
	cumulativeCapillaryWork += capPower * PSystem->timeStep;

	if(history.is_open()) {
		history << PSystem->timeCurrent << "," << R << "," << H << "," << beta << "," << thetaDeg << ",";
		history << KE << "," << cumulativeDissipation << "," << cumulativeCapillaryWork << "\n";
		history.flush();
	}

	if(capillaryDebug.is_open()) {
		double maxAccCap = 0.0;
		int nearWallFluid = 0;
		for(int i=0; i<Particles->numParticles; ++i) {
			if(Particles->particleType[i] != PSystem->fluid) continue;
			const double ax = Particles->accCapillary[i*3  ];
			const double ay = Particles->accCapillary[i*3+1];
			const double az = Particles->accCapillary[i*3+2];
			const double nrm = std::sqrt(ax*ax + ay*ay + az*az);
			if(std::isfinite(nrm) && nrm > maxAccCap) maxAccCap = nrm;
			if(Particles->particleNearWall[i]) nearWallFluid++;
		}
		const double clipDist = std::max(PSystem->pairwiseShortClip * PSystem->partDist, PSystem->epsilonZero);
		capillaryDebug << PSystem->timeCurrent << "," << capPower << "," << maxAccCap << "," << nearWallFluid << ","
			<< PSystem->pairwiseStrength << "," << PSystem->pairwiseStrengthWall << "," << PSystem->reCapillary << "," << clipDist << "\n";
		capillaryDebug.flush();
	}
}

void Diagnostics::finalize() {
	if(history.is_open()) {
		history.close();
	}
	if(capillaryDebug.is_open()) {
		capillaryDebug.close();
	}
	initialized = false;
}

void Diagnostics::ensureInitialRadius(MpsParticleSystem *PSystem, MpsParticle *Particles) {
	if(cachedInitialRadius > PSystem->epsilonZero) return;

	double R = 0.0, H = 0.0, beta = 0.0;
	computeGeometry(PSystem, Particles, R, H, beta);
	if(R > PSystem->epsilonZero) {
		cachedInitialRadius = R;
		PSystem->initialRadius = R;
	}
}

void Diagnostics::computeGeometry(MpsParticleSystem *PSystem, MpsParticle *Particles, double &R, double &H, double &beta) {
	double massSum = 0.0;
	double cx = 0.0, cy = 0.0;
	for(int i=0; i<Particles->numParticles; ++i) {
		if(Particles->particleType[i] != PSystem->fluid) continue;
		const double rho = Particles->RHO[i];
		if(!std::isfinite(rho)) continue;
		const double mass = rho * PSystem->particleVolume;
		if(!std::isfinite(mass) || mass <= 0.0) continue;
		massSum += mass;
		cx += mass * Particles->pos[i*3  ];
		cy += mass * Particles->pos[i*3+1];
	}
	if(massSum > PSystem->epsilonZero) {
		cx /= massSum;
		cy /= massSum;
	}

	double maxR = 0.0;
	double maxZ = -1e9;
	for(int i=0; i<Particles->numParticles; ++i) {
		if(Particles->particleType[i] != PSystem->fluid) continue;
		if(!std::isfinite(Particles->pos[i*3  ]) || !std::isfinite(Particles->pos[i*3+1]) || !std::isfinite(Particles->pos[i*3+2])) continue;
		const double dx = Particles->pos[i*3  ] - cx;
		const double dy = Particles->pos[i*3+1] - cy;
		const double dz = Particles->pos[i*3+2];
		const double r = std::sqrt(dx*dx + dy*dy);
		if(r > maxR) maxR = r;
		if(dz > maxZ) maxZ = dz;
	}

	R = maxR;
	H = std::max(0.0, maxZ - PSystem->substrateLevel);
	const double denom = (cachedInitialRadius > PSystem->epsilonZero) ? cachedInitialRadius : R;
	beta = (denom > PSystem->epsilonZero) ? (R / denom) : 0.0;
	if(!std::isfinite(R)) R = 0.0;
	if(!std::isfinite(H)) H = 0.0;
	if(!std::isfinite(beta)) beta = 0.0;
}

void Diagnostics::computeEnergy(MpsParticleSystem *PSystem, MpsParticle *Particles, double &KE, double &dissPower, double &capPower) {
	KE = 0.0;
	dissPower = 0.0;
	capPower = 0.0;
	for(int i=0; i<Particles->numParticles; ++i) {
		if(Particles->particleType[i] != PSystem->fluid) continue;
		const double mass = Particles->RHO[i] * PSystem->particleVolume;
		if(!std::isfinite(mass) || mass <= 0.0) continue;
		const double vx = Particles->vel[i*3  ];
		const double vy = Particles->vel[i*3+1];
		const double vz = Particles->vel[i*3+2];
		if(!std::isfinite(vx) || !std::isfinite(vy) || !std::isfinite(vz)) continue;
		KE += 0.5 * mass * (vx*vx + vy*vy + vz*vz);

		const double viscDot = Particles->accViscous[i*3  ]*vx + Particles->accViscous[i*3+1]*vy + Particles->accViscous[i*3+2]*vz;
		const double capDot = Particles->accCapillary[i*3  ]*vx + Particles->accCapillary[i*3+1]*vy + Particles->accCapillary[i*3+2]*vz;
		if(std::isfinite(viscDot)) dissPower += mass * viscDot;
		if(std::isfinite(capDot)) capPower += mass * capDot;
	}
}

double Diagnostics::computeContactAngleDeg(double R, double H) const {
	if(R <= 0.0 || H <= 0.0) return 0.0;
	double theta = 2.0 * std::atan2(H, R);
	return theta * 180.0 / 3.14159265358979323846;
}
