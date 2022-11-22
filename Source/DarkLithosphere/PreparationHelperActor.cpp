
#include "PreparationHelperActor.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"


APreparationHelperActor::APreparationHelperActor() {
	PrimaryActorTick.bCanEverTick = true;

}

bool CheckSaveDirLocal(FString SaveDir) {
	UE_LOG(LogTemp, Log, TEXT("Check directory: %s"), *SaveDir);
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*SaveDir)) {
		PlatformFile.CreateDirectory(*SaveDir);
		UE_LOG(LogTemp, Log, TEXT("Create directory: %s"), *SaveDir);
		if (!PlatformFile.DirectoryExists(*SaveDir)) {
			UE_LOG(LogTemp, Warning, TEXT("Unable to create directory: %s"), *SaveDir);
			return false;
		}
	}

	return true;
}

FString GetVersionString();

void APreparationHelperActor::BeginPlay()
{
	Super::BeginPlay();

	// TODO finish
	FString MapName = TEXT("World 0");
	FString SavePath = FPaths::ProjectSavedDir();
	FString SaveDir = SavePath + TEXT("Map/");

	if (!CheckSaveDirLocal(SaveDir)) {
		// log error
	}

	FString SaveDirWorld0 = SaveDir + MapName + TEXT("/");
	if (!CheckSaveDirLocal(SaveDirWorld0)) {
		// log error
	}


	FString Url = TEXT("http://192.168.1.109:8080/api/v1/lastversion?v=") + GetVersionString();
	FHttpModule& HttpModule = FHttpModule::Get();
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> RestRequest = HttpModule.CreateRequest();
	RestRequest->SetVerb(TEXT("GET"));
	//pRequest->SetHeader(TEXT("Content-Type"), TEXT("application/x-www-form-urlencoded"));
	//FString RequestContent = TEXT("identity=") + NewUser + TEXT("&password=") + NewPassword + TEXT("&query=") + uriQuery;
	//pRequest->SetContentAsString(RequestContent);

	RestRequest->SetURL(Url);
	RestRequest->OnProcessRequestComplete().BindLambda(
		[&](FHttpRequestPtr Request, FHttpResponsePtr Response, bool connectedSuccessfully) mutable {
				if (connectedSuccessfully) {
					FString ResponseString = Response->GetContentAsString();
					UE_LOG(LogTemp, Warning, TEXT("%s"), *ResponseString);
				} else {
					switch (Request->GetStatus()) {
					case EHttpRequestStatus::Failed_ConnectionError:
						UE_LOG(LogTemp, Error, TEXT("Connection failed."));
					default:
						UE_LOG(LogTemp, Error, TEXT("Request failed."));
					}
				}
		});

	RestRequest->ProcessRequest();
}

void APreparationHelperActor::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

}

