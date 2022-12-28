#pragma once

#include "CoreMinimal.h"
#include <functional>
#include <string>
#include <unordered_map>


class TNotificationHelper {

public:

	static void AddObserver(std::string ObjectName, std::string NotificationName, std::function<void()> Function);

	static void RemoveObserver(std::string ObjectName);

	static void SendNotification(std::string ObjectName);

private:

	static std::unordered_map<std::string, std::unordered_map<std::string, std::function<void()>>> ObserverMap;

};

