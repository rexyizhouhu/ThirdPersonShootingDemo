// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CppCharacter.generated.h"

UCLASS()
class THIRDPERSONDEMO_API ACppCharacter : public ACharacter
{
	GENERATED_BODY()

    ///** 摄像机吊杆将摄像机置于角色身后 */
    //UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
    //    class USpringArmComponent* CameraBoom;

    ///** 跟随摄像机 */
    //UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
    //    class UCameraComponent* FollowCamera;
public:

    /** 构造函数 */
    ACppCharacter();

    /** 属性复制 */
    void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /** 基础旋转速度，单位为度/秒。其他缩放比例可能会影响最终旋转速度。*/
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
        float BaseTurnRate;

    /** 基础向上/下看速度，单位为度/秒。其他缩放比例可能会影响最终速度。*/
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
        float BaseLookUpRate;

protected:
    /** 玩家的最大生命值。这是玩家的最高生命值，也是出生时的生命值。*/
    UPROPERTY(EditDefaultsOnly, Category = "Health")
        float MaxHealth;

    /** 玩家的当前生命值。降到0就表示死亡。*/
    UPROPERTY(ReplicatedUsing = OnRep_CurrentHealth)
        float CurrentHealth;

    /** RepNotify，用于同步对当前生命值所做的更改。*/
    UFUNCTION()
        void OnRep_CurrentHealth();

    /** 响应要更新的生命值。修改后，立即在服务器上调用，并在客户端上调用以响应RepNotify*/
    void OnHealthUpdate();

public:
    /** 最大生命值的取值函数。*/
    UFUNCTION(BlueprintPure, Category = "Health")
        FORCEINLINE float GetMaxHealth() const { return MaxHealth; }

    /** 当前生命值的取值函数。*/
    UFUNCTION(BlueprintPure, Category = "Health")
        FORCEINLINE float GetCurrentHealth() const { return CurrentHealth; }

    /** 当前生命值的存值函数。将此值的范围限定在0到MaxHealth之间，并调用OnHealthUpdate。仅在服务器上调用。*/
    UFUNCTION(BlueprintCallable, Category = "Health")
        void SetCurrentHealth(float healthValue);

    /** 承受伤害的事件。从APawn覆盖。*/
    UFUNCTION(BlueprintCallable, Category = "Health")
        float TakeDamage(float DamageTaken, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

protected:
    /** 在VR中重置HMD方向。*/
    void OnResetVR();

    /** 调用用于向前/向后输入 */
    void MoveForward(float Value);

    /** 调用用于侧向输入 */
    void MoveRight(float Value);

    //void BeginPlay();

    /**
     * 通过输入调用，以给定速度旋转。
     * @param Rate  这是标准化速度，即1.0表示100%的所需旋转速度
     */
    void TurnAtRate(float Rate);

    /**
     * 通过输入调用，以给定速度向上/下看。
     * @param Rate  这是标准化速度，即1.0表示100%的所需旋转速度
     */
    void LookUpAtRate(float Rate);

    /** 触控输入开始时使用的处理程序。*/
    void TouchStarted(ETouchIndex::Type FingerIndex, FVector Location);

    /** 触控输入停止时使用的处理程序。*/
    void TouchStopped(ETouchIndex::Type FingerIndex, FVector Location);

protected:
    // APawn界面
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
    // APawn界面结束

    /** 角色要发射的发射物类型。*/
    UPROPERTY(EditDefaultsOnly, Category = "Gameplay|Projectile")
        TSubclassOf<class ACppProjectile> ProjectileClass;

    /** 射击之间的延迟，单位为秒。用于控制测试发射物的射击速度，还可防止服务器函数的溢出导致将SpawnProjectile直接绑定至输入。*/
    UPROPERTY(EditDefaultsOnly, Category = "Gameplay")
        float FireRate;

    /** 若为true，此武器正在发射过程中。*/
    bool bIsFiringWeapon;

    /** 用于启动武器发射的函数。应仅可由本地玩家触发。*/
    UFUNCTION(BlueprintCallable, Category = "Gameplay")
        void StartFire();

    /** 用于结束武器射击的函数。一旦调用这段代码，玩家可再次使用StartFire。*/
    UFUNCTION(BlueprintCallable, Category = "Gameplay")
        void StopFire();

    /** 用于生成投射物的服务器函数。*/
    UFUNCTION(Server, Reliable)
        void HandleFire();

    /** 定时器句柄，用于提供生成间隔时间内的射速延迟。*/
    FTimerHandle FiringTimer;

public:
    ///** 返回CameraBoom子对象 **/
    //FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
    ///** 返回FollowCamera子对象 **/
    //FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};