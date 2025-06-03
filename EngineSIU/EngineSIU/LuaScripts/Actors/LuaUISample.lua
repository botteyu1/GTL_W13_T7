
setmetatable(_ENV, { __index = EngineTypes })

-- Template은 AActor라는 가정 하에 작동.

local ReturnTable = {} -- Return용 table. cpp에서 Table 단위로 객체 관리.

local FVector = EngineTypes.FVector -- EngineTypes로 등록된 FVector local로 선언.

local LuaUIBind = EngineTypes.LuaUIBind
local RectTransform = EngineTypes.RectTransform
local FString = EngineTypes.FString
local AnchorDirection = EngineTypes.AnchorDirection
local FLinearColor = EngineTypes.FLinearColor

local LuaTextUI = EngineTypes.LuaTextUI
local LuaImageUI = EngineTypes.LuaImageUI

-- BeginPlay: Actor가 처음 활성화될 때 호출
function ReturnTable:BeginPlay()

    print("BeginPlay ", self.Name) -- Table에 등록해 준 Name 출력.

    self.ScoreUI = nil
    self.ManagedImageUI = nil
    self.ElapsedTime = 0 -- 시간 누적용
    self.fireCount=0

    self:InitScoreUI()
    self:InitCountUI()
    self:InitControlsUI()
    self:InitResultUI()

    -- local imageName = FString.new("MyImageUI")
    -- local textureName = FString.new("ExplosionColor")

    -- local TexturePosX = -450.0
    -- local TexturePosY = 50.0
    -- local TextureWidth = 300.0
    -- local TextureHeight = 80.0
    -- local TextureAnchor = AnchorDirection.MiddleCenter

    -- local TextureRect = RectTransform.new(TexturePosX, TexturePosY, TextureWidth, TextureHeight, TextureAnchor)

    -- local TextureSortOrder = 10
    -- local TextureColor = FLinearColor.new(1.0, 1.0, 1.0, 0.1)

    -- LuaUIBind.CreateImage(imageName, TextureRect, TextureSortOrder, textureName, TextureColor)


    -- 생성된 UI 객체 가져오기 (Tick에서 사용하기 위해)
    -- LuaUIBind.GetTextUI/GetImageUI는 포인터를 반환하므로, Lua에서 객체로 다뤄짐
    -- self.ManagedImageUI = LuaUIBind.GetImageUI(imageName)


    -- if not self.ManagedImageUI then
    --     print("Error: Could not get ManagedImageUI with name: " .. imageName:ToAnsiString())
    -- end

end

-- Tick: 매 프레임마다 호출
function ReturnTable:Tick(DeltaTime)
    -- 기본적으로 Table로 등록된 변수는 self, Class usertype으로 선언된 변수는 self.this로 불러오도록 설정됨.
    -- sol::property로 등록된 변수는 변수 사용으로 getter, setter 등록이 되어 .(dot) 으로 접근가능하고
    -- 바로 등록된 경우에는 PropertyName() 과 같이 함수 형태로 호출되어야 함.
    local this = self.this
    self:UpdateScoreUI()
    self:UpdateCountUI();
end

-- EndPlay: Actor가 파괴되거나 레벨이 전환될 때 호출
function ReturnTable:EndPlay(EndPlayReason)
    -- print("[Lua] EndPlay called. Reason:", EndPlayReason) -- EndPlayReason Type 등록된 이후 사용 가능.
    print("EndPlay")
    local fallbackImageColor = FLinearColor.new(0.0, 1.0, 1.0, 0.0) 
    self.ResultUI:SetColor(fallbackImageColor)

end

function ReturnTable:Attack(AttackDamage)
    self.GetDamate(AttackDamage)

end
function ReturnTable:InitScoreUI()
    local uiName = FString.new("ScoreUI")
    local textContent = FString.new("Score:")

    local posX = 0.0
    local posY = 50.0
    local width = 200.0     -- Text는 FontSize에만 크기 영향 받음
    local height = 80.0
    local anchor = AnchorDirection.TopLeft -- Enum 값 사용

    local myRect = RectTransform.new(posX, posY, width, height, anchor)

    local SortOrder = 20
    local fontName = FString.new("Default")
    local fontSize = 50
    local fontColor = FLinearColor.new(1.0, 0.0, 0.0, 1.0)

    LuaUIBind.CreateText(uiName,  myRect, SortOrder, textContent, fontName, fontSize, fontColor)
    self.ScoreUI = LuaUIBind.GetTextUI(uiName)
        -- 객체를 제대로 가져왔는지 확인 (nil 체크)
    if not self.ScoreUI then
        print("Error: Could not get ScoreUI with name: "..uiName)
    end
