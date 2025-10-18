#pragma once

#include "Event.h"

namespace Engine {

	class EventMessenger
	{
	public:
		EventMessenger(Event& e)
			: m_Event(e)
		{
		}

		template<typename T, typename F>
		void send(F fn)
		{
			if (m_Event.get_type() == T::get_static_type())
				fn(static_cast<T&>(m_Event));
		}
	private:
		Event& m_Event;
	};

}