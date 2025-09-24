#pragma once
#include "pch.h"
#include "Utils.h"

class Tournaments {
public:
	static void Kill(AFortPlayerControllerAthena*);
	static void Placement(int32, int32);
	static void PlacementForController(AFortPlayerControllerAthena*, int32);
};