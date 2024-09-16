#pragma once

class Singleton 
{
protected:
	Singleton() = default;

public:
	Singleton(const Singleton&) = delete;
	Singleton(Singleton&&) noexcept = default;
	Singleton& operator=(const Singleton&) = delete;
	Singleton& operator=(Singleton&&) noexcept = default;
};	
