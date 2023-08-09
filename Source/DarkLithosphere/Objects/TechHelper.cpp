

#include "TechHelper.h"
#include "ElectricDevice.h"

#include <unordered_map>
#include <unordered_set>
#include <list>


struct TEnergyNetNodeParam {

	double x, y, z;

	int type = 0;

	float max_distance;
};


struct TEnergyNetNodeState {

	int state = 0;

	float val = 0;
};


struct TEnergyNetNode {

	int id = 0;

	std::unordered_set<int> in;

	std::unordered_set<int> out;

	TEnergyNetNodeParam param;

	int autolink = 1;

	int flag_active = 0;

};


class TEnergyNet {

private:

	int id_counter = 1;

	std::unordered_map<int, TEnergyNetNode> net;

public: 

	void clear() {
		id_counter = 1;
		net.clear();
	};

	int create_node() {
		int new_id = id_counter;
		net[new_id].id = new_id;
		id_counter++;
		return new_id;
	};

	void delete_node(int id) {
		net.erase(id);
	};

	void set_node_flag_active(int id, int flag_active) {
		net[id].flag_active = flag_active;
	};

	void set_node_param(int id, const TEnergyNetNodeParam& param) {
		net[id].param = param;
	};

	int get_node_parent(int id) {
		int parent_id = 0;

		if(net[id].id != 0){
			const auto& t = net[id];
			if (t.in.size() > 0) {
				for (int res : t.in) {
					return res;
				}
			}
		}
		
		return parent_id;
	};

	void get_node_pos(int id, double& x, double& y, double& z) {
		int parent_id = 0;

		if (net[id].id != 0) {
			const auto t = net[id].param;
			x = t.x;
			y = t.y;
			z = t.z;
		}
	};

	int find_nearest_node(int id) {
		const auto& source_node = net[id];

		if (source_node.id == 0) {
			return 0;
		}

		double min_len = 9999999.f;
		int target_id = 0;

		const FVector source_node_v(source_node.param.x, source_node.param.y, source_node.param.z);

		for (const auto& itm : net) {
			const auto& n = itm.second;

			if (n.param.type == 0) {
				continue;
			}

			if (n.id == id) {
				continue;
			}

			if (n.in.contains(id)) {
				continue;
			}

			bool connect = false;

			if (source_node.param.type == ED_TYPE_TARGET && n.param.type == ED_TYPE_ENDPOINT){
				connect = true;
			}

			if (source_node.param.type == ED_TYPE_ENDPOINT && n.param.type == ED_TYPE_TRANSMITER) {
				connect = true;
			}

			if (source_node.param.type == ED_TYPE_TRANSMITER && n.param.type == ED_TYPE_TRANSMITER) {
				connect = true;
			}

			if (source_node.param.type == ED_TYPE_TRANSMITER && n.param.type == ED_TYPE_SOURCE) {
				connect = true;
			}

			if (connect) {
				double len = FVector::Dist(source_node_v, FVector(n.param.x, n.param.y, n.param.z));
				if (len < min_len && len < source_node.param.max_distance) {
					min_len = len;
					target_id = n.id;
				}
			}
		}

		return target_id;
	};

	void rebuild() {
		for (auto& itm : net) {
			auto& node = itm.second;
			node.in.clear();
			node.out.clear();
		}

		for (auto& itm : net) {
			auto& node = itm.second;
			if (node.autolink == 1) {
				int nearest_node_id = find_nearest_node(node.id);
				node.in.insert(nearest_node_id);

				auto& t = net[nearest_node_id];
				t.out.insert(node.id);
			}
		}
	};

	std::unordered_map<int, float> pass() {

		std::list<int> list;
		std::unordered_map<int, float> net_node_val;

		for (auto& itm : net) {
			auto& node = itm.second;
			if (node.param.type == ED_TYPE_SOURCE) {
				list.push_back(node.id);
				//UE_LOG(LogTemp, Log, TEXT("source -> %d"), node.id);

				const float source_val = 1.f; // TODO
				net_node_val.insert({ node.id, source_val });
			}
		}

		const float r = 0.f;

		while (list.size() > 0) {
			int node_id = list.front();
			list.pop_front();

			//UE_LOG(LogTemp, Log, TEXT("node -> %d"), node_id);

			auto& node = net[node_id];
			float val = 0;

			if (node.flag_active == 1) {
				val = net_node_val[node_id];
			}

			for (int out_node_id : node.out) {
				if (out_node_id > 0) {
					list.push_back(out_node_id);
					net_node_val[out_node_id] = val - r;
				}
			}
		}

		return net_node_val;
	};



};


static TEnergyNet* EnergyNet;


