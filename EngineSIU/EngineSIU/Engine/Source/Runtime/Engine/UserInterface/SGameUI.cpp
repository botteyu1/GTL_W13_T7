#include "SGameUI.h"
#include <algorithm>
#include "Engine/AssetManager.h"
#include "Engine/EditorEngine.h"

SGameUI& SGameUI::GetInstance()
{
    static SGameUI instance;
    return instance;
}

void SGameUI::Initialize()
{
    BackgroundTexture = FEngineLoop::ResourceManager.GetTexture(L"Contents/Texture/mariobackground.png");
    MarioTexture = FEngineLoop::ResourceManager.GetTexture(L"Contents/Texture/mario.png");
    StartTexture = FEngineLoop::ResourceManager.GetTexture(L"Contents/Texture/start.png");
    EndTexture = FEngineLoop::ResourceManager.GetTexture(L"Contents/Texture/end.png");

    MarioCurrentPos = ImVec2(-100, Height / 2);
    MarioTargetPos = ImVec2(Width * 0.2f, Height / 2);

    ButtonDefaultSize = ImVec2(300.f, 150.f);
    ButtonHoverSize = ImVec2(400.f, 200.f);
    StartButtonSize = ButtonDefaultSize;
    EndButtonSize = ButtonDefaultSize;
}

void SGameUI::Draw()
{
    // 창 크기 및 위치 계산
    const ImVec2 DisplaySize = ImVec2(Width + 20, Height + 20);
    
    // 창 위치와 크기를 고정
    ImGui::SetNextWindowPos(ImVec2(-10, -10), ImGuiCond_Always);
    ImGui::SetNextWindowSize(DisplaySize, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0);
    ImGui::Begin("Game Information", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse
        | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    DrawBackground();
    if (bMovedMario)
    {
        DrawButtons();
    }
    
    ImGui::End();
}

void SGameUI::OnResize(HWND hWnd)
{
    RECT ClientRect;
    GetClientRect(hWnd, &ClientRect);
    Width = ClientRect.right - ClientRect.left;
    Height = ClientRect.bottom - ClientRect.top;
}

void SGameUI::DrawBackground()
{
    ImVec2 DisplaySize = ImVec2(Width + 20, Height + 20);
    ImGui::Image((ImTextureID)(intptr_t)BackgroundTexture->TextureSRV, DisplaySize);

    if (MarioCurrentPos.x <= MarioTargetPos.x)
    {
        MarioCurrentPos.x += MarioPositionXSpeed * ImGui::GetIO().DeltaTime;
        MarioCurrentPos.x = std::min(MarioCurrentPos.x, MarioTargetPos.x);
        if (MarioCurrentPos.x == MarioTargetPos.x)
        {
            bMovedMario = true;
        }
    }

    ImGui::SetCursorPos(MarioCurrentPos);
    ImGui::Image((ImTextureID)(intptr_t)MarioTexture->TextureSRV, ImVec2(500, 500));
}

void SGameUI::DrawButtons()
{
    ImVec2 StartCenter = ImVec2(Width * 0.8f, Height * 0.5f);

    ImVec2 StartTopLeft = ImVec2(
        StartCenter.x - StartButtonSize.x * 0.5f,
        StartCenter.y - StartButtonSize.y * 0.5f
    );

    ImGui::SetCursorPos(StartTopLeft);
    ImGui::Image(
        (ImTextureID)(intptr_t)StartTexture->TextureSRV,
        StartButtonSize
    );

    bool bStartHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

    if (bStartHovered)
    {
        StartButtonSize = ButtonHoverSize;
    }
    else
    {
        StartButtonSize = ButtonDefaultSize;
    }

    if (bStartHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        UEditorEngine* Engine = Cast<UEditorEngine>(GEngine);
        if (Engine)
        {
            Engine->StartPIE();
            GEngineLoop.bPendingGame = false;
        }
    }
    
    ImVec2 EndCenter = ImVec2(
        StartCenter.x,
        StartCenter.y
        + StartButtonSize.y * 0.5f
        + 100.f
        + EndButtonSize.y * 0.5f
    );

    ImVec2 EndTopLeft = ImVec2(
        EndCenter.x - EndButtonSize.x * 0.5f,
        EndCenter.y - EndButtonSize.y * 0.5f
    );

    ImGui::SetCursorPos(EndTopLeft);
    ImGui::Image(
        (ImTextureID)(intptr_t)EndTexture->TextureSRV,
        EndButtonSize
    );

    bool bEndHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    if (bEndHovered)
    {
        EndButtonSize = ButtonHoverSize;
    }
    else
    {
        EndButtonSize = ButtonDefaultSize;
    }
    
    if (bEndHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        UEditorEngine* Engine = Cast<UEditorEngine>(GEngine);
        if (Engine)
        {
            Engine->HideLobby();
            MarioCurrentPos = ImVec2(-100, Height / 2);
        }
    }
}
