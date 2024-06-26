/**************************************************************************
 * 3to4++ - https://github.com/rayzchen/3to4++
 *-------------------------------------------------------------------------
 * Copyright 2024 Ray Chen
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 **************************************************************************/

#include <sstream>
#include <linmath.h>
#include <glad/gl.h>
#include <cstdlib>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <nfd.h>
#include "gui.h"
#include "font.h"
#include "control.h"
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

// todo: replace with keybinds
std::vector<std::string> GuiRenderer::helpText = {
	"Controls:",
	"Left Click + Drag - Move Camera",
	"Scroll Wheel - Zoom In / Out",
	"Middle Click + Drag - Explode Pieces",
	"W, E, R, S, D, F, C, V - Select Cell",
	"(L, U, B, F, I, R, D, O)",
	"I, K - Rotate x, x'",
	"J, L - Rotate y, y'",
	"O, U - Rotate z, z'",
	"SPACE - Gyro (Selected > I)",
	"M, COMMA, PERIOD - Move Outer Parts",
};
std::vector<std::array<std::string, 3>> GuiRenderer::creditsText = {
	{"Join the Hypercubers ", "Discord!", "https://discord.gg/BuKJksy37P"},
	{"Simulator made by ", "Rayzchen (GitHub)", "https://github.com/rayzchen"},
	{"App inspired by ", "Akkei (Instagram)", "https://www.instagram.com/ake_cubes"},
	{"Puzzle designed by ", "Grant S (YouTube)", "https://www.youtube.com/channel/UCamz5yyKs4naf290b9uCo6Q"}
};

GuiRenderer::GuiRenderer(GLFWwindow *window, PuzzleController *controller, int width, int height) {
	this->controller = controller;
	this->history = controller->history;
	this->width = width;
	this->height = height;
	showHelp = false;
	modalToggle = false;

	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init();
	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = NULL;

	float xscale, yscale;
	glfwGetWindowContentScale(window, &xscale, &yscale);
	hudFont = io.Fonts->AddFontFromMemoryCompressedTTF(
		notosans_compressed_data,
		notosans_compressed_size,
		20 * xscale
	);
	uiFont = io.Fonts->AddFontFromMemoryCompressedTTF(
		notosans_compressed_data,
		notosans_compressed_size,
		16 * xscale
	);

#ifndef NO_DEMO_WINDOW
	showDemoWindow = false;
#endif
}

GuiRenderer::~GuiRenderer() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

bool GuiRenderer::captureMouse() {
	ImGuiIO& io = ImGui::GetIO();
	return io.WantCaptureMouse;
}

int GuiRenderer::getTextWidth(std::string text) {
	return ImGui::CalcTextSize(text.c_str()).x;
}

void GuiRenderer::renderText(std::string text, float x, float y, int color) {
	ImGui::GetForegroundDrawList()->AddText({x, y}, color, text.c_str());
}

void GuiRenderer::renderLink(std::string text, std::string link, float x, float y, int color, int index) {
	std::string title = "link" + std::to_string(index);
	std::string id = "##" + title;
	renderText(text, x, y, color);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
	ImGui::Begin(title.c_str(), NULL, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration);
	ImGui::SetWindowPos({x, y});
	if (ImGui::InvisibleButton(id.c_str(), ImGui::CalcTextSize(text.c_str()))) {
#ifdef __EMSCRIPTEN__
		EM_ASM(window.open(UTF8ToString($0)), link.c_str());
#else
#ifdef _WIN32
		std::string command = "start " + link;
#else
		std::string command = "xdg-open " + link;
#endif
		system(command.c_str());
#endif
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
#ifdef __EMSCRIPTEN__
		EM_ASM(document.documentElement.style.cursor = UTF8ToString($0), "pointer");
#endif
	}
	ImGui::End();
	ImGui::PopStyleVar(2);
}

void GuiRenderer::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
	this->width = width;
	this->height = height;
}

void GuiRenderer::renderGui() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::PushFont(uiFont);
	displayMenuBar();
	displayStatusBar();
	displayModal();
#ifndef NO_DEMO_WINDOW
	if (showDemoWindow) {
		ImGui::ShowDemoWindow();
	}
#endif
	ImGui::PopFont();

	ImGui::PushFont(hudFont);
	displayHUD();
	ImGui::PopFont();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void GuiRenderer::displayMenuBar() {
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Open", "Ctrl+O")) checkUnsaved("open another file");
			if (ImGui::MenuItem("Save", "Ctrl+S", false, false)) {}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Edit")) {
			if (ImGui::MenuItem("Undo", "Z", false, history->canUndo())) controller->undoMove();
            if (ImGui::MenuItem("Redo", "Y", false, history->canRedo())) controller->redoMove();
            ImGui::Separator();
			if (ImGui::MenuItem("Reset", "Ctrl+R")) checkUnsaved("reset puzzle");
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Scramble")) {
			for (int i = 1; i < 9; i++) {
				if (ImGui::MenuItem(std::to_string(i).c_str(), NULL)) checkUnsaved("scramble", i);
			}
            ImGui::Separator();
			if (ImGui::MenuItem("Full", "Ctrl+F")) checkUnsaved("scramble", 0);
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Tools")) {
#ifndef NO_DEMO_WINDOW
			if (ImGui::MenuItem("Show demo window", NULL, &showDemoWindow)) {}
#endif
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Help")) {
			if (ImGui::MenuItem("Show help", "H")) toggleHelp();
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
}

