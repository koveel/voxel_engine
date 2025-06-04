#pragma once

#include "Rect.h"

namespace Engine {

	class LayoutSolver
	{
	public:
		LayoutSolver() = default;

		void solve();
	private:
		std::vector<Rect> m_Elements;
	};

}