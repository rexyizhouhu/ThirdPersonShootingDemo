// Fill out your copyright notice in the Description page of Project Settings.


#include "CppCharacter.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"
#include "TimerManager.h"
#include "CppProjectile.h"

// Sets default values
ACppCharacter::ACppCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

    // 设置碰撞胶囊体的大小
    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

    // 设置输入的旋转速度
    BaseTurnRate = 45.f;
    BaseLookUpRate = 45.f;

    // 控制器旋转时不旋转。只影响摄像机。
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    // 配置角色移动
    GetCharacterMovement()->bOrientRotationToMovement = true; // 角色朝输入的方向移动...  
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...采用此旋转速度
    GetCharacterMovement()->JumpZVelocity = 600.f;
    GetCharacterMovement()->AirControl = 0.2f;

    //// 创建摄像机吊杆（发生碰撞时向玩家拉近）
    //CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    //CameraBoom->SetupAttachment(RootComponent);
    //CameraBoom->TargetArmLength = 300.0f; // 摄像机以这个距离跟在角色身后 
    //CameraBoom->bUsePawnControlRotation = true; // 基于控制器旋转吊臂

    //// 创建跟随摄像头
    //FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    //FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // 将摄像机连接到吊杆末端，调节吊杆以匹配控制器方向
    //FollowCamera->bUsePawnControlRotation = false; // 摄像机不相对于吊臂旋转

    // 注意：骨架网格体和网格体组件上的动画蓝图引用（继承自角色） 
    // 都在名为MyCharacter的派生蓝图资产中设置（以避免C++环境下的直接内容引用）

    //初始化玩家生命值
    MaxHealth = 100.0f;
    CurrentHealth = MaxHealth;

    //初始化投射物类
    ProjectileClass = ACppProjectile::StaticClass();
    //初始化射速
    FireRate = 0.25f;
    bIsFiringWeapon = false;
}

//////////////////////////////////////////////////////////////////////////
// 输入

void ACppCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
    //Super::SetupPlayerInputComponent(PlayerInputComponent);

    // 设置游戏进程键绑定
    check(PlayerInputComponent);
    PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
    PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

    PlayerInputComponent->BindAxis("MoveForward", this, &ACppCharacter::MoveForward);
    PlayerInputComponent->BindAxis("MoveRight", this, &ACppCharacter::MoveRight);

    // 我们有两个旋转绑定版本，可以用不同的方式处理不同类型的设备
    // "turn"处理提供绝对增量的设备。
    // "turnrate"用于选择视为变化速度的设备，例如模拟操纵杆
    PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
    PlayerInputComponent->BindAxis("TurnRate", this, &ACppCharacter::TurnAtRate);
    PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
    PlayerInputComponent->BindAxis("LookUpRate", this, &ACppCharacter::LookUpAtRate);

    // 处理触控设备
    PlayerInputComponent->BindTouch(IE_Pressed, this, &ACppCharacter::TouchStarted);
    PlayerInputComponent->BindTouch(IE_Released, this, &ACppCharacter::TouchStopped);

    // VR头戴设备功能
    PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &ACppCharacter::OnResetVR);

    // 处理发射投射物
    PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ACppCharacter::StartFire);
    //PlayerInputComponent->BindAction("Fire", IE_Released, this, &AThirdPersonMPCharacter::StopFire);
}


//////////////////////////////////////////////////////////////////////////
// 复制的属性

void ACppCharacter::GetLifetimeReplicatedProps(TArray <FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    //复制当前生命值。
    DOREPLIFETIME(ACppCharacter, CurrentHealth);
}

void ACppCharacter::StartFire()
{
    if (!bIsFiringWeapon)
    {
        bIsFiringWeapon = true;
        UWorld* World = GetWorld();
        World->GetTimerManager().SetTimer(FiringTimer, this, &ACppCharacter::StopFire, FireRate, false);
        HandleFire();
    }
}

void ACppCharacter::StopFire()
{
    bIsFiringWeapon = false;
}

void ACppCharacter::HandleFire_Implementation()
{
    FVector spawnLocation = GetActorLocation() + (GetActorRotation().Vector() * 100.0f) + (GetActorUpVector() * 50.0f);
    FRotator spawnRotation = GetActorRotation();

    FActorSpawnParameters spawnParameters;
    spawnParameters.Instigator = GetInstigator();
    spawnParameters.Owner = this;

    auto * spawnedProjectile = GetWorld()->SpawnActor<ACppProjectile>(ProjectileClass, spawnLocation, spawnRotation, spawnParameters);
}

void ACppCharacter::OnHealthUpdate()
{
    //客户端特定的功能
    if (IsLocallyControlled())
    {
        FString healthMessage = FString::Printf(TEXT("You now have %f health remaining."), CurrentHealth);
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, healthMessage);

        if (CurrentHealth <= 0)
        {
            FString deathMessage = FString::Printf(TEXT("You have been killed."));
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, deathMessage);
        }
    }

    //服务器特定的功能
    if (GetLocalRole() == ROLE_Authority)
    {
        FString healthMessage = FString::Printf(TEXT("%s now has %f health remaining."), *GetFName().ToString(), CurrentHealth);
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, healthMessage);
    }

    //在所有机器上都执行的函数。 
    /*
        因任何因伤害或死亡而产生的特殊功能都应放在这里。
    */

//TODO: 
    //Ragdoll: OnDeath()
}

void ACppCharacter::OnRep_CurrentHealth()
{
    OnHealthUpdate();
}

void ACppCharacter::SetCurrentHealth(float healthValue)
{
    if (GetLocalRole() == ROLE_Authority)
    {
        CurrentHealth = FMath::Clamp(healthValue, 0.f, MaxHealth);
        OnHealthUpdate();
    }
}

float ACppCharacter::TakeDamage(float DamageTaken, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    float damageApplied = CurrentHealth - DamageTaken;
    SetCurrentHealth(damageApplied);
    return damageApplied;
}

void ACppCharacter::OnResetVR()
{
    UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void ACppCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
    Jump();
}

void ACppCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
    StopJumping();
}

void ACppCharacter::TurnAtRate(float Rate)
{
    // 根据速度信息计算此帧的增量
    AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void ACppCharacter::LookUpAtRate(float Rate)
{
    // 根据速度信息计算此帧的增量
    AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void ACppCharacter::MoveForward(float Value)
{
    if ((Controller != NULL) && (Value != 0.0f))
    {
        // find out which way is forward
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);

        // 获取前向矢量
        const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        AddMovementInput(Direction, Value);
    }
}

void ACppCharacter::MoveRight(float Value)
{
    if ((Controller != NULL) && (Value != 0.0f))
    {
        // 找出正确的道路
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);

        // 获取正确的矢量 
        const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
        // add movement in that direction
        AddMovementInput(Direction, Value);
    }
}

// Called when the game starts or when spawned
//void ACppCharacter::BeginPlay()
//{
//	Super::BeginPlay();
//    OnRep_CurrentHealth();
//}

//// Called every frame
//void ACppCharacter::Tick(float DeltaTime)
//{
//	Super::Tick(DeltaTime);
//
//}

// Called to bind functionality to input
//void ACppCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
//{
//	Super::SetupPlayerInputComponent(PlayerInputComponent);
//
//}

