﻿// Fill out your copyright notice in the Description page of Project Settings.


#include "UEExtendedInverseKinematicsInstance.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetMathLibrary.h"
#include "UEExpandedFramework/Gameplay/Trace/UEExtendedTraceData.h"
#include "UEExpandedFramework/Gameplay/Trace/UEExtendedTraceLibrary.h"

UUEExtendedInverseKinematicsInstance::UUEExtendedInverseKinematicsInstance(const FObjectInitializer& ObjectInitializer)
{
	bNeedsUpdate = true;
}

void UUEExtendedInverseKinematicsInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	if (const auto character = Cast<ACharacter>(TryGetPawnOwner()))
	{
		Character = character;
		CharacterMesh = character->GetMesh();
		CharacterCapsule = character->GetCapsuleComponent();
	}
}


void UUEExtendedInverseKinematicsInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	IKUpdate(true);
}



float UUEExtendedInverseKinematicsInstance::IKFootTrace(float TraceDistance, FName SocketName)
{
	float Offset = 0.f;
	
	if (Character && CharacterMesh)
	{
		const FVector SocketVec = CharacterMesh->GetSocketLocation(SocketName);

		LineTraceStruct.Start = FVector(SocketVec.X, SocketVec.Y, SocketVec.Z + TraceDistance);
		LineTraceStruct.End = FVector(SocketVec.X, SocketVec.Y, SocketVec.Z - TraceDistance);
		LineTraceStruct.bTraceComplex = true;

		if(UUEExtendedTraceLibrary::ExtendedLineTraceSingle(GetWorld(),LineTraceStruct))
		{
			const FVector HitVector = LineTraceStruct.GetHitLocation() - LineTraceStruct.GetHitTraceEnd();
			Offset = IKAdjustOffset + HitVector.Size() - TraceDistance;
		}
		Impact = LineTraceStruct.GetHitNormal();
	}
	return Offset;
}

void UUEExtendedInverseKinematicsInstance::UpdateCapsuleHalfHeight(float HipShifts, bool ResetDefault)
{
	/*
	if (CharacterCapsule)
	{
		float fShift = IKCapsuleHalfHeight - (FMath::Abs(HipShifts) / 2);
		float fTarget = UKismetMathLibrary::SelectFloat(IKCapsuleHalfHeight, fShift, ResetDefault);
		float finterp = FMath::FInterpTo(
			IKCapsuleHalfHeight,
			fTarget,
			GetWorld()->GetDeltaSeconds(),
			IKHipsInterpSpeed
		);
		CharacterCapsule->SetCapsuleHalfHeight(finterp, true);
	}

	*/
}

FRotator UUEExtendedInverseKinematicsInstance::NormalToRotator(FVector Normal)
{
	float XRoll =FMath::RadiansToDegrees(FMath::Atan2(Normal.Y, Normal.Z));
	float YPitch = FMath::RadiansToDegrees(FMath::Atan2(Normal.X, Normal.Z) * (-1));
	FRotator Rot = FRotator(YPitch, 0.f, XRoll);
	UE_LOG(LogTemp, Warning, TEXT("%f , %f , %f"), &Rot.Roll, &Rot.Pitch, &Rot.Yaw);
	return Rot;
}

void UUEExtendedInverseKinematicsInstance::IKUpdateFootOffset(float TargetValue, float& EffectorVal, float InterpSpeed)
{
	float fInterp = FMath::FInterpTo(EffectorVal, TargetValue, GetWorld()->GetDeltaSeconds(), InterpSpeed);
	EffectorVal = fInterp;
}

void UUEExtendedInverseKinematicsInstance::IKUpdateFootRotation(FRotator TargetValue, FRotator& RotationVar,float InterpSpeed)
{
	FRotator rInterp = FMath::RInterpTo(RotationVar, TargetValue, GetWorld()->GetDeltaSeconds(), InterpSpeed);
	RotationVar = rInterp;
}

void UUEExtendedInverseKinematicsInstance::IKResetVars()
{
	/*Reset Foot Loction*/
	IKUpdateFootOffset(0.0f, RightEffectorLoc, IKFeetInterpSpeed);
	IKUpdateFootOffset(0.0f, LeftEffectorLoc, IKFeetInterpSpeed);
	/*Reset Foot Rotation*/
	IKUpdateFootRotation(FRotator(0.f, 0.f, 0.f), RightFootRotation, IKFeetInterpSpeed);
	IKUpdateFootRotation(FRotator(0.f, 0.f, 0.f), LeftFootRotation, IKFeetInterpSpeed);
	/*Reste Hips Loction*/
	IKUpdateFootOffset(0.0f, HipsOffset, IKHipsInterpSpeed);
	UpdateCapsuleHalfHeight(0.0f, true);
}

bool UUEExtendedInverseKinematicsInstance::IsMoving()
{
	if (Character->GetVelocity().Size() > 0)
		return true;
	return false;
}

void UUEExtendedInverseKinematicsInstance::IKUpdate(bool bEnable)
{
	
	if (bEnable == false)
	{
		return;
	}

	/*Trace Foot offset*/
	float LeftFootOffset = IKFootTrace(IKTraceDistance, LeftFootSocket);
	FRotator Normal = NormalToRotator(Impact);
	IKUpdateFootRotation(Normal, LeftFootRotation, IKFeetInterpSpeed);
	//UKismetSystemLibrary::PrintString(GetWorld(),FString::Printf(TEXT("Rotation is %f ,%f ,%f"),GetActorRotation().Pitch , GetActorRotation().Roll, GetActorRotation().Yaw), 4.0f);
	float RightFootOffset = IKFootTrace(IKTraceDistance, RightFootSocket);
	Normal = NormalToRotator(Impact);
	IKUpdateFootRotation(Normal, RightFootRotation, IKFeetInterpSpeed);

	/*Update Hip translation*/
	bool pickA = FMath::Min(LeftFootOffset, RightFootOffset) < 0;
	float HipOffset = UKismetMathLibrary::SelectFloat(FMath::Min(LeftFootOffset, RightFootOffset), 0.f, pickA);
	IKUpdateFootOffset(HipOffset, HipsOffset, IKHipsInterpSpeed);
	UpdateCapsuleHalfHeight(HipOffset, false);

	/*Update Foot Locations*/
	IKUpdateFootOffset(LeftFootOffset - HipOffset , LeftEffectorLoc, IKFeetInterpSpeed);
	IKUpdateFootOffset(RightFootOffset - HipOffset, RightEffectorLoc, IKFeetInterpSpeed);
}
