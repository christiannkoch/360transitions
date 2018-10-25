/*
Author: Arne-Tobias Rak
TU Darmstadt
*/

#pragma once

#include <cmath>
#include <map>
#include <deque>
#include <numeric>
#include <algorithm>

#include "Quaternion.hpp"
#include "mpd.h"

#define TIME_NOW_EPOCH_MS std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()
#define SAMPLERES 8
#define SAMPLEPOINTS (SAMPLERES+1)*(SAMPLERES+1)

#define TIMER auto ttt = TIME_NOW_EPOCH_MS
#define TIMEROUT(s) auto ttt2 = TIME_NOW_EPOCH_MS; std::cout << s << " TIMER: " << ttt2 - ttt << std::endl

float PI = 3.141592653589793238462643383279502884L;

static const double monocular_horizontal = 92.0;
static const double monocular_vertical = 92.0;

static const double maxHDist = 2 * std::tan(monocular_horizontal * PI / 180.0 / 2.0);
static const double maxVDist = 2 * std::tan(monocular_vertical * PI / 180.0 / 2.0);

using namespace IMT;

class AdaptionUnit
{
public:
	struct NormalizedCoordinate { double x, y; };

	AdaptionUnit(const DASH::MPD* mpd) : mpd(mpd)
	{
		auto srd = mpd->period.adaptationSets[0].srd;

		int frameWidth = srd.w * srd.th;
		int frameHeight = srd.h * srd.tv;

		for (int i = 0; i < mpd->period.adaptationSets.size(); i++)
		{
			srd = mpd->period.adaptationSets[i].srd;

			double normalizedCoordX = (srd.x + srd.w) / (double)frameWidth;
			double normalizedCoordY = (srd.y + srd.h) / (double)frameHeight;

			normalizedCoordTileMapping[normalizedCoordX][normalizedCoordY] = i;
		}

		for (int i = 0; i <= SAMPLERES; i++)
			for (int j = 0; j <= SAMPLERES; j++)
				samplePoints[i * (SAMPLERES + 1) + j] = { i * (1.0 / SAMPLERES), j * (1.0 / SAMPLERES) };
	}

	std::map<int, int> computeTileVisibility(const Quaternion& headRotation) const
	{
		std::map<int, int> tileVisibilityMap;

		for (int j = 0; j < SAMPLEPOINTS; j++)
			tileVisibilityMap[mapCoordToTile(fromViewportCoordToEquirectCoord(headRotation, samplePoints[j]))]++;

		return tileVisibilityMap;
	}

private:
	const DASH::MPD* mpd;
	std::map<double, std::map<double, int>> normalizedCoordTileMapping;
	std::map<int, int> tileQuality;
	NormalizedCoordinate samplePoints[SAMPLEPOINTS];

	int mapCoordToTile(NormalizedCoordinate coord) const
	{
		return normalizedCoordTileMapping.lower_bound(coord.x)->second.lower_bound(coord.y)->second;
	}

	static NormalizedCoordinate fromViewportCoordToEquirectCoord(const Quaternion& headRotation, const NormalizedCoordinate& viewportCoord)
	{
		double u = (viewportCoord.x - 0.5) * (2 * maxHDist);
		double v = (0.5 - viewportCoord.y) * (2 * maxVDist);

		VectorCartesian coordBefRot(1, u, v);
		coordBefRot /= coordBefRot.Norm();

		VectorSpherical pixel3dPolar = headRotation.Rotation(coordBefRot);

		NormalizedCoordinate equirectCoord;
		equirectCoord.x = 1.0 - std::fmod(0.75 + pixel3dPolar.GetTheta() / (2.0*PI), 1.0);
		equirectCoord.y = pixel3dPolar.GetPhi() / PI;

		return equirectCoord;
	}
};
