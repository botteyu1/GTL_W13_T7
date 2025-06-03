IsWPressed = false
IsTurningR = false
IsTurningL = false
IsRPressed = false
MaxVelocity = 50
MaxBoost = 4000
DeltaSteerAngle = math.pi / 6

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
    print("Tick")
    local CarLocX = actor.Location.X;
    if CarLocX > EndLoc and not Car.IsBoosted then
        Car:BoostCar()
    end

    if(Car.IsBoosted and Car:Speed() < 0.1) then
        if IsRPressed then
            Car:Restart()
        end
    end

    local Velocity = Car.Velocity
    local SteerAngle = Car.SteerAngle
    local Boost = Car.Boost
    if IsWPressed then
        if Velocity < 0 then
            Velocity = 0
        end
        Velocity = Velocity + 0.1
        Boost = Boost + 10 * Velocity/MaxVelocity
    else
        if Velocity > 0 then
            Velocity = Velocity - 0.1
        end
        if math.abs(Velocity) < 0.1 then
            Velocity = 0
        end
        if not Car.IsBoosted then
            Boost = Boost - 2.5
        end
    end

    if IsTurningL then
        SteerAngle = DeltaSteerAngle
    elseif IsTurningR then
        SteerAngle = -DeltaSteerAngle
    else
        CurAngle = Car:CurSteerAngle()
        if CurAngle > 0.1 then
            SteerAngle = - DeltaSteerAngle
        elseif CurAngle < -0.1 then
            SteerAngle = DeltaSteerAngle
        end
        if math.abs(CurAngle)<0.01 then
            SteerAngle = 0 
        end
    end
    Velocity = Clamp(Velocity, 0, MaxVelocity)
    Boost = Clamp(Boost, 0, MaxBoost)
    
    if not Car.IsBoosted then
        Car.Velocity = Velocity
        Car.SteerAngle = SteerAngle
    else
        Car.Velocity = 0
        Car.SteerAngle = 0
    end
    Car.Boost = Boost
    
    Car:Move()
    IsWPressed = false
    IsTurningR = false
    IsTurningL = false
    IsRPressed = false
end

function BeginOverlap()
end

function EndOverlap()
end
