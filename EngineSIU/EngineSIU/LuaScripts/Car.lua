IsWPressed = false
IsTurningR = false
IsTurningL = false
MaxVelocity = 30
MaxBoost = 4000
DeltaSteerAngle = math.pi / 9

function BeginPlay()
    SlopeAngle = Car.SlopeAngle
    EndLoc = 100 * math.cos(SlopeAngle) + 1.5
end

function EndPlay()
    print("[EndPlay]")
end

function OnOverlap(OtherActor)
end

function InitializeLua()
    controller("W", OnPressW)    
    controller("S", OnPressS)
    controller("A", OnPressA)
    controller("D", OnPressD)
end

function OnPressW(dt)
    IsWPressed = true
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

function Clamp(x, lower, upper)
    if x < lower then
        return lower
    elseif x > upper then
        return upper
    else
        return x
    end
end


function Tick(dt)
    local CarLocX = actor.Location.X;
    if CarLocX > EndLoc and not Car.IsBoosted then
        Car:BoostCar()
    end

    local Velocity = Car.Velocity
    local SteerAngle = Car.SteerAngle
    local Boost = Car.Boost
    if IsWPressed then
        if Velocity < 0 then
        Velocity = 0
        end
        Velocity = Velocity + 0.1
        Boost = Boost + 20 * Velocity/MaxVelocity
    else
        if Velocity > 0 then
            Velocity = Velocity - 0.1
        end
        if math.abs(Velocity) < 0.1 then
            Velocity = 0
        end
        if Car.IsBoosted then
            Boost = Boost - 5
        end
    end

    if IsTurningL then
        SteerAngle = SteerAngle + DeltaSteerAngle
    elseif IsTurningR then
        SteerAngle = SteerAngle - DeltaSteerAngle
    else
        SteerAngle = 0
    end
    Velocity = Clamp(Velocity, 0, MaxVelocity)
    Boost = Clamp(Boost, 0, MaxBoost)
    Car.Velocity = Velocity
    Car.Boost = Boost
    Car.SteerAngle = SteerAngle
    if not Car.IsBoosted then
        Car:Move()
    end
    IsWPressed = false
    IsTurningR = false
    IsTurningL = false
end

function BeginOverlap()
end

function EndOverlap()
end
