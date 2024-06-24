// UWP STORAGE MANAGER
// Copyright (c) 2023 Bashar Astifan.
// Email: bashar@astifan.online
// Telegram: @basharastifan
// GitHub: https://github.com/basharast/UWP2Win32

// WaitTask functions originally by Team Kodi

#pragma once

#include "pch.h"
#include <ppl.h>
#include <ppltasks.h>
#include <wrl.h>
#include <wrl/implements.h>

#include "StorageLog.h"
#include "StorageExtensions.h"

using namespace winrt::Windows::UI::Core;

// Don't add 'using' 'Windows::Foundation'
// it might cause confilct with some types like 'Point'

#pragma region Async Handlers


namespace winrt
{
	using namespace Windows::Foundation;
}

inline void WaitTask(const winrt::IAsyncAction& asyncOp)
{
	if (asyncOp.Status() == winrt::AsyncStatus::Completed)
		return;

	if (!winrt::impl::is_sta_thread())
		return asyncOp.get();

	auto __sync = std::make_shared<Concurrency::event>();
	asyncOp.Completed([&](auto&&, auto&&) {
		__sync->set();
	});
	__sync->wait();
}

template <typename TResult, typename TProgress> inline
TResult WaitTask(const winrt::IAsyncOperationWithProgress<TResult, TProgress>& asyncOp)
{
	if (asyncOp.Status() == winrt::AsyncStatus::Completed)
		return asyncOp.GetResults();

	if (!winrt::impl::is_sta_thread())
		return asyncOp.get();

	auto __sync = std::make_shared<Concurrency::event>();
	asyncOp.Completed([&](auto&&, auto&&) {
		__sync->set();
	});
	__sync->wait();

	return asyncOp.GetResults();
}

template <typename TResult> inline
TResult WaitTask(const winrt::IAsyncOperation<TResult>& asyncOp)
{
	if (asyncOp.Status() == winrt::AsyncStatus::Completed)
		return asyncOp.GetResults();

	if (!winrt::impl::is_sta_thread())
		return asyncOp.get();

	auto __sync = std::make_shared<Concurrency::event>();
	asyncOp.Completed([&](auto&&, auto&&)
		{
			__sync->set();
		});
	__sync->wait();

	return asyncOp.GetResults();
}

template <typename TResult> inline
TResult WaitTask(const Concurrency::task<TResult>& asyncOp)
{
	if (asyncOp.is_done())
		return asyncOp.get();

	if (!winrt::impl::is_sta_thread()) // blocking suspend is allowed
		return asyncOp.get();

	auto _sync = std::make_shared<Concurrency::event>();
	asyncOp.then([&](TResult result)
		{
			_sync->set();
		});
	_sync->wait();

	return asyncOp.get();
}

template<typename T>
T TaskPass(winrt::Windows::Foundation::IAsyncOperation<T> task, T def)
{
	try {
		return WaitTask(task);
	}
	catch (...) {
		return def;
	}
}

template<typename T>
T TaskPass(winrt::Windows::Foundation::IAsyncOperation<T> task)
{
	return WaitTask(task);
}


bool ActionPass(winrt::Windows::Foundation::IAsyncAction action);

#pragma endregion

// Now it's more simple to execute async task
// @out: output variable
// @task: async task
template<typename T>
void ExecuteTask(T& out, winrt::Windows::Foundation::IAsyncOperation<T> task)
{
	out = TaskPass<T>(task);
};

// For specific return default value
// @out: output variable
// @task: async task
// @def: default value when fail
template<typename T>
void ExecuteTask(T& out, winrt::Windows::Foundation::IAsyncOperation<T> task, T def)
{
	out = TaskPass<T>(task, def);
};


// Async action such as 'Delete' file
// @action: async action
// return false when action failed
bool ExecuteTask(winrt::Windows::Foundation::IAsyncAction action);
