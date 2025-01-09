#define SDL_MAIN_HANDLED
#include <SDL_main.h>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_syswm.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <map>

#include "png/miside00.h"
#include "png/miside02.h"
#include "png/miside05.h"
#include "png/miside07.h"
#include "png/miside08.h"
using namespace std;

int Standart_Width = 384, Standart_Height = Standart_Width / 4 * 3; //768


struct ImageData {
    void* data;
    size_t length;
};

struct Settings {
    bool DragEnabled;
    SDL_bool AlwaysOnTop;
    int Width;
    int Height;
    int PosX;
    int PosY;
    int SavedPosX;
    int SavedPosY;
    int FrameDelay; 
    bool DoubleSize;
    bool MouseOff;
    bool Debug;

    static Settings defaultSettings() {
        return { true, SDL_TRUE, Standart_Width, Standart_Height, 0, 0, 0, 0, 40, false, false, false};
    }
};

HWND GetHWND(SDL_Window* window) {
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(window, &wmInfo);
    return wmInfo.info.win.window;
}

string GetSettingsFile() {
    char* appDataPath = nullptr;
    errno_t err = _dupenv_s(&appDataPath, 0, "APPDATA");
    if (err || !appDataPath) {
        printf("Failed to open APPDATA.");
        return "";
    }
    string filePath = string(appDataPath) + "\\Mita_Dance.ini";
    return filePath;
}

void SaveSettingsToINI(const string& filePath, const Settings &settings, SDL_Window* window) {
    ofstream file(filePath);
    HWND hwnd = GetHWND(window);
    RECT rect;
    GetWindowRect(hwnd, &rect);
    if (!file.is_open()) {
        printf("Failed to open file for saving: ");
        return;
    }

    file << "[Window]\n";
    file << "Width=" << (!settings.DoubleSize ? settings.Width : settings.Width / 2) << "\n";
    file << "Height=" << (!settings.DoubleSize ? settings.Height : settings.Height / 2) << "\n";
    file << "PosX=" << rect.left << "\n"; 
    file << "PosY=" << rect.top << "\n"; 
    file << "SavedPosX=" << settings.SavedPosX << "\n";
    file << "SavedPosY=" << settings.SavedPosY << "\n";
    file << "FrameDelay=" << settings.FrameDelay << "\n"; // Коэфициент Задержки между кадрами (мс)
    file << "AlwaysOnTop=" << (settings.AlwaysOnTop == SDL_TRUE ? "true" : "false") << "\n";

    file << "\n[Features]\n";
    file << "DragEnabled=" << (settings.DragEnabled ? "true" : "false") << "\n";
    file << "DoubleSize=" << (settings.DoubleSize ? "true" : "false") << "\n";
    file << "MouseOff=" << (settings.MouseOff ? "true" : "false") << "\n";
    file << "Debug=" << (settings.Debug ? "true" : "false") << "\n";

    file.close();
}

void LoadSettingsFromINI(const string &filePath, Settings &settings) {
    ifstream file(filePath);
    settings = Settings::defaultSettings();
    if (!file.is_open()) {
        printf("Open setting file is failed: ");
        return;
    }
    
    string line;
    map<string, string> keyValues;

    while (getline(file, line)) {
        line.erase(0, line.find_first_not_of(" \t")); // Удаление пробелов в начале строки
        if (line.empty()) continue; // Пропускаем пустые строки

        auto delimiterPos = line.find('=');
        if (delimiterPos == string::npos) continue;
        string key = line.substr(0, delimiterPos);
        string value = line.substr(delimiterPos + 1);

        // Удаление пробелов вокруг ключа и значения
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);

        keyValues[key] = value;
    }
    file.close();

    if (keyValues.count("Width")) settings.Width = stoi(keyValues["Width"]);
    if (keyValues.count("Height")) settings.Height = stoi(keyValues["Height"]);
    if (keyValues.count("PosX")) settings.PosX = stoi(keyValues["PosX"]);
    if (keyValues.count("PosY")) settings.PosY = stoi(keyValues["PosY"]);
    if (keyValues.count("SavedPosX")) settings.SavedPosX = stoi(keyValues["SavedPosX"]);
    if (keyValues.count("SavedPosY")) settings.SavedPosY = stoi(keyValues["SavedPosY"]);
    if (keyValues.count("FrameDelay")) settings.FrameDelay = stoi(keyValues["FrameDelay"]);
    if (keyValues.count("AlwaysOnTop")) settings.AlwaysOnTop = keyValues["AlwaysOnTop"] == "true" ? SDL_TRUE : SDL_FALSE;
    if (keyValues.count("DragEnabled")) settings.DragEnabled = keyValues["DragEnabled"] == "true";
    if (keyValues.count("DoubleSize")) settings.DoubleSize = keyValues["DoubleSize"] == "true";
    if (keyValues.count("MouseOff")) settings.MouseOff = keyValues["MouseOff"] == "true";
    if (keyValues.count("Debug")) settings.Debug = keyValues["Debug"] == "true";
}


