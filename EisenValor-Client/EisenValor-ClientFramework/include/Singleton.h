#pragma once

template <typename T>
class Singleton
{
protected:
	Singleton() {}
	virtual ~Singleton() {}

private:
	Singleton(const Singleton&) = delete;
	Singleton& operator=(const Singleton&) = delete;
	Singleton(Singleton&&) noexcept = delete;
	Singleton& operator=(Singleton&&) noexcept = delete;

public:
	static T& GetInstance()
	{
		static T instance{};
		return instance;
	}

	virtual void Initialize() {};
	virtual void Release() {};
};