// NotifyNG - keeps every HUD notification (the messages in the corner of the screen)
// and shows them on a page in the SKSE Menu Framework; optionally also writes them to a file.

#include "PCH.h"
#include "notify.h"

namespace
{
	std::mutex              g_mutex;    // entries
	std::deque<nn::Entry>   g_entries;
	std::uint64_t           g_total  = 0;
	std::uint64_t           g_nextId = 1;

	std::filesystem::path   g_dir;
	std::filesystem::path   g_iniPath;
	std::mutex              g_fileMutex;  // notification log file
	std::ofstream           g_file;
	bool                    g_fileOpenedOnce = false;

	std::filesystem::path LogDir()
	{
		if (auto dir = SKSE::log::log_directory()) {
			return *dir;
		}
		std::error_code ec;
		return std::filesystem::current_path(ec) / "Data" / "SKSE" / "Plugins";
	}

	std::string RealTime()
	{
		const auto t = std::time(nullptr);
		std::tm    tm{};
		localtime_s(&tm, &t);
		char buf[16]{};
		std::strftime(buf, sizeof(buf), "%H:%M:%S", &tm);
		return buf;
	}

	std::string GameTime()
	{
		auto calendar = RE::Calendar::GetSingleton();
		if (!calendar) {
			return {};
		}
		char buf[128]{};
		calendar->GetTimeDateString(buf, sizeof(buf), true);
		return buf;
	}

	std::string Place()
	{
		auto player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return {};
		}
		if (auto location = player->GetCurrentLocation(); location && location->GetName() && *location->GetName()) {
			return location->GetName();
		}
		if (auto cell = player->GetParentCell(); cell && cell->GetName() && *cell->GetName()) {
			return cell->GetName();
		}
		return {};
	}

	// called on the game thread from the HUD menu hook
	void Record(const char* a_text)
	{
		if (!a_text || !*a_text) {
			return;
		}
		nn::Entry e;
		e.real  = RealTime();
		e.game  = GameTime();
		e.place = Place();
		e.text  = a_text;

		if (nn::g_writeLog) {
			std::scoped_lock l(g_fileMutex);
			if (g_file.is_open()) {
				g_file << '[' << e.real << "] [" << e.game << "] [" << e.place << "] " << e.text << '\n';
				g_file.flush();
			}
		}

		std::scoped_lock l(g_mutex);
		++g_total;
		if (!g_entries.empty() && g_entries.back().text == e.text && g_entries.back().place == e.place) {
			auto& last = g_entries.back();  // the same message again: count it instead of a new line
			last.count++;
			last.real = e.real;
			last.game = e.game;
			return;
		}
		e.id = g_nextId++;
		g_entries.push_back(std::move(e));
		const auto max = static_cast<std::size_t>(std::clamp(nn::g_maxEntries.load(), 100, 5000));
		while (g_entries.size() > max) {
			g_entries.pop_front();
		}
	}

	// HUDMenu::ProcessMessage (IMenu vfunc 0x04): every notification reaches the HUD as a
	// HUDData message of type kNotification - from Papyrus (Debug.Notification), the game
	// itself ("You cannot save right now.") and SKSE plugins alike.
	struct HUDMenuHook
	{
		static RE::UI_MESSAGE_RESULTS ProcessMessage(RE::HUDMenu* a_this, RE::UIMessage& a_message)
		{
			if (a_message.data) {
				if (auto data = skyrim_cast<RE::HUDData*>(a_message.data); data && data->type == RE::HUD_MESSAGE_TYPE::kNotification) {
					Record(data->text.c_str());
				}
			}
			return _original(a_this, a_message);
		}

		static void Install()
		{
			REL::Relocation<std::uintptr_t> vtbl{ RE::VTABLE_HUDMenu[0] };
			_original = vtbl.write_vfunc(0x4, ProcessMessage);
			SKSE::log::info("HUD menu hook installed");
		}

		static inline REL::Relocation<decltype(ProcessMessage)> _original;
	};

	void LoadSettings()
	{
		std::ifstream file(g_iniPath);
		std::string   line;
		while (std::getline(file, line)) {
			const auto eq = line.find('=');
			if (line.empty() || line[0] == ';' || line[0] == '[' || eq == std::string::npos) {
				continue;
			}
			const auto key   = line.substr(0, eq);
			const int  value = std::atoi(line.c_str() + eq + 1);
			if (key == "english") {
				nn::g_english = value != 0;
			} else if (key == "newestFirst") {
				nn::g_newestFirst = value != 0;
			} else if (key == "writeLogFile") {
				nn::g_writeLog = value != 0;
			} else if (key == "maxEntries") {
				nn::g_maxEntries = std::clamp(value, 100, 5000);
			}
		}
	}

	void SetupLog()
	{
		try {
			auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>((g_dir / "NotifyNG.log").string(), true);
			auto log  = std::make_shared<spdlog::logger>("global", std::move(sink));
			log->set_level(spdlog::level::info);
			log->flush_on(spdlog::level::info);
			spdlog::set_default_logger(std::move(log));
			spdlog::set_pattern("[%H:%M:%S.%e] %v");
		} catch (...) {
		}
	}
}

namespace nn
{
	std::vector<Entry> GetEntries()
	{
		std::scoped_lock l(g_mutex);
		return { g_entries.begin(), g_entries.end() };
	}

	std::uint64_t TotalCount()
	{
		std::scoped_lock l(g_mutex);
		return g_total;
	}

	void Clear()
	{
		std::scoped_lock l(g_mutex);
		g_entries.clear();
	}

	void SetLogFile(bool a_on)
	{
		std::scoped_lock l(g_fileMutex);
		if (!a_on) {
			if (g_file.is_open()) {
				g_file.close();
			}
			return;
		}
		if (g_file.is_open()) {
			return;
		}
		// new file at the first open of a game session, appended to when switched on again later
		g_file.open(g_dir / "NotifyNG-Notifications.log", g_fileOpenedOnce ? std::ios::app : std::ios::trunc);
		g_fileOpenedOnce = true;
	}

	void SaveSettings()
	{
		std::ofstream file(g_iniPath, std::ios::trunc);
		file << "; NotifyNG settings - 1 = on, 0 = off\n"
			 << "; Can also be changed in game: SKSE Menu Framework -> NotifyNG (written back here).\n"
			 << "\n[General]\n"
			 << "; language of the menu page: 0 = German, 1 = English\n"
			 << "english=" << int(g_english.load()) << "\n"
			 << "; newest notification at the top of the list\n"
			 << "newestFirst=" << int(g_newestFirst.load()) << "\n"
			 << "; also write every notification to NotifyNG-Notifications.log (next to this file)\n"
			 << "writeLogFile=" << int(g_writeLog.load()) << "\n"
			 << "; how many notifications the menu page keeps (100-5000)\n"
			 << "maxEntries=" << g_maxEntries.load() << "\n";
	}
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
	SKSE::Init(a_skse);
	g_dir     = LogDir();
	g_iniPath = g_dir / "NotifyNG.ini";
	SetupLog();
	SKSE::log::info("NotifyNG 1.0.0 - {}", REL::Module::get().version().string("."));
	LoadSettings();
	nn::SaveSettings();  // create / rewrite with all keys and comments (keeps the loaded values)
	if (nn::g_writeLog) {
		nn::SetLogFile(true);
	}

	HUDMenuHook::Install();

	SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message* a_msg) {
		if (a_msg->type == SKSE::MessagingInterface::kDataLoaded) {
			nn::RegisterMenu();
		}
	});
	return true;
}
