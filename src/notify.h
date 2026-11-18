#pragma once

// Shared between the game thread (main.cpp: HUD hook, settings, log file) and the
// SKSE Menu Framework render thread (ui.cpp). No CommonLib types here.

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

namespace nn
{
	struct Entry
	{
		std::uint64_t id{ 0 };
		std::string   real;   // real time  HH:MM:SS
		std::string   game;   // in-game date and time
		std::string   place;  // location or cell of the player
		std::string   text;   // the notification
		std::uint32_t count{ 1 };  // same message several times in a row
	};

	inline std::atomic<bool> g_english{ true };       // language of the menu page
	inline std::atomic<bool> g_newestFirst{ true };   // list order on the page
	inline std::atomic<bool> g_writeLog{ false };     // also write NotifyNG-Notifications.log
	inline std::atomic<int>  g_maxEntries{ 1000 };    // kept in memory (100-5000)

	inline const char* L(const char* a_de, const char* a_en) { return g_english ? a_en : a_de; }

	std::vector<Entry> GetEntries();  // copy for the menu page
	std::uint64_t      TotalCount();  // all notifications since the game started
	void               Clear();
	void               SaveSettings();
	void               SetLogFile(bool a_on);  // open / close NotifyNG-Notifications.log

	void RegisterMenu();  // ui.cpp
}
