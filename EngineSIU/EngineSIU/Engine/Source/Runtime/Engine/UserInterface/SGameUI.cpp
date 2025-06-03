#include "SGameUI.h"
#include <algorithm>

#include "SoundManager.h"
#include "Engine/AssetManager.h"
#include "Engine/EditorEngine.h"
#include "GameFramework/GameMode.h"


SGameUI::SGameUI()
    : Width()
    , Height(0)
{
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

    if (GEngineLoop.bPauseGame)
    {
        DrawPause();
    }
    else
    {
        DrawBackground();
        if (bMovedMario)
        {
            DrawButtons();
        }
    }
    
    ImGui::End();
}

void SGameUI::OnResize(HWND hWnd)
{
    if (hWnd)
    {
        RECT ClientRect;
        GetClientRect(hWnd, &ClientRect);
        Width = ClientRect.right - ClientRect.left;
        Height = ClientRect.bottom - ClientRect.top;
    }
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
            FSoundManager::GetInstance().PlaySound("Select");
            Engine->StartPIE();
            GEngineLoop.bPendingGame = false;
            GEngineLoop.GetGameMode()->StartMatch();
            FSoundManager::GetInstance().StopSound("Intro");
            FSoundManager::GetInstance().PlaySound("Play");
            OnHandle.Add(GEngineLoop.GetGameMode()->OnGamePause.AddLambda([](bool bPaused)
            {
                if (bPaused)
                {
                    GEngineLoop.bPendingGame = true;
                    GEngineLoop.bPauseGame = true;
                }
                else
                {
                    GEngineLoop.bPendingGame = false;
                    GEngineLoop.bPauseGame = false;
                }
            }));
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

void SGameUI::DrawPause()
{
    // 1) 전체 화면 크기 기준으로 윈도우를 풀스크린으로 설정
    ImGui::SetNextWindowPos(ImVec2(-10.0f, -10.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(Width + 20.0f, Height + 20.0f), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.8f);
    ImGui::Begin("Game Pause", nullptr,
        ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoCollapse
        | ImGuiWindowFlags_NoTitleBar
        | ImGuiWindowFlags_NoScrollbar
        | ImGuiWindowFlags_NoScrollWithMouse);

    // 2) 윈도우 내부 크기 가져오기
    ImVec2 WindowSize = ImGui::GetWindowSize();

    // 3) 버튼 설정: 크기, 간격 등
    const ImVec2 ButtonSize = ImVec2(250.0f, 100.0f);
    const float VerticalSpacing = 20.0f; // 버튼 사이 세로 간격

    // 4) 총 버튼 영역 높이 계산 (버튼 3개 + 간격 2개)
    float TotalButtonsHeight = ButtonSize.y * 3.0f + VerticalSpacing * 2.0f;

    // 5) 첫 번째 버튼의 Y 시작 위치 = (윈도우 높이 - 전체 버튼 영역 높이) / 2
    float StartY = (WindowSize.y - TotalButtonsHeight) * 0.5f;

    // 6) 모든 버튼의 X 위치 = (윈도우 너비 - 버튼 너비) / 2  → 가운데 정렬
    float CenterX = (WindowSize.x - ButtonSize.x) * 0.5f;

    // 7) 첫 번째 버튼 배치
    ImGui::SetCursorPos(ImVec2(CenterX, StartY));
    if (ImGui::Button("Resume", ButtonSize))
    {
        FSoundManager::GetInstance().PlaySound("Select");
        GEngineLoop.GetGameMode()->PauseMatch();
    }

    // 8) 두 번째 버튼 배치: Y = StartY + ButtonHeight + VerticalSpacing
    float SecondY = StartY + ButtonSize.y + VerticalSpacing;
    ImGui::SetCursorPos(ImVec2(CenterX, SecondY));
    if (ImGui::Button("Restart", ButtonSize))
    {
        // Restart 버튼 클릭 시 처리 로직
        // 예: RestartGame();
        FSoundManager::GetInstance().PlaySound("Select");
    }

    // 9) 세 번째 버튼 배치: Y = SecondY + ButtonHeight + VerticalSpacing
    float ThirdY = SecondY + ButtonSize.y + VerticalSpacing;
    ImGui::SetCursorPos(ImVec2(CenterX, ThirdY));
    if (ImGui::Button("MainMenu", ButtonSize))
    {
        for (auto Handle : OnHandle)
        {
            GEngineLoop.GetGameMode()->OnGamePause.Remove(Handle);
        }
        GEngineLoop.GetGameMode()->EndMatch(false);
        GEngineLoop.bPendingGame = true;
        GEngineLoop.bPauseGame = false;
        FSoundManager::GetInstance().StopAllSounds();
        FSoundManager::GetInstance().StopSound("Car_Engine");
        FSoundManager::GetInstance().StopSound("Car_Idle");
        FSoundManager::GetInstance().PlaySound("Select");
        FSoundManager::GetInstance().PlaySound("Intro");
    }

    ImGui::End();
}