vector<SDL_Texture*> loadFrames(SDL_Renderer* renderer, const vector<string>& filePaths, const vector<ImageData>& imageData){
    vector<SDL_Texture*> frames;
    int i = 0;
    for (const auto& path : filePaths) {
        SDL_Surface* surface = IMG_Load(path.c_str());
        if (!surface) {
            SDL_RWops* rw = nullptr;
            if (i >= 0 && i < imageData.size()) {
                rw = SDL_RWFromMem(imageData[i].data, imageData[i].length);
                i++;
            }
            surface = IMG_LoadPNG_RW(rw);
            if (!surface) {
                SDL_Log("Surface Load Failed %s", IMG_GetError());
                continue;
            }
            SDL_RWclose(rw);

        }
        SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_BLEND); // alpha mode png

        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
        if (!texture) {
            SDL_Log("Texture creation Failed %s", SDL_GetError());
            continue;
        }
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);  
        frames.push_back(texture);
    }
    return frames;
}

void MakeWindowTransparent(SDL_Window * window) {
    HWND hwnd = GetHWND(window);
    LONG style = GetWindowLong(hwnd, GWL_EXSTYLE);
    style |= WS_EX_LAYERED;
    SetWindowLong(hwnd, GWL_EXSTYLE, style);
    SetLayeredWindowAttributes(hwnd, RGB(0, 255, 0), 0, LWA_COLORKEY);
}

// Контекстное меню
int ShowContextMenu(HWND hwnd, int x, int y, const Settings settings) {
    HMENU hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING, 1, L"Exit");
    AppendMenuW(hMenu, MF_STRING | (settings.DragEnabled ? MF_CHECKED : MF_UNCHECKED), 2, L"Enable Drag");
    AppendMenuW(hMenu, MF_STRING | (settings.AlwaysOnTop ? MF_CHECKED : MF_UNCHECKED), 3, L"Always on Top");
    AppendMenuW(hMenu, MF_STRING, 4, L"Save Pos");
    AppendMenuW(hMenu, MF_STRING, 5, L"Restore Pos");
    AppendMenuW(hMenu, MF_STRING | (settings.MouseOff ? MF_CHECKED : MF_UNCHECKED), 7, L"MouseOff");
    AppendMenuW(hMenu, MF_STRING | (settings.DoubleSize ? MF_CHECKED : MF_UNCHECKED), 6, L"???");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, L"");
    AppendMenuW(hMenu, MF_STRING, 0, L"Created by DelaitMax");

    int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN, x, y, 0, hwnd, nullptr);
    DestroyMenu(hMenu);
    return cmd;
}

void AntiClickMode(HWND hwnd, bool mouse) {
    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    if (mouse) exStyle |= WS_EX_TRANSPARENT;
    else exStyle &= ~WS_EX_TRANSPARENT;
    SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);
}

