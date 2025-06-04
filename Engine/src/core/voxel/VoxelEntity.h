#pragma once

#include "VoxelMesh.h"

namespace Engine {	

	class VoxelEntity
	{
	public:
		VoxelMesh Mesh;
		Transformation Transform;
	};

}