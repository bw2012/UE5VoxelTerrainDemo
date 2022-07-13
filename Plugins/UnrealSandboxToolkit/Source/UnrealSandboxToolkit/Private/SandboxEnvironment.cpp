// Fill out your copyright notice in the Description page of Project Settings.

#include "SandboxEnvironment.h"
#include <ctime>
#include "SunPos.h"


ASandboxEnvironment::ASandboxEnvironment() {
	PrimaryActorTick.bCanEverTick = true;
	TimeSpeed = 10.f;
	RecaptureSkyTreshold = 0.5f;
	bEnableDayNightCycle = true;
	Lng = 27.55;
	Lat = 53.91;
	TimeZone = +3;
	PlayerPos = FVector::ZeroVector;
	InitialSkyIntensity = 0.1;
	CaveSkyLightRatio = 1;
	GroundLevelPlayerPos = 0;

	CaveFogDensity = 0.5;
	CaveFogOpacity = 0.5;
}

float GetSkyLightIntensity(ASkyLight* SkyLight) {
	if (SkyLight) {
		USkyLightComponent* SkyLightComponent = SkyLight->GetLightComponent();
		if (SkyLightComponent) {
			return SkyLightComponent->Intensity;
		}
	}

	return -1;
}

void ASandboxEnvironment::BeginPlay() {
	Super::BeginPlay();

	if (DirectionalLightSource){
		DirectionalLightSource->SetActorRotation(FRotator(-90.0f, 0.0f, 0.0f));
	}

	if (CaveSphere) {
		CaveSphere->GetStaticMeshComponent()->SetVisibility(bCaveMode);
	}

	if (GlobalFog) {
		UExponentialHeightFogComponent* FogCmoponent = GlobalFog->GetComponent();
		InitialFogOpacity = FogCmoponent->FogMaxOpacity;
		InitialFogDensity = FogCmoponent->FogDensity;
		//FogColor = FogCmoponent->FogInscatteringColor;
	}

	if (SkyLight) {
		InitialSkyIntensity = GetSkyLightIntensity(SkyLight);
	}
}

void ASandboxEnvironment::Tick( float DeltaTime ) {
	Super::Tick( DeltaTime );

	if(bEnableDayNightCycle) {
		PerformDayNightCycle();
	}
}

void SetSkyLightIntensity(ASkyLight* SkyLight, float Intensity) {
	if (SkyLight) {
		USkyLightComponent* SkyLightComponent = SkyLight->GetLightComponent();
		if (SkyLightComponent) {
			SkyLightComponent->Intensity = Intensity;
			//SkyLightComponent->RecaptureSky();
			SkyLightComponent->MarkRenderStateDirty();
		}
	}
}

void SetFogParams(AExponentialHeightFog* GlobalFog, float Density, float Opacity) {
	if (GlobalFog) {
		UExponentialHeightFogComponent* FogCmoponent = GlobalFog->GetComponent();
		FogCmoponent->SetFogDensity(Density);
		FogCmoponent->SetFogMaxOpacity(Opacity);
	}
}

void SetFogParams(AExponentialHeightFog* GlobalFog, float Density, float Opacity, FLinearColor Color) {
	if (GlobalFog) {
		UExponentialHeightFogComponent* FogCmoponent = GlobalFog->GetComponent();
		FogCmoponent->SetFogDensity(Density);
		FogCmoponent->SetFogMaxOpacity(Opacity);
		FogCmoponent->SetFogInscatteringColor(Color);
	}
}


void ASandboxEnvironment::PerformDayNightCycle() {
	UWorld* World = GetWorld();
	AGameStateBase* GameState = World->GetGameState();

	if (!GameState) {
		return;
	}

	float RealServerTime = GameState->GetServerWorldTimeSeconds();
	SandboxGameTime GameDayTime = ClcGameTimeOfDay(RealServerTime, false); // use UTC time

	//UE_LOG(LogSandboxTerrain, Log, TEXT("%f"), RealServerTime);
	//UE_LOG(LogSandboxTerrain, Log, TEXT("%d : %d"), GameTimeOfDay.hours, GameTimeOfDay.minutes);

	cTime Time;
	Time.iYear = GameDayTime.year;
	Time.iMonth = GameDayTime.month;
	Time.iDay = GameDayTime.days;

	Time.dHours = GameDayTime.hours;
	Time.dMinutes = GameDayTime.minutes;
	Time.dSeconds = GameDayTime.seconds;

	cLocation GeoLoc;
	GeoLoc.dLongitude = Lng;
	GeoLoc.dLatitude = Lat;

	cSunCoordinates SunPosition;

	sunpos(Time, GeoLoc, &SunPosition);

	if (DirectionalLightSource) {
		DirectionalLightSource->SetActorRotation(FRotator(-(90 - SunPosition.dZenithAngle), SunPosition.dAzimuth, 0.0f));

		if (bCaveMode) {
			//DirectionalLightSource->SetEnabled(false);
		} else {
			//DirectionalLightSource->SetEnabled(true);
		}

		float H = 1 - SunPosition.dZenithAngle / 180;
		bIsNight = H < 0.5;

		const float Zc = PlayerPos.Z / 1000;
		const float ZZ = Zc - GroundLevelPlayerPos;

		//UE_LOG(LogSandboxTerrain, Log, TEXT("dZenithAngle -> %f %f"), SunPosition.dZenithAngle, Temp);

		float HeightFactor = 1;
		if (HeightCurve) {
			HeightFactor = HeightCurve->GetFloatValue(ZZ);
			//UE_LOG(LogTemp, Log, TEXT("HeightFactor -> %f %f %f"), Zc, ZZ, HeightFactor);
		}

		if (SkyLight) {
			//SkyLightWorkaround(SkyLight);

			if (DayNightCycleSkyLightCurve) {
				float DayNightFactor = 1;
				float DayNightIntensity = InitialSkyIntensity;

				if (DayNightCycleSkyLightCurve) {
					DayNightFactor = DayNightCycleSkyLightCurve->GetFloatValue(H);
					DayNightIntensity *= DayNightFactor;
				}

				const float CaveSkyLightIntensity = InitialSkyIntensity * CaveSkyLightRatio;
				const float Intensity = (DayNightIntensity * HeightFactor) + (CaveSkyLightIntensity * (1 - HeightFactor));

				//UE_LOG(LogTemp, Log, TEXT("Intensity -> %f"), Intensity);
				SetSkyLightIntensity(SkyLight, Intensity);
			}
		}

		if (GlobalFog) {
			UExponentialHeightFogComponent* FogCmoponent = GlobalFog->GetComponent();
			FogCmoponent->SetFogInscatteringColor(FogColor);

			if (GlobalFogDensityCurve) {
				const float DayNightFogDensity = InitialFogDensity * GlobalFogDensityCurve->GetFloatValue(H);
				const float FogDensity = (DayNightFogDensity * HeightFactor) + (CaveFogDensity * (1 - HeightFactor));
				FogCmoponent->SetFogDensity(FogDensity);
			}

			if (GlobalFogOpacityCurve) {
				const float DayNightFogOpacity = InitialFogOpacity * GlobalFogOpacityCurve->GetFloatValue(H);
				const float FogOpacity = (DayNightFogOpacity * HeightFactor) + (CaveFogOpacity * (1 - HeightFactor));
				FogCmoponent->SetFogMaxOpacity(FogOpacity);
			}
		}
	}
}

