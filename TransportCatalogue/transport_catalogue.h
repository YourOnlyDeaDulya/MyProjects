#pragma once

#include <deque>
#include <unordered_map>
#include <string_view>
#include <algorithm>
#include <map>
#include <vector>
#include <iostream>
#include <unordered_set>
#include <string>
#include <exception>

#include "domain.h"


using namespace domain;

namespace transport_catalogue
{
	class TransportCatalogue
	{

	public:

		TransportCatalogue() = default;
		void AddStop(std::string_view stop_name, double lat, double lng, const std::vector<std::pair<std::string, size_t>>& distances);
		void AddStop(const Stop& stop, const std::vector<std::pair<std::string, size_t>>& distances);
		void AddBus(std::string_view bus_name, const std::vector<std::string>& stops, BusType type);
		void AddBus(const Bus& bus);
		void SetRawDistance(std::pair<Stop*, Stop*> stops, size_t distance);
		StopData StopInfo(std::string_view searched_stop) const;
		BusData BusInfo(std::string_view searched_bus) const;
		size_t StopsCount() const noexcept;
		size_t BusCount() const noexcept;
		size_t TestMetersCount(Stop* a, Stop* b);
		double CalculateBusLength(const std::vector<Stop*>& stops) const;
		double CalculateGeoLength(const std::vector<Stop*>& stops) const;
		size_t CalculateUniqueStops(const std::vector<Stop*>& stops) const;
		Bus* FindBus(std::string_view searched_bus) const;
		Bus* FindBus(size_t bus_id) const;
		Stop* FindStop(std::string_view searched_stop) const;
		Stop* FindStop(size_t stop_id) const;
		std::vector<std::string> GetBusesNameList() const;
		std::vector<Bus*> GetBuses() const;
		std::vector<Stop*> GetStops() const;
		size_t GetDistance(std::pair<Stop*, Stop*> stops) const;
		const std::unordered_map <std::pair<Stop*, Stop*>, size_t, PairOfStopsHasher>& GetDistances() const;

	private:
		
		std::deque<Stop> stop_vault_;
		std::deque<Bus> bus_vault_;
		std::unordered_map<std::string_view, Stop*> stopname_to_address_;
		std::unordered_map<std::string_view, Bus*> busname_to_address_;
		std::unordered_map<size_t, Stop*> stop_id_to_adress_;
		std::unordered_map<size_t, Bus*> bus_id_to_adress_;
		std::unordered_map <std::pair<Stop*, Stop*>, size_t, PairOfStopsHasher> stop_to_stop_distance_;
	};
}
