// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine.h"
#include "GameFramework/Actor.h"
#include "SandboxEnvironment.generated.h"

struct SandboxGameTime {
	long days;
	long hours;
	long minutes;
	long seconds;

	int month;
	int year;
};


UCLASS()
class UNREALSANDBOXTOOLKIT_API ASandboxEnvironment : public AActor
{
	GENERATED_BODY()
	
public:	
	ASandboxEnvironment();

	virtual void BeginPlay() override;
	
	virtual void Tick( float DeltaSeconds ) override;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	ADirectionalLight* DirectionalLightSource;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	ASkyLight* SkyLight;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	UCurveFloat* DayNightCycleSkyLightCurve;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	UCurveFloat* HeightCurve;

	UPROPERTY(EditAnywhere, Category = "Sandbox Cave")
	float CaveSkyLightRatio;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	AExponentialHeightFog* GlobalFog;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	UCurveFloat* GlobalFogDensityCurve;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	UCurveFloat* GlobalFogOpacityCurve;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	AStaticMeshActor* CaveSphere;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	AAmbientSound* AmbientSound;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	float TimeSpeed;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	float RecaptureSkyTreshold;

	UPROPERTY(EditAnywhere, Category = "Sandbox Cave")
	float CaveFogDensity;

	UPROPERTY(EditAnywhere, Category = "Sandbox Cave")
	float CaveFogOpacity;

	UPROPERTY(EditAnywhere, Category = "Sandbox Cave")
	FLinearColor CaveFogInscatteringColor;

	UPROPERTY(EditAnywhere, Category = "Sandbox DayNight cycle")
	bool bEnableDayNightCycle;

	UPROPERTY(EditAnywhere, Category = "Sandbox DayNight cycle")
	int InitialYear = 2016;

	UPROPERTY(EditAnywhere, Category = "Sandbox DayNight cycle")
	int InitialMonth = 6;

	UPROPERTY(EditAnywhere, Category = "Sandbox DayNight cycle")
	int InitialDay = 10;

	//UPROPERTY(EditAnywhere, Category = "Sandbox DayNight cycle")
	//int InitialHour = 12;

	UPROPERTY(EditAnywhere, Category = "Sandbox DayNight cycle")
	int TimeZone;

	UPROPERTY(EditAnywhere, Category = "Sandbox DayNight cycle")
	float Lat;

	UPROPERTY(EditAnywhere, Category = "Sandbox DayNight cycle")
	float Lng;

	float ClcGameTime(float RealServerTime);

	SandboxGameTime ClcLocalGameTime(float RealServerTime);

	SandboxGameTime ClcGameTimeOfDay(float RealServerTime, bool bAccordingTimeZone);

	void SetTimeOffset(float time);

	double GetNewTimeOffset();

	void UpdatePlayerPosition(FVector Pos, float GroundLevel = 0);

	void SetCaveMode(bool bCaveModeEnabled);

	bool IsNight() const;

private:

	bool bIsNight;

	bool bCaveMode = false;

	float LastTime;

	float InitialFogOpacity;

	float InitialFogDensity;

	double RealTimeOffset = 0;

	void PerformDayNightCycle();

	FLinearColor FogColor;

	FVector PlayerPos;

	float GroundLevelPlayerPos;

	float InitialSkyIntensity;
	
};
