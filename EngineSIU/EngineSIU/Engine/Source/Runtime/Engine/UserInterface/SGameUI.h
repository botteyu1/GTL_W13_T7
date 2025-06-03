#pragma once
#include "GameFramework/Actor.h"

class SGameUI
{
private:
    SGameUI() = default;
    virtual ~SGameUI() = default;

public:
    // 복사 방지
    SGameUI(const SGameUI&) = delete;
    SGameUI& operator=(const SGameUI&) = delete;
    SGameUI(SGameUI&&) = delete;
    SGameUI& operator=(SGameUI&&) = delete;

public:
    static SGameUI& GetInstance(); // 참조 반환으로 변경
    void Initialize();
    void Draw();
    void OnResize(HWND hWnd);

private:
    void DrawBackground();
    void DrawButtons();
    
private:
    UINT Width;
    UINT Height;

    std::shared_ptr<FTexture> BackgroundTexture;
    std::shared_ptr<FTexture> MarioTexture;
    std::shared_ptr<FTexture> StartTexture;
    std::shared_ptr<FTexture> EndTexture;

    // Mario
    float MarioPositionXSpeed = 300.0f;
    ImVec2 MarioCurrentPos;
    ImVec2 MarioTargetPos;
    bool bMovedMario = false;

    // Button
    ImVec2 StartButtonSize;
    ImVec2 EndButtonSize;
    ImVec2 ButtonHoverSize;
    ImVec2 ButtonDefaultSize;
};
