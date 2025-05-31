#pragma once
#include "Axis.h"
#include "Quat.h"
#include "Vector.h"
#include "Container/String.h"
#include "Serialization/Archive.h"

struct FTransform
{
    // 기존 멤버 변수
    FVector Translation;
    FQuat Rotation;
    FVector Scale3D;
    
    // 기존 생성자들
    FTransform();
    FTransform(const FQuat& InRotation, const FVector& InTranslation, const FVector& InScale3D = FVector(1.0f, 1.0f, 1.0f));
    FTransform(const FRotator& InRotation, const FVector& InTranslation, const FVector& InScale3D = FVector(1.0f, 1.0f, 1.0f));
    
    // 추가 생성자들
    explicit FTransform(const FVector& InTranslation);
    explicit FTransform(const FQuat& InRotation);
    explicit FTransform(const FRotator& InRotation);
    explicit FTransform(const FMatrix& InMatrix);
    
    // 변환 함수들
    FVector TransformPosition(const FVector& V) const;
    FVector InverseTransformPosition(const FVector& V) const;
    FVector TransformVector(const FVector& V) const;
    FVector TransformVectorWithoutScale(const FVector& V) const;
    FVector InverseTransformVector(const FVector& V) const;
    FVector TransformDirection(const FVector& V) const;
    FVector InverseTransformDirection(const FVector& V) const;
    
    // 회전 관련 추가 함수들
    FQuat GetRotation() const { return Rotation; }
    FRotator Rotator() const { return Rotation.Rotator(); }
    void SetRotation(const FQuat& InRotation) { Rotation = InRotation; }
    
    // 위치 관련 함수들
    FVector GetTranslation() const { return Translation; }
    void SetTranslation(const FVector& InTranslation) { Translation = InTranslation; }
    void AddToTranslation(const FVector& InTranslation) { Translation += InTranslation; }
    
    // 스케일 관련 함수들
    FVector GetScale3D() const { return Scale3D; }
    void SetScale3D(const FVector& InScale) { Scale3D = InScale; }
    bool IsUniform() const;
    float GetMaximumAxisScale() const;
    float GetMinimumAxisScale() const;
    
    // 누적 변환 함수들
    void Accumulate(const FTransform& Other);
    void MultiplyScale3D(const FVector& InMultiplyScale);
    
    // 정규화 및 유효성 검사
    bool IsValid() const;
    bool IsRotationNormalized() const;
    void NormalizeRotation();
    
    // 변환 연산자들과 함수들
    FTransform operator*(const FTransform& Other) const;
    FTransform operator*(float Scale) const;
    
    void Blend(const FTransform& Atom1, const FTransform& Atom2, float Alpha);
    FTransform BlendWith(const FTransform& Other, float Alpha) const;
    
    // 선형 보간 함수
    static FTransform LerpTransform(const FTransform& A, const FTransform& B, float Alpha);
    
    // 항등 변환 관련
    bool IsIdentity() const;
    void SetIdentity();
    void SetIdentityZeroScale();
    static const FTransform Identity;
    
    // 역변환 계산
    FTransform Inverse() const;
    
    // 행렬 변환
    FMatrix ToMatrixWithScale() const;
    FMatrix ToMatrixNoScale() const;
    void SetFromMatrix(const FMatrix& InMatrix);
    
    // 상대 변환 계산
    FTransform GetRelativeTransform(const FTransform& Other) const;
    FTransform GetRelativeTransformReverse(const FTransform& Other) const;

    /**
     * @brief 지정된 로컬 축에 해당하는 월드 공간 단위 벡터를 반환합니다.
     * @param Axis 가져올 축 (EAxis::X, EAxis::Y, EAxis::Z)
     * @return 트랜스폼의 회전에 의해 변환된 단위 벡터
     */
    FVector GetUnitAxis(EAxis::Type Axis) const
    {
        switch (Axis)
        {
        case EAxis::X:
            return Rotation.RotateVector(FVector(1.0f, 0.0f, 0.0f));
        case EAxis::Y:
            return Rotation.RotateVector(FVector(0.0f, 1.0f, 0.0f));
        case EAxis::Z:
            return Rotation.RotateVector(FVector(0.0f, 0.0f, 1.0f));
        default:
            // 잘못된 축이거나 EAxis::None과 같은 경우, 0 벡터 또는 기본 축을 반환할 수 있습니다.
            // 여기서는 X축을 기본값으로 반환하거나, 필요하다면 경고/오류 처리를 추가합니다.
            // check(false); // 잘못된 입력에 대한 어설트
            return {1.0f, 0.0f, 0.0f}; // 또는 FVector::ZeroVector
        }
    }
    
    // 디버깅 및 직렬화 관련
    FString ToString() const;
    bool Serialize(FArchive& Ar);
    
    // 메모리 레이아웃 최적화 관련 (SIMD 연산용)
    bool ContainsNaN() const;
    
    // 추가적인 변환 유틸리티
    FVector4 TransformFVector4(const FVector4& V) const;
    FVector4 TransformFVector4NoScale(const FVector4& V) const;
    
    // 애니메이션 관련 함수들
    void ScaleTranslation(const FVector& InScale3D);
    void RemoveScaling(float Tolerance = SMALL_NUMBER);
    FVector GetSafeScaleReciprocal(const FVector& InScale, float Tolerance = SMALL_NUMBER) const;
    
    // 보간 함수들
    static void BlendFromIdentityAndAccumulate(FTransform& OutTransform, const FTransform& InTransform, float Alpha);
    
    void AccumulateWithShortestRotation(const FTransform& DeltaAtom, float BlendWeight);

    inline friend FArchive& operator<<(FArchive& Ar, FTransform& Transform)
    {
        return Ar << Transform.Translation
                  << Transform.Rotation
                  << Transform.Scale3D;
    }
};
