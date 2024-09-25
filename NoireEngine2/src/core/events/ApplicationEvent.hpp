/*****************************************************************//**
 * \file   ApplicationEvent.h
 * \brief  Specifies the different events related to the Application
 *
 * \author Hank Xu
 * \date   November 2023
 *********************************************************************/

#pragma once

#include "Event.hpp"

class WindowResizeEvent : public Event
{
public:
	WindowResizeEvent(unsigned int width, unsigned int height)
		: m_Width(width), m_Height(height) {}

	uint32_t m_Width, m_Height;
	// TODO: move this to _DEBUG and avoid using stringstream for allocation!
	std::string ToString() const override
	{
		std::stringstream ss;
		ss << "WindowResizeEvent: " << m_Width << ", " << m_Height;
		return ss.str();
	}

	EVENT_CLASS_TYPE(WindowResize)
	EVENT_CLASS_CATEGORY(EventCategoryApplication)
};

class WindowIconfyEvent : public Event
{
public:
	WindowIconfyEvent(bool minimized)
		: m_Minimized(minimized) {}

	bool m_Minimized;
	// TODO: move this to _DEBUG and avoid using stringstream for allocation!
	std::string ToString() const override
	{
		std::stringstream ss;
		ss << "WindowIconfyEvent: " << m_Minimized;
		return ss.str();
	}

	EVENT_CLASS_TYPE(WindowIconfy)
	EVENT_CLASS_CATEGORY(EventCategoryApplication)
};

class WindowCloseEvent : public Event
{
public:
	WindowCloseEvent() = default;

	EVENT_CLASS_TYPE(WindowClose)
	EVENT_CLASS_CATEGORY(EventCategoryApplication)
};

class AppTickEvent : public Event
{
public:
	AppTickEvent() = default;

	EVENT_CLASS_TYPE(AppTick)
	EVENT_CLASS_CATEGORY(EventCategoryApplication)
};

class AppUpdateEvent : public Event
{
public:
	AppUpdateEvent() = default;

	EVENT_CLASS_TYPE(AppUpdate)
	EVENT_CLASS_CATEGORY(EventCategoryApplication)
};

class AppRenderEvent : public Event
{
public:
	AppRenderEvent() = default;

	EVENT_CLASS_TYPE(AppRender)
	EVENT_CLASS_CATEGORY(EventCategoryApplication)
};
