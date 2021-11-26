// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CppProjectile.generated.h"

class UParticleSystem;
class UStaticMeshComponent;
class USphereComponent;
class UProjectileMovementComponent;
class UDamageType;

UCLASS()
class THIRDPERSONDEMO_API ACppProjectile : public AActor
{
	GENERATED_BODY()
	
public:
    // 为此Actor的属性设置默认值
    ACppProjectile();

    // 此投射物的基本组件
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
        USphereComponent* SphereComponent;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
        UStaticMeshComponent* StaticMesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
        UProjectileMovementComponent* ProjectileMovementComponent;
    UPROPERTY(EditAnywhere, Category = "Effects")
        UParticleSystem* ExplosionEffect;

    //此投射物将造成的伤害类型和伤害。
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage")
        TSubclassOf<UDamageType> DamageType;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage")
        float Damage;

protected:
    // 当游戏开始或生成时调用
    virtual void BeginPlay() override;

    UFUNCTION()
        void OnProjectileImpact(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

    void Destroyed() override;

public:
    // 每一帧调用
    virtual void Tick(float DeltaTime) override;
};