void GuiRenderer::displayModal() {
    if (modalResolve) {
    	resolveModal();
    	modalResolve = false;
    }
	if (modalToggle) {
		ImGui::SetNextWindowSize({100, 0});
		ImGui::OpenPopup("Unsaved changes");
		modalToggle = false;
	}
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Unsaved changes", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped("Discard puzzle state and %s?", modalText.c_str());
        if (ImGui::Button("Yes", ImVec2(80, 0))) {
        	ImGui::CloseCurrentPopup();
        	modalResolve = true;
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button("No", ImVec2(80, 0))) {
        	ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void GuiRenderer::displayHUD() {
#ifdef __EMSCRIPTEN__
	EM_ASM(document.documentElement.style.removeProperty(UTF8ToString($0)), "cursor");
#endif
	const int white = IM_COL32_WHITE;
	const int red = IM_COL32(255, 127, 127, 255);
	const int blue = IM_COL32(127, 127, 255, 255);

	int lineHeight = ImGui::CalcTextSize("").y;
	int textWidth;
	if (showHelp) {
		textWidth = getTextWidth(helpText[10]);
		for (size_t i = 0; i < helpText.size(); i++) {
			float x = width - 5 - textWidth;
			float y = (i + 1) * lineHeight + ImGui::GetFrameHeight();
			if (i == 0) x = width - 5 - (textWidth + getTextWidth(helpText[i])) / 2;
			renderText(helpText[i], x, y, white);
		}

		for (size_t i = 0; i < creditsText.size(); i++) {
			float x = 5;
			float y = height - 5 - (creditsText.size() - i) * lineHeight - ImGui::GetFrameHeight();
			textWidth = getTextWidth(creditsText[i][0]);
			renderText(creditsText[i][0], x, y, white);
			renderLink(creditsText[i][1], creditsText[i][2], x + textWidth, y, blue, i);
		}
		float y = height - 5 - (creditsText.size() + 1) * lineHeight - ImGui::GetFrameHeight();
		renderText("links now work lol", 5, y, red);
	}

	std::string saveWarning = "No saving in this version!";
	textWidth = getTextWidth(saveWarning);
	renderText(saveWarning, width - 5 - textWidth, ImGui::GetFrameHeight(), red);

	std::string helpHint = "Help: H";
	textWidth = getTextWidth(helpHint);
	renderText(helpHint, width - 5 - textWidth, height - 5 - lineHeight - ImGui::GetFrameHeight(), white);
}

void GuiRenderer::displayStatusBar() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;
    float height = ImGui::GetFrameHeight();
    if (ImGui::BeginViewportSideBar("##StatusBar", viewport, ImGuiDir_Down, height, window_flags)) {
	    if (ImGui::BeginMenuBar()) {
	        ImGui::Text("%s", controller->getStatus().c_str());

			std::ostringstream stream;
			stream << "Move Count: " << history->getTurnCount();
			std::string text = stream.str();

	        ImGui::SameLine(
	        	ImGui::GetWindowContentRegionMax().x - ImGui::CalcTextSize(text.c_str()).x - 5
	        );
	        ImGui::Text("%s", text.c_str());
	        ImGui::EndMenuBar();
	    }
	    ImGui::End();
	}
}

void GuiRenderer::toggleHelp() {
	showHelp = !showHelp;
}

void GuiRenderer::keyCallback(GLFWwindow* window, int key, int action, int mods) {
	if (action == GLFW_PRESS) {
		if (mods == 0) {
			if (key == GLFW_KEY_H) {
				toggleHelp();
			}
		} else if (mods & GLFW_MOD_CONTROL) {
			if (key == GLFW_KEY_F) {
				checkUnsaved("scramble", 0);
			} else if (key == GLFW_KEY_R) {
				checkUnsaved("reset puzzle");
			} else if (key == GLFW_KEY_O) {
#ifndef __EMSCRIPTEN__
				checkUnsaved("open another file");
#endif
			}
		}
	}
}

void GuiRenderer::checkUnsaved(std::string action, int argument) {
	modalArg = argument;
	checkUnsaved(action);
}

void GuiRenderer::checkUnsaved(std::string action) {
	modalText = action;
	modalToggle = true;
}

void GuiRenderer::resolveModal() {
	if (modalText == "reset puzzle") {
		controller->resetPuzzle();
	} else if (modalText == "scramble") {
		controller->resetPuzzle();
		controller->scramblePuzzle(modalArg);
	} else if (modalText == "open another file") {
#ifndef __EMSCRIPTEN__
		nfdchar_t *outPath = NULL;
		nfdresult_t result = NFD_OpenDialog(NULL, NULL, &outPath);
		if (result == NFD_OKAY) {
			std::string file(outPath);
			controller->openFile(file);
		}
#endif
	} else if (modalText == "exit") {
		// todo
	}
}