ATechHelper::ATechHelper() {
	PrimaryActorTick.bCanEverTick = true;

	if (!EnergyNet) {
		EnergyNet = new TEnergyNet();
	}
}

void ATechHelper::FinishDestroy() {
	Super::FinishDestroy();

	EnergyNet->clear();
}

void ATechHelper::BeginPlay() {
	Super::BeginPlay();

}


void ATechHelper::EndPlay(const EEndPlayReason::Type EndPlayReason) {
	Super::EndPlay(EndPlayReason);
}

void ATechHelper::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);
}

void ATechHelper::RegisterElectricDevice(AElectricDevice* DeviceActor) {
	if (GetNetMode() == NM_Client) {
		return;
	}

	FString Name = DeviceActor->GetName();

	if (!ElectricDeviceMap.Contains(Name)) {
		int DevId = EnergyNet->create_node();
		ElectricDeviceMap.Add(Name, { DeviceActor, DevId });

		UE_LOG(LogTemp, Log, TEXT("RegisterElectricDevice -> %s -> %d"), *Name, DevId);

		const FVector Pos = DeviceActor->GetActorLocation();
		EnergyNet->set_node_param(DevId, { Pos.X, Pos.Y, Pos.Z, DeviceActor->GetElectricDeviceType(),  DeviceActor->ElectricLinkDistance });

		int p = EnergyNet->find_nearest_node(DevId);
		UE_LOG(LogTemp, Log, TEXT("find_nearest_node -> %d"), p);

	} else {
		// TODO handle error
	}
}

void ATechHelper::SetActiveElectricDevice(FString Name, int FlagActive) {
	if (GetNetMode() == NM_Client) {
		return;
	}

	if (ElectricDeviceMap.Contains(Name)) {
		const auto& Itm = ElectricDeviceMap[Name];
		const int DevId = Itm.Id;
		EnergyNet->set_node_flag_active(DevId, FlagActive);
	} else {
		// TODO handle error
	}

	RebuildEnergyNet();
}

void ATechHelper::UnregisterElectricDevice(AElectricDevice* DeviceActor) {
	if (GetNetMode() == NM_Client) {
		return;
	}

	FString Name = DeviceActor->GetName();

	if (ElectricDeviceMap.Contains(Name)) {
		const auto& Itm = ElectricDeviceMap[Name];
		const int DevId = Itm.Id;
		EnergyNet->delete_node(DevId);
		ElectricDeviceMap.Remove(Name);

		UE_LOG(LogTemp, Log, TEXT("UnregisterElectricDevice -> %s -> %d"), *Name, DevId);
	} else {
		// TODO handle error
	}

	RebuildEnergyNet();
}

void ATechHelper::RebuildEnergyNet() {
	double Start = FPlatformTime::Seconds();

	EnergyNet->rebuild();
	auto Net = EnergyNet->pass();

	for (const auto& Itm : ElectricDeviceMap) {
		const FString Name = Itm.Key;
		const auto& DevProxy = Itm.Value;
		const float F = Net[DevProxy.Id];

		//UE_LOG(LogTemp, Log, TEXT("device val = %s -- %d -> %f"), *Name, DevProxy.Id, F);

		int NewState = (F > 0) ? 1 : 0;

		//if (NewState != DevProxy.DeviceActor->GetElectricDeviceServerState()) {
			//UE_LOG(LogTemp, Log, TEXT("enable/disable device = %s - %f - %d - %d "), *Name, F, NewState, DevProxy.DeviceActor->GetElectricDeviceServerState());
			DevProxy.DeviceActor->SetElectricDeviceServerState(NewState);
		//}
	}

	double End = FPlatformTime::Seconds();
	double Time = (End - Start) * 1000;
	UE_LOG(LogTemp, Log, TEXT("RebuildEnergyNet --> %f ms"), Time);

	//DrawDebugEnergyNet();
}

void ATechHelper::DrawDebugEnergyNet() {
	for (const auto& Itm : ElectricDeviceMap) {
		const FString Name = Itm.Key;
		const auto& DevProxy = Itm.Value;
		const int P = EnergyNet->get_node_parent(DevProxy.Id);

		//UE_LOG(LogTemp, Log, TEXT("device = %s -- %d -> %d"), *Name, DevProxy.Id, P);

		if (P != 0) {
			double X1 = 0;
			double X2 = 0;

			double Y1 = 0;
			double Y2 = 0;

			double Z1 = 0;
			double Z2 = 0;

			EnergyNet->get_node_pos(DevProxy.Id, X1, Y1, Z1);
			EnergyNet->get_node_pos(P, X2, Y2, Z2);

			DrawDebugLine(GetWorld(), FVector(X1, Y1, Z1), FVector(X2, Y2, Z2), FColor(255, 255, 255, 0), false, 5, 0, 2);
		}
	}
}
