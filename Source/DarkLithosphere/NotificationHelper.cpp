
#include "NotificationHelper.h"

std::unordered_map<std::string, std::unordered_map<std::string, std::function<void()>>> TNotificationHelper::ObserverMap;

void TNotificationHelper::AddObserver(std::string ObjectName, std::string NotificationName, std::function<void()> Function) {
	FString Test(ObjectName.c_str());
	UE_LOG(LogTemp, Warning, TEXT("TEST -> %s"), *Test);

	auto& M = ObserverMap[ObjectName];
	M[NotificationName] = Function;
}

void TNotificationHelper::RemoveObserver(std::string ObjectName) {
	ObserverMap.erase(ObjectName);
}

void TNotificationHelper::SendNotification(std::string NotificationName) {
	for (const auto& P : ObserverMap) {
		const auto& Map = P.second;
		auto Itr = Map.find(NotificationName);
		if (Itr != Map.end()) {
			auto Function = Itr->second;
			Function();
		}
	}
}