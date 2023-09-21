#pragma once

#include <vector>
#include <string>
#include <string_view>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>

#include "geo.h"


using namespace geo;

namespace domain
{
	enum class BusType
	{
		LINEAR,
		CIRCULAR,
	};

	enum class StopInfoType
	{
		FOUND,
		NO_BUSES,
		NOT_FOUND,
	};

	enum class BusInfoType
	{
		FOUND,
		NOT_FOUND,
	};

	struct StopData
	{
		StopInfoType info_type_;
		std::string stop_name_;
		std::vector<std::string_view> stop_buses_;
	};

	struct BusData
	{
		BusInfoType info_type_;
		std::string bus_name_;
		size_t stop_count_;
		size_t unique_stops_;
		double bus_length_;
		double curvature_;
	};

	struct Stop
	{
		Stop() = default;
		explicit Stop(std::string_view name, double lat, double lng, size_t id = 0);
		Stop& operator=(const Stop& other);
		Stop(const Stop& other);
		bool operator==(const Stop& other) const;
		std::string stop_name_;
		double lat_;
		double lng_;
		size_t id_;
		std::unordered_set<std::string_view> buses_;
	};

	struct Bus
	{
		Bus() = default;
		explicit Bus(std::string_view name, std::vector<Stop*>&& stops, size_t id, BusType type = BusType::CIRCULAR);
		bool operator==(const Bus& other) const;

		std::string bus_name_;
		std::vector<Stop*> stops_;
		size_t id_;
		BusType bus_type_;
	};

	class PairOfStopsHasher
	{
	public:
		size_t operator()(const std::pair<Stop*, Stop*> p) const;
	};

	double CalculateBusLength(const std::vector<Stop*>& stops, const std::unordered_map<std::pair<Stop*, Stop*>, size_t, PairOfStopsHasher> stop_to_stop_distance);
	double CalculateGeoLength(const std::vector<Stop*>& stops);
	size_t CalculateUniqueStops(const std::vector<Stop*>& stops);
}