void AlwaysOnTopMode(SDL_Window* window, SDL_bool& AlwaysOnTop) {
    HWND hwnd = GetHWND(window);
    if (AlwaysOnTop == SDL_TRUE) {
        SDL_SetWindowAlwaysOnTop(window, AlwaysOnTop);
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }
    else {
        SDL_SetWindowAlwaysOnTop(window, AlwaysOnTop);
        SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
}//плохо работаетб переделать.


int main(int argc, char* argv[]) {
    const string settingsFile = GetSettingsFile();
    if (settingsFile.empty()) {
        printf("INI file creation failed");
        return -1;
    }
    Settings settings;
    LoadSettingsFromINI(settingsFile, settings);
    int& Width = settings.Width;
    int& Height = settings.Height;
    SDL_bool& AlwaysOnTop = settings.AlwaysOnTop;
    int& PosX = settings.PosX;
    int& PosY = settings.PosY;
    int& SavedPosX = settings.SavedPosX;
    int& SavedPosY = settings.SavedPosY;
    int& FrameDelay = settings.FrameDelay;
    bool& DragEnabled = settings.DragEnabled;
    bool& DoubleSize = settings.DoubleSize;
    bool& MouseOff = settings.MouseOff;
    if (settings.Debug) AllocConsole();
    if (DoubleSize) {
        Width *= 2;
        Height *= 2;
    }

    vector<string> framePaths = {
    "miside00.png", "miside02.png", "miside05.png", "miside07.png", "miside08.png"
    }; //For debug
    vector<ImageData> imageData = {
        { miside00_png, miside00_png_len },
        { miside02_png, miside02_png_len },
        { miside05_png, miside05_png_len },
        { miside07_png, miside07_png_len },
        { miside08_png, miside08_png_len }
    }; //Headers png

    bool running = true, dragging = false;
    Uint32 lastFrameTime = SDL_GetTicks();
    int FrameToFrameDelay[] = { 2,3,2,1,3 };
    int dragOffsetX = 0, dragOffsetY = 0;
    size_t frameIndex = 0;


    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("SDL initialization failed %s", SDL_GetError());
        return -1;
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        SDL_Log("SDL_image initialization failed %s", IMG_GetError());
        SDL_Quit();
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Mita", PosX, PosY, Width, Height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_ALWAYS_ON_TOP | SDL_WINDOW_BORDERLESS | SDL_WINDOW_SKIP_TASKBAR
    );
    MakeWindowTransparent(window);
    AlwaysOnTopMode(window, AlwaysOnTop);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND); 

    vector<SDL_Texture*> frames = loadFrames(renderer, framePaths, imageData);// Загружаем кадры
    if (frames.size() != framePaths.size()) {
        printf("Some texture are missing");
        return -1;
    }

    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            HWND hwnd = GetHWND(window);
            RECT rect;
            GetWindowRect(hwnd, &rect);
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    if (MouseOff) {
                        AntiClickMode(hwnd, true);
                    }
                    else {
                        dragging = true;
                        dragOffsetX = event.button.x;
                        dragOffsetY = event.button.y;
                    }
                }
                else if (event.button.button == SDL_BUTTON_RIGHT) {
                    int cmd = ShowContextMenu(hwnd,
                        rect.left + event.button.x, rect.top + event.button.y, settings);

                    switch (cmd) {
                    case 1:
                        running = false;
                        break;
                    case 2:
                        DragEnabled = !DragEnabled;
                        break;
                    case 3:
                        AlwaysOnTop = AlwaysOnTop == SDL_TRUE ? SDL_FALSE : SDL_TRUE;
                        AlwaysOnTopMode(window, AlwaysOnTop);
                        break;
                    case 4:
                        SavedPosX = rect.left;
                        SavedPosY = rect.top;
                        break;
                    case 5:
                        PosX = SavedPosX;
                        PosY = SavedPosY;
                        MoveWindow(hwnd, PosX, PosY, Width, Height, TRUE);
                        break;
                    case 7:
                        MouseOff = !MouseOff;
                        break;
                    case 6:
                        DoubleSize = !DoubleSize;
                        Width = DoubleSize ? Width*2 : Width / 2;
                        Height = DoubleSize ? Height*2 : Height / 2;
                        SDL_SetWindowSize(window, Width, Height);
                        break;
                    }
                    SaveSettingsToINI(settingsFile, settings, window);
                }
            }
            else if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
                dragging = false;
            }
            else if (event.type == SDL_MOUSEMOTION && dragging && DragEnabled) {
                MoveWindow(hwnd, rect.left + (event.motion.x - dragOffsetX), rect.top + (event.motion.y - dragOffsetY), Width, Height, TRUE);
            }
            else if ((event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) && (event.key.keysym.sym == SDLK_LALT || event.key.keysym.sym == SDLK_RALT)) {
                AntiClickMode(hwnd, false);
            } // Off MouseOff mode. Alt+Tab to Mita, Re-press Alt, RMB on Mita.
            //else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {

            //    SetWindowPos(hwnd, HWND_TOPMOST, rect.left + (event.motion.x - dragOffsetX), rect.top + (event.motion.y - dragOffsetY), 0, 0, SWP_NOSIZE);
            //    cout << rect.left << rect.top << "xa\n";
            //    EnumWindows()
            //}
        }

        if (SDL_GetTicks() - lastFrameTime > FrameDelay * FrameToFrameDelay[frameIndex]) {
            frameIndex = (frameIndex + 1) % frames.size();
            lastFrameTime = SDL_GetTicks();
        }


        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        SDL_RenderSetLogicalSize(renderer, Width, Height);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, frames[frameIndex], nullptr, nullptr);
        SDL_RenderPresent(renderer);
    }

    // Освобождение ресурсов
    for (auto* texture : frames) SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