float ASandboxEnvironment::ClcGameTime(float RealServerTime) {
	return (RealServerTime + RealTimeOffset) * TimeSpeed;
}

SandboxGameTime ASandboxEnvironment::ClcLocalGameTime(float RealServerTime) {
	long input_seconds = (long)(ClcGameTime(RealServerTime));

	const int cseconds_in_day = 86400;
	const int cseconds_in_hour = 3600;
	const int cseconds_in_minute = 60;
	const int cseconds = 1;

	SandboxGameTime ret;
	ret.days = input_seconds / cseconds_in_day;
	ret.hours = (input_seconds % cseconds_in_day) / cseconds_in_hour;
	ret.minutes = ((input_seconds % cseconds_in_day) % cseconds_in_hour) / cseconds_in_minute;
	ret.seconds = (((input_seconds % cseconds_in_day) % cseconds_in_hour) % cseconds_in_minute) / cseconds;

	return ret;
}

SandboxGameTime ASandboxEnvironment::ClcGameTimeOfDay(float RealServerTime, bool bAccordingTimeZone) {
	std::tm initial_ptm {};
	initial_ptm.tm_hour = 12;
	initial_ptm.tm_min = 0;
	initial_ptm.tm_sec = 0;
	initial_ptm.tm_mon = InitialMonth + 1;
	initial_ptm.tm_mday = InitialDay;
	initial_ptm.tm_year = InitialYear - 1900;

	time_t initial_time = std::mktime(&initial_ptm);

	//static const uint64 InitialOffset = 60 * 60 * 12; // always start game at 12:00
	const uint64 InitialOffset = initial_time;
	const uint64 TimezoneOffset = bAccordingTimeZone ? 60 * 60 * TimeZone : 0;
	const uint64 input_seconds = (int)ClcGameTime(RealServerTime) + InitialOffset + TimezoneOffset;

	time_t rawtime = (time_t)input_seconds;
	tm ptm;

#ifdef _MSC_VER
	gmtime_s(&ptm, &rawtime);
#else
	ptm = *gmtime_r(&rawtime, &ptm);
#endif

	SandboxGameTime Time;
	Time.hours = ptm.tm_hour;
	Time.minutes = ptm.tm_min;
	Time.seconds = ptm.tm_sec;
	Time.days = ptm.tm_mday;
	Time.month = ptm.tm_mon + 1;
	Time.year = ptm.tm_year + 1900;

	return Time;
}

void ASandboxEnvironment::SetTimeOffset(float Offset) {
	RealTimeOffset = Offset;
}

double ASandboxEnvironment::GetNewTimeOffset() {
	AGameStateBase* GameState = GetWorld()->GetGameState();

	if (!GameState) {
		return RealTimeOffset;
	}

	float RealServerTime = GameState->GetServerWorldTimeSeconds();
	return RealTimeOffset + RealServerTime;
}

void ASandboxEnvironment::UpdatePlayerPosition(FVector Pos, float GroundLevel) {
	PlayerPos = Pos;

	if (CaveSphere) {
		CaveSphere->SetActorLocation(Pos);
	}

	if (GlobalFog) {
		GlobalFog->SetActorLocation(Pos);
	}

	
	/*
	if (AmbientSound) {
		float Value = 0;
		if (Pos.Z < -1000) {
			Value = Pos.Z / -3000;
			if (Value > 1) {
				Value = 1;
			}
		}

		FAudioComponentParam Param(TEXT("Z"));
		Param.FloatParam = Value;
		AmbientSound->GetAudioComponent()->SetSoundParameter(Param);
	}
	*/
}


void ASandboxEnvironment::SetCaveMode(bool bCaveModeEnabled) {
	if (bCaveMode == bCaveModeEnabled) {
		return;
	}

	if (CaveSphere) {
		CaveSphere->GetStaticMeshComponent()->SetVisibility(bCaveModeEnabled);
	}

	bCaveMode = bCaveModeEnabled;
}

bool ASandboxEnvironment::IsNight() const {
	return bIsNight;
}
