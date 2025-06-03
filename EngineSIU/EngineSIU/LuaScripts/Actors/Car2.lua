
setmetatable(_ENV, { __index = EngineTypes })

local ReturnTable = {}

IsWPressed = false
IsTurningR = false
IsTurningL = false
IsRPressed = false
MaxVelocity = 50
MaxBoost = 4000
DeltaSteerAngle = math.pi / 6

function ReturnTable:InitializeLua()
    controller("W", OnPressW)    
    controller("S", OnPressS)
    controller("A", OnPressA)
    controller("D", OnPressD)
    controller("R", OnPressR)
end

function OnPressW(dt)
    IsWPressed = true
    print("W")
    --
end

function OnPressS(dt)
    --후진은 불가
end

function OnPressA(dt)
    IsTurningL = true
end

function OnPressD(dt)
    IsTurningR = true
end

function OnPressR(dt)
    print("R Pressed")
    IsRPressed = true
end

-- BeginPlay: Actor가 처음 활성화될 때 호출
function ReturnTable:BeginPlay()
    --SlopeAngle = Car.SlopeAngle
    --EndLoc = 100 * math.cos(SlopeAngle) + 1.5
    print("BeginPlay ", self.Name) -- Table에 등록해 준 Name 출력.

end

-- Tick: 매 프레임마다 호출
function ReturnTable:Tick(DeltaTime)
    local this = self.this
    -- this.ActorLocation = this.ActorLocation + FVector(1.0, 0.0, 0.0) * DeltaTime -- X 방향으로 이동하도록 선언.

end

-- EndPlay: Actor가 파괴되거나 레벨이 전환될 때 호출
function ReturnTable:EndPlay(EndPlayReason)
    -- print("[Lua] EndPlay called. Reason:", EndPlayReason) -- EndPlayReason Type 등록된 이후 사용 가능.
    print("EndPlay")

end

function ReturnTable:Attack(AttackDamage)
    self.GetDamate(AttackDamage)

end

return ReturnTable
