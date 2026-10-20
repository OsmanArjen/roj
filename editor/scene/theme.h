// Everything that decides how the scene editor looks: palette, child flags, and the window style.
// Header-only on purpose: it is tiny, and every part of the editor may need a colour.
#pragma once
#include <imgui.h>

namespace roj::scene_ui {

	inline const ImVec4 kAccent(0.45f, 0.70f, 1.00f, 1.00f);
	inline const ImVec4 kWarn(1.00f, 0.45f, 0.30f, 1.00f);

	// ImGui older than 1.91.1 calls this ImGuiChildFlags_Border.
	inline constexpr ImGuiChildFlags kPanelBorder = ImGuiChildFlags_Borders;

	// Pushes style for this window only, so the editor's look never leaks into the rest of your UI.
	// The scope counts its own pushes, so pops can never go out of balance.
	class StyleScope {
	public:
		StyleScope() {
			var(ImGuiStyleVar_WindowPadding, ImVec2(6.0f, 6.0f));
			var(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 3.0f));
			var(ImGuiStyleVar_ItemSpacing, ImVec2(6.0f, 5.0f));
			var(ImGuiStyleVar_IndentSpacing, 14.0f);
			var(ImGuiStyleVar_FrameRounding, 4.0f);
			var(ImGuiStyleVar_ChildRounding, 4.0f);
			var(ImGuiStyleVar_GrabRounding, 4.0f);
			var(ImGuiStyleVar_ScrollbarRounding, 6.0f);

			color(ImGuiCol_ChildBg, ImVec4(0.085f, 0.090f, 0.105f, 1.0f));
			color(ImGuiCol_FrameBg, ImVec4(0.160f, 0.170f, 0.200f, 1.0f));
			color(ImGuiCol_FrameBgHovered, ImVec4(0.210f, 0.230f, 0.270f, 1.0f));
			color(ImGuiCol_Header, ImVec4(0.220f, 0.380f, 0.600f, 0.55f));
			color(ImGuiCol_HeaderHovered, ImVec4(0.280f, 0.470f, 0.720f, 0.70f));
			color(ImGuiCol_HeaderActive, ImVec4(0.320f, 0.520f, 0.800f, 0.85f));
			color(ImGuiCol_Border, ImVec4(0.240f, 0.260f, 0.310f, 1.0f));
		}
		~StyleScope() {
			ImGui::PopStyleColor(colors_);
			ImGui::PopStyleVar(vars_);
		}
		StyleScope(const StyleScope&) = delete;
		StyleScope& operator=(const StyleScope&) = delete;

	private:
		void var(ImGuiStyleVar v, ImVec2 x) { ImGui::PushStyleVar(v, x); ++vars_; }
		void var(ImGuiStyleVar v, float x) { ImGui::PushStyleVar(v, x); ++vars_; }
		void color(ImGuiCol c, ImVec4 x) { ImGui::PushStyleColor(c, x); ++colors_; }
		int vars_ = 0;
		int colors_ = 0;
	};

}
