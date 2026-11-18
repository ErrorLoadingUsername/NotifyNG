// Page in the SKSE Menu Framework: list of all notifications, search, settings.

#include "PCH.h"

#define NOMINMAX
#include "SKSEMenuFramework.h"
#include "notify.h"

namespace
{
	using namespace ImGuiMCP;

	char g_filter[128]{};

	void Toggle(const char* a_label, std::atomic<bool>& a_flag, void (*a_after)(bool) = nullptr)
	{
		bool v = a_flag.load();
		if (Checkbox(a_label, &v)) {
			a_flag.store(v);
			if (a_after) {
				a_after(v);
			}
			nn::SaveSettings();
		}
	}

	std::string Lower(std::string a_text)
	{
		for (auto& c : a_text) {
			c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
		}
		return a_text;
	}

	void __stdcall RenderNotifications()
	{
		const auto entries = nn::GetEntries();
		const auto filter  = Lower(g_filter);

		Text(nn::L("%zu Meldungen in der Liste, %llu seit Spielstart", "%zu notifications in the list, %llu since the game started"),
			entries.size(), static_cast<unsigned long long>(nn::TotalCount()));
		InputTextWithHint("##nn_filter", nn::L("Suchen (Text oder Ort) ...", "Search (text or place) ..."), g_filter, sizeof(g_filter));
		SameLine();
		if (Button(nn::L("Liste leeren", "Clear list"))) {
			nn::Clear();
		}

		if (BeginTable("##nn_list", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
			TableSetupColumn(nn::L("Uhrzeit", "Time"), ImGuiTableColumnFlags_WidthFixed);
			TableSetupColumn(nn::L("Spielzeit", "Game time"), ImGuiTableColumnFlags_WidthFixed);
			TableSetupColumn(nn::L("Ort", "Place"), ImGuiTableColumnFlags_WidthFixed);
			TableSetupColumn(nn::L("Meldung", "Notification"), ImGuiTableColumnFlags_WidthStretch);
			TableHeadersRow();

			auto row = [&](const nn::Entry& a_e) {
				if (!filter.empty() && Lower(a_e.text).find(filter) == std::string::npos && Lower(a_e.place).find(filter) == std::string::npos) {
					return;
				}
				TableNextRow();
				TableSetColumnIndex(0);
				Text("%s", a_e.real.c_str());
				TableSetColumnIndex(1);
				Text("%s", a_e.game.c_str());
				TableSetColumnIndex(2);
				Text("%s", a_e.place.empty() ? "-" : a_e.place.c_str());
				TableSetColumnIndex(3);
				if (a_e.count > 1) {
					TextWrapped("%s  (%ux)", a_e.text.c_str(), a_e.count);
				} else {
					TextWrapped("%s", a_e.text.c_str());
				}
			};
			if (nn::g_newestFirst) {
				for (auto it = entries.rbegin(); it != entries.rend(); ++it) {
					row(*it);
				}
			} else {
				for (const auto& e : entries) {
					row(e);
				}
			}
			EndTable();
		}
		if (entries.empty()) {
			TextDisabled("%s", nn::L("Noch keine Meldungen.", "No notifications yet."));
		}
	}

	void __stdcall RenderSettings()
	{
		{
			int  lang    = nn::g_english ? 1 : 0;
			Text("%s", nn::L("Sprache:", "Language:"));
			SameLine();
			bool changed = RadioButton("Deutsch", &lang, 0);
			SameLine();
			changed |= RadioButton("English", &lang, 1);
			if (changed) {
				nn::g_english = lang == 1;
				nn::SaveSettings();
			}
			TextDisabled("%s", nn::L("Die Seitennamen links wechseln die Sprache erst nach einem Neustart.",
				"The page names on the left change language only after a restart."));
		}
		Toggle(nn::L("Neueste Meldung oben", "Newest notification at the top"), nn::g_newestFirst);

		int max = nn::g_maxEntries.load();
		if (SliderInt(nn::L("Meldungen in der Liste behalten", "Notifications kept in the list"), &max, 100, 5000, "%d")) {
			nn::g_maxEntries = max;
		}
		if (IsItemDeactivatedAfterEdit()) {
			nn::SaveSettings();
		}

		SeparatorText(nn::L("Datei", "File"));
		Toggle(nn::L("Meldungen zusaetzlich in eine Datei schreiben", "Also write notifications to a file"), nn::g_writeLog, nn::SetLogFile);
		TextDisabled("%s", nn::L("Dokumente\\My Games\\Skyrim Special Edition\\SKSE\\NotifyNG-Notifications.log - wird bei jedem Spielstart neu angelegt.",
			"Documents\\My Games\\Skyrim Special Edition\\SKSE\\NotifyNG-Notifications.log - started fresh at every game start."));
	}
}

namespace nn
{
	void RegisterMenu()
	{
		if (!GetMenuFrameworkModule()) {
			SKSE::log::info("SKSE Menu Framework not loaded - no menu page");
			return;
		}
		SKSEMenuFramework::SetSection("NotifyNG");
		SKSEMenuFramework::AddSectionItem(L("Meldungen", "Notifications"), RenderNotifications);
		SKSEMenuFramework::AddSectionItem(L("Einstellungen", "Settings"), RenderSettings);
		SKSE::log::info("menu page registered in SKSE Menu Framework");
	}
}
