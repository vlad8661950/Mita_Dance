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
    bool dragEnabled;
    SDL_bool AlwaysOnTop;
    int Width;
    int Height;
    int PosX;
    int PosY;
    int SavedPosX;
    int SavedPosY;
    bool Twice_size;
    bool Debug;

    static Settings defaultSettings() {
        return { true, SDL_TRUE, Standart_Width, Standart_Height, 0, 0, 0, 0, false, false};
    }
};

HWND getHWND(SDL_Window* window) {
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

void saveSettingsToINI(const string& filePath, const Settings &settings, SDL_Window* window) {
    ofstream file(filePath);
    HWND hwnd = getHWND(window);
    RECT rect;
    GetWindowRect(hwnd, &rect);
    if (!file.is_open()) {
        printf("Failed to open file for saving: ");
        return;
    }

    file << "[Window]\n";
    file << "Width=" << (!settings.Twice_size ? settings.Width : settings.Width / 2) << "\n";
    file << "Height=" << (!settings.Twice_size ? settings.Height : settings.Height / 2) << "\n";
    file << "AlwaysOnTop=" << (settings.AlwaysOnTop  == SDL_TRUE? "true" : "false") << "\n";
    file << "PosX=" << rect.left << "\n"; //settings.PosX
    file << "PosY=" << rect.top << "\n"; //settings.PosY
    file << "SavedPosX=" << settings.SavedPosX << "\n";
    file << "SavedPosY=" << settings.SavedPosY << "\n";

    file << "\n[Features]\n";
    file << "dragEnabled=" << (settings.dragEnabled ? "true" : "false") << "\n";
    file << "Twice_size=" << (settings.Twice_size ? "true" : "false") << "\n";
    file << "Debug=" << (settings.Debug ? "true" : "false") << "\n";

    file.close();
}

void loadSettingsFromINI(const string &filePath, Settings &settings) {
    ifstream file(filePath);
    settings = Settings::defaultSettings();
    if (!file.is_open()) {
        printf("Open setting file is failed: ");
        return;
    }
    
    string line, section;
    map<string, string> keyValues;

    while (getline(file, line)) {
        line.erase(0, line.find_first_not_of(" \t")); // Удаление пробелов в начале строки
        if (line.empty()) continue; // Пропускаем пустые строки

        if (line[0] == '[') section = line.substr(1, line.find(']') - 1);
        else {
            auto delimiterPos = line.find('=');
            if (delimiterPos == string::npos) continue;
            string key = line.substr(0, delimiterPos);
            string value = line.substr(delimiterPos + 1);

            // Удаление пробелов вокруг ключа и значения
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);

            keyValues[section + "." + key] = value;
        }
    }
    file.close();

    if (keyValues.count("Window.Width")) settings.Width = stoi(keyValues["Window.Width"]);
    if (keyValues.count("Window.Height")) settings.Height = stoi(keyValues["Window.Height"]);
    if (keyValues.count("Window.AlwaysOnTop")) settings.AlwaysOnTop = keyValues["Window.AlwaysOnTop"] == "true" ? SDL_TRUE : SDL_FALSE;
    if (keyValues.count("Window.PosX")) settings.PosX = stoi(keyValues["Window.PosX"]);
    if (keyValues.count("Window.PosY")) settings.PosY = stoi(keyValues["Window.PosY"]);
    if (keyValues.count("Window.SavedPosX")) settings.SavedPosX = stoi(keyValues["Window.SavedPosX"]);
    if (keyValues.count("Window.SavedPosY")) settings.SavedPosY = stoi(keyValues["Window.SavedPosY"]);
    if (keyValues.count("Features.dragEnabled")) settings.dragEnabled = keyValues["Features.dragEnabled"] == "true";
    if (keyValues.count("Features.Twice_size")) settings.Twice_size = keyValues["Features.Twice_size"] == "true";
    if (keyValues.count("Features.Debug")) settings.Debug = keyValues["Features.Debug"] == "true";
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
        SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_BLEND); // alpha канал 

        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);// Создание текстуры из изображения
        SDL_FreeSurface(surface);
        if (!texture) {
            SDL_Log("Texture creation Failed %s", SDL_GetError());
            continue;
        }
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);  // альфа-смешивание
        frames.push_back(texture);
    }
    return frames;
}

void makeWindowTransparent(SDL_Window * window) {
    HWND hwnd = getHWND(window);
    LONG style = GetWindowLong(hwnd, GWL_EXSTYLE);
    style |= WS_EX_LAYERED;
    SetWindowLong(hwnd, GWL_EXSTYLE, style);
    SetLayeredWindowAttributes(hwnd, RGB(0, 255, 0), 0, LWA_COLORKEY);
}

