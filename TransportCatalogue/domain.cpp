#include "domain.h"


using namespace std;

namespace domain
{
	Stop::Stop(string_view name, double lat, double lng, size_t id)
		: stop_name_(string(name)), lat_(lat), lng_(lng), id_(id)
	{
	}

	Stop& Stop::operator=(const Stop& other)
	{
		if (this != &other) {
			stop_name_ = other.stop_name_;
			lat_ = other.lat_;
			lng_ = other.lng_;
			buses_ = other.buses_;
			id_ = other.id_;
		}

		return *this;
	}

	Stop::Stop(const Stop& other)
	{
		*this = other;
	}

	bool Stop::operator==(const Stop& other) const
	{
		return stop_name_ == other.stop_name_ &&
			lat_ == other.lat_ &&
			lng_ == other.lng_ &&
			buses_ == other.buses_;
	}

	Bus::Bus(string_view name, vector<Stop*>&& stops, size_t id, BusType type)
		: bus_name_(string(name)), stops_(move(stops)), id_(id), bus_type_(type)
	{
	}

	bool Bus::operator==(const Bus& other) const
	{
		return bus_name_ == other.bus_name_ &&
			bus_type_ == other.bus_type_ &&
			stops_ == other.stops_;
	}

	size_t PairOfStopsHasher::operator()(const pair<Stop*, Stop*> p) const
	{
		size_t hasher1 = p.first->stop_name_.size() * 11 + p.first->stop_name_[0] * 11 * 11;
		size_t hasher2 = p.second->stop_name_.size() * 11 * 11 + p.second->stop_name_[0] * 11 * 11 * 11 * 11;
		return hasher1 + hasher2;
	}

	double CalculateBusLength(const vector<Stop*>& stops, const unordered_map<std::pair<Stop*, Stop*>, size_t, PairOfStopsHasher> stop_to_stop_distance)
	{
		double length = 0;

		for (size_t i = 0; i < stops.size() - 1; ++i)
		{
			length += stop_to_stop_distance.at({ stops[i], stops[i + 1] });
		}

		return length;
	}

	double CalculateGeoLength(const vector<Stop*>& stops)
	{
		double bus_length = 0;
		vector<geo::Coordinates> coord(stops.size());
		transform(stops.begin(), stops.end(), coord.begin(), [](Stop* stop)
			{
				return geo::Coordinates{ stop->lat_, stop->lng_ };
			});

		for (size_t i = 0; i < coord.size() - 1; ++i)
		{
			bus_length += (geo::ComputeDistance(coord[i], coord[i + 1]));
		}

		return bus_length;
	}

	size_t CalculateUniqueStops(const vector<Stop*>& stops)
	{
		unordered_set<string_view> handled_stops;
		size_t unique_stops = count_if(stops.begin(), stops.end(), [&handled_stops](Stop* stop)
			{
				if (handled_stops.count(stop->stop_name_))
				{
					return false;
				}
				handled_stops.insert(stop->stop_name_);
				return true;
			});

		return unique_stops;
	}
}