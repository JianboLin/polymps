// Copyright (c) 2021 Rubens AMARO
// Distributed under the MIT License.

#include <experimental/filesystem> 	///< numeric_limits
#include "MpsParticleSystem.h"

using namespace std;

// Constructor declaration
MpsParticleSystem::MpsParticleSystem()
	: surfaceTension(0.0)
	, pairwiseCapillary(false)
	, pairwiseCSigma(0.0)
	, pairwiseReOverDx(0.0)
	, pairwiseShortClip(0.3)
	, pairwiseWettingScale(1.0)
	, contactAngle(0.0)
	, cosContactAngle(0.0)
	, pairwiseStrength(0.0)
	, pairwiseStrengthWall(0.0)
	, reCapillary(0.0)
	, reCapillary2(0.0)
	, particleVolume(0.0)
	, initialRadius(0.0)
	, substrateLevel(0.0)
	, outputDir("output")
	, iterOutputTime(0.0)
	, historyStep(0) {
}
// Destructor declaration
MpsParticleSystem::~MpsParticleSystem()
{
}