end
function ReturnTable:UpdateScoreUI()
    if not self.ScoreUI then
        return
    end

    local maxEnemy = GetEnemyCountInWorld()
    local ragdollEnemy = GetRagdollEnemyCountInWorld()
    if maxEnemy==ragdollEnemy then
        if self.fireCount==0 then
            self.fireCount=GetFireCount()
        end
        self:UpdateResultUI()
    end
    if self.ScoreUI then
        local text = FString.new(string.format("Score: %d / %d", ragdollEnemy, maxEnemy))
        self.ScoreUI:SetText(text)
    end
end

function ReturnTable:InitCountUI()
    local uiName = FString.new("CountUI")
    local textContent = FString.new("Fire Count Left:")

    local posX = 0.0
    local posY = 100.0
    local width = 200.0     -- Text는 FontSize에만 크기 영향 받음
    local height = 80.0
    local anchor = AnchorDirection.TopLeft -- Enum 값 사용

    local myRect = RectTransform.new(posX, posY, width, height, anchor)

    local SortOrder = 20
    local fontName = FString.new("Default")
    local fontSize = 50
    local fontColor = FLinearColor.new(1.0, 0.0, 0.0, 1.0)

    LuaUIBind.CreateText(uiName,  myRect, SortOrder, textContent, fontName, fontSize, fontColor)
    self.CountUI = LuaUIBind.GetTextUI(uiName)
        -- 객체를 제대로 가져왔는지 확인 (nil 체크)
    if not self.CountUI then
        print("Error: Could not get CountUI with name: "..uiName)
    end
end
function ReturnTable:UpdateCountUI()
    if not self.CountUI then
        return
    end

    local curFireCount = GetFireCount()

    if self.CountUI then
        local text = FString.new(string.format("Fire Count: %d", curFireCount))
        self.CountUI:SetText(text)
    end
end
function ReturnTable:InitResultUI()
    local uiName = FString.new("ResultUI")
    local initialText = FString.new("")  -- 처음에는 비어 있음

    local posX = 0.0
    local posY = 0.0
    local width = 600.0
    local height = 100.0
    local anchor = AnchorDirection.MiddleCenter

    local myRect = RectTransform.new(posX, posY, width, height, anchor)

    local SortOrder = 100
    local fontName = FString.new("Default")
    local fontSize = 60
    local fontColor = FLinearColor.new(0.0, 1.0, 0.0, 1.0)

    LuaUIBind.CreateText(uiName, myRect, SortOrder, initialText, fontName, fontSize, fontColor)
    self.ResultUI = LuaUIBind.GetTextUI(uiName)

    if not self.ResultUI then
        print("Error: Could not get ResultUI with name: " .. uiName)
    end
end
function ReturnTable:UpdateResultUI()
    if not self.ResultUI then
        return
    end

    local resultText = FString.new(string.format("You Win! Total Fire Count: %d", self.fireCount))
    self.ResultUI:SetText(resultText)

    local visibleColor = FLinearColor.new(0.0, 0.0, 0.0, 1.0)
    self.ResultUI:SetFontColor(visibleColor)
end

function ReturnTable:InitControlsUI()
    local uiName = FString.new("ControlsUI")
    local controlsText = FString.new("Controls:\nQ: Boost\nR: Start\n1: Free Camera")

    local posX = -200.0  -- 오른쪽에서 약간 왼쪽으로
    local posY = 20.0   -- 상단에서 약간 아래로
    local width = 400.0
    local height = 150.0
    local anchor = AnchorDirection.TopRight

    local myRect = RectTransform.new(posX, posY, width, height, anchor)

    local SortOrder = 30
    local fontName = FString.new("Default")
    local fontSize = 28
    local fontColor = FLinearColor.new(0.0, 1.0, 0.0, 1.0) -- 흰색

    LuaUIBind.CreateText(uiName, myRect, SortOrder, controlsText, fontName, fontSize, fontColor)
    self.ControlsUI = LuaUIBind.GetTextUI(uiName)

    if not self.ControlsUI then
        print("Error: Could not get ControlsUI with name: "..uiName)
    end
end

return ReturnTable
