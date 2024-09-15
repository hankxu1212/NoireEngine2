/*****************************************************************//**
 * \file   KeyEvent.h
 * \brief  Specifies the different events related to KeyPresses
 * 
 * \author Hank Xu
 * \date   November 2023
 *********************************************************************/

#pragma once

#include "Event.hpp"
#include "core/KeyCodes.hpp"
#include "core/Core.hpp"

class KeyEvent : public Event
{
public:
	KeyEvent(const KeyCode keycode) : m_KeyCode(keycode) {}

	KeyCode m_KeyCode;

	EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)
};

class KeyPressedEvent : public KeyEvent
{
public:
	KeyPressedEvent(const KeyCode keycode, bool isRepeat = false)
		: KeyEvent(keycode), m_IsRepeat(isRepeat) {}

	bool m_IsRepeat;

	// TODO: move this to _DEBUG and avoid using stringstream for allocation!
	std::string ToString() const override
	{
		std::stringstream ss;
		ss << "KeyPressedEvent: " << m_KeyCode << " (repeat = " << m_IsRepeat << ")";
		return ss.str();
	}

	EVENT_CLASS_TYPE(KeyPressed)
};

class KeyReleasedEvent : public KeyEvent
{
public:
	KeyReleasedEvent(const KeyCode keycode) : KeyEvent(keycode) {}

	// TODO: move this to _DEBUG and avoid using stringstream for allocation!
	std::string ToString() const override
	{
		std::stringstream ss;
		ss << "KeyReleasedEvent: " << m_KeyCode;
		return ss.str();
	}

	EVENT_CLASS_TYPE(KeyReleased)
};

class KeyTypedEvent : public KeyEvent
{
public:
	KeyTypedEvent(const KeyCode keycode) : KeyEvent(keycode) {}

	// TODO: move this to _DEBUG and avoid using stringstream for allocation!
	std::string ToString() const override
	{
		std::stringstream ss;
		ss << "KeyTypedEvent: " << m_KeyCode;
		return ss.str();
	}

	EVENT_CLASS_TYPE(KeyTyped)
};
