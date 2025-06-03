#pragma once
#include "GameFramework/Actor.h"
#include "Delegates/Delegate.h"

class SGameUI
{
public:
    SGameUI();
    virtual ~SGameUI() = default;
    void Initialize();
    void Draw();
    void OnResize(HWND hWnd);

private:
    void DrawBackground();
    void DrawButtons();

    void DrawPause();
    
public:
    TArray<FDelegateHandle> OnHandle;
    
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