// Контекстное меню
int showContextMenu(HWND hwnd, int x, int y, bool drag, bool top, bool twice_size) {
    HMENU hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING, 1, L"Exit");
    AppendMenuW(hMenu, MF_STRING | (drag ? MF_CHECKED : MF_UNCHECKED), 2, L"Enable Drag");
    AppendMenuW(hMenu, MF_STRING | (top ? MF_CHECKED : MF_UNCHECKED), 3, L"Always on Top");
    AppendMenuW(hMenu, MF_STRING, 4, L"Save Pos");
    AppendMenuW(hMenu, MF_STRING, 5, L"Restore Pos");
    AppendMenuW(hMenu, MF_STRING | (twice_size ? MF_CHECKED : MF_UNCHECKED), 6, L"???");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, L"");
    AppendMenuW(hMenu, MF_STRING, 0, L"Created by DelaitMax");

    int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN, x, y, 0, hwnd, nullptr);
    DestroyMenu(hMenu);
    return cmd;
}



int main(int argc, char* argv[]) {
    const string settingsFile = GetSettingsFile();
    if (settingsFile.empty()) {
        printf("INI file creation failed");
        return -1;
    }
    Settings settings;
    loadSettingsFromINI(settingsFile, settings);
    int& Width = settings.Width;
    int& Height = settings.Height;
    SDL_bool& AlwaysOnTop = settings.AlwaysOnTop;
    int& PosX = settings.PosX;
    int& PosY = settings.PosY;
    int& SavedPosX = settings.SavedPosX;
    int& SavedPosY = settings.SavedPosY;
    bool& dragEnabled = settings.dragEnabled;
    bool& Twice_size = settings.Twice_size;
    if (settings.Debug) AllocConsole();
    if (Twice_size) {
        Width *= 2;
        Height *= 2;
    }

    vector<string> framePaths = {
    "miside00.png", "miside02.png", "miside05.png", "miside07.png", "miside08.png"
    }; //For VS
    vector<ImageData> imageData = {
        { miside00_png, miside00_png_len },
        { miside02_png, miside02_png_len },
        { miside05_png, miside05_png_len },
        { miside07_png, miside07_png_len },
        { miside08_png, miside08_png_len }
    }; //Headers png

    bool running = true, dragging = false;
    Uint32 lastFrameTime = SDL_GetTicks(), frameDelay = 35;// Коэфициент Задержки между кадрами (мс)
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
    makeWindowTransparent(window); // Устанавливаем прозрачность окна
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND); // Включаем альфа-смешивание для рендерера

    vector<SDL_Texture*> frames = loadFrames(renderer, framePaths, imageData);// Загружаем кадры
    if (frames.size() != framePaths.size()) {
        printf("Some texture are missing");
        return -1;
    }

    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            HWND hwnd = getHWND(window);
            RECT rect;
            GetWindowRect(hwnd, &rect);
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    dragging = true;
                    dragOffsetX = event.button.x;
                    dragOffsetY = event.button.y;
                }
                else if (event.button.button == SDL_BUTTON_RIGHT) {
                    int cmd = showContextMenu(hwnd,
                        rect.left + event.button.x, rect.top + event.button.y, dragEnabled, AlwaysOnTop, Twice_size);

                    switch (cmd) {
                    case 1:
                        running = false;
                        break;
                    case 2:
                        dragEnabled = !dragEnabled;
                        break;
                    case 3:
                        AlwaysOnTop = AlwaysOnTop == SDL_TRUE ? SDL_FALSE : SDL_TRUE;
                        SDL_SetWindowAlwaysOnTop(window, AlwaysOnTop);
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
                    case 6:
                        Twice_size = !Twice_size;
                        Width = Twice_size ? Width*2 : Width / 2;
                        Height = Twice_size ? Height*2 : Height / 2;
                        SDL_SetWindowSize(window, Width, Height);
                        break;
                    }
                    saveSettingsToINI(settingsFile, settings, window);
                }
            }
            else if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
                dragging = false;
            }
            else if (event.type == SDL_MOUSEMOTION && dragging && dragEnabled) {
                MoveWindow(hwnd, rect.left + (event.motion.x - dragOffsetX), rect.top + (event.motion.y - dragOffsetY), Width, Height, TRUE);
            }
        }

        // Переключение кадров
        if (SDL_GetTicks() - lastFrameTime > frameDelay * FrameToFrameDelay[frameIndex]) {
            frameIndex = (frameIndex + 1) % frames.size();
            lastFrameTime = SDL_GetTicks();
        }

        // Рендеринг
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
