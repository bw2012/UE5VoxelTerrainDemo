// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"


class FDarkLithosphereGameModule : public IModuleInterface {

public:

    virtual void StartupModule() override;

    virtual void ShutdownModule() override;

    virtual bool IsGameModule() const override {
        return true;
    }

};