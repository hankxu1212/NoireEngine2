/*****************************************************************//**
 * \file   MouseEvent.h
 * \brief  Specifies the different events related to Mouse 
 *		   Drags/Clicks/Scrolls
 *
 * \author Hank Xu
 * \date   November 2023
 *********************************************************************/

#pragma once

#include "Event.hpp"
#include "core/MouseCodes.hpp"

class MouseMovedEvent : public Event
{
public:
	MouseMovedEvent(const float x, const float y)
		: m_MouseX(x), m_MouseY(y) {}

	float m_MouseX, m_MouseY;

	// TODO: move this to _DEBUG and avoid using stringstream for allocation!
	std::string ToString() const override
	{
		std::stringstream ss;
		ss << "MouseMovedEvent: " << m_MouseX << ", " << m_MouseY;
		return ss.str();
	}

	EVENT_CLASS_TYPE(MouseMoved)
	EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)
};

class MouseScrolledEvent : public Event
{
public:
	MouseScrolledEvent(const float xOffset, const float yOffset)
		: m_XOffset(xOffset), m_YOffset(yOffset) {}

	float m_XOffset, m_YOffset;

	// TODO: move this to _DEBUG and avoid using stringstream for allocation!
	std::string ToString() const override
	{
		std::stringstream ss;
		ss << "MouseScrolledEvent: " << m_XOffset << ", " << m_YOffset;
		return ss.str();
	}

	EVENT_CLASS_TYPE(MouseScrolled)
	EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)
};

class MouseButtonEvent : public Event
{
public:
	MouseButtonEvent(const MouseCode button) : m_Button(button) {}

	MouseCode m_Button;

	EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput | EventCategoryMouseButton)
};

class MouseButtonPressedEvent : public MouseButtonEvent
{
public:
	MouseButtonPressedEvent(const MouseCode button) : MouseButtonEvent(button) {}

	// TODO: move this to _DEBUG and avoid using stringstream for allocation!
	std::string ToString() const override
	{
		std::stringstream ss;
		ss << "MouseButtonPressedEvent: " << m_Button;
		return ss.str();
	}

	EVENT_CLASS_TYPE(MouseButtonPressed)
};

class MouseButtonReleasedEvent : public MouseButtonEvent
{
public:
	MouseButtonReleasedEvent(const MouseCode button) : MouseButtonEvent(button) {}

	// TODO: move this to _DEBUG and avoid using stringstream for allocation!
	std::string ToString() const override
	{
		std::stringstream ss;
		ss << "MouseButtonReleasedEvent: " << m_Button;
		return ss.str();
	}

	EVENT_CLASS_TYPE(MouseButtonReleased)
};

