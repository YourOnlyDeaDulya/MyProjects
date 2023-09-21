#include "transport_catalogue.h"


using namespace std;
using namespace domain;

namespace transport_catalogue
{
	void TransportCatalogue::AddStop(string_view stop_name, double lat, double lng, const vector<pair<string, size_t>>& distances)
	{
		AddStop(move(Stop(string(stop_name), lat, lng)), distances);
	}

	void TransportCatalogue::AddStop(const Stop& stop, const vector<pair<string, size_t>>& distances)
	{
		Stop* begin_stop = nullptr;

		if (stopname_to_address_.count(stop.stop_name_))
		{
			begin_stop = FindStop(stop.stop_name_);
			size_t stop_id = begin_stop->id_;
			*begin_stop = stop;
			begin_stop->id_ = stop_id;
		}
		else
		{
			size_t stop_id = stop_vault_.size();
			auto pos = stop_vault_.insert(stop_vault_.end(), move(stop));
			stopname_to_address_[pos->stop_name_] = &(*pos);
			pos->id_ = stop_id;
			stop_id_to_adress_[stop_id] = &(*pos);

			begin_stop = &(*pos);
		}

		if (distances.empty())
		{
			return;
		}

		for (const auto& [end_stop_name, meters] : distances)
		{
			Stop* end_stop = nullptr;

			if (!stopname_to_address_.count(end_stop_name))
			{
				size_t stop_id = stop_vault_.size();
				auto pos = stop_vault_.insert(stop_vault_.end(),
					move(Stop(end_stop_name, 0, 0, stop_id)));
				stopname_to_address_[pos->stop_name_] = &(*pos);
				stop_id_to_adress_[stop_id] = &(*pos);

				end_stop = &(*pos);
			}
			else
			{
				end_stop = FindStop(end_stop_name);
			}



			if (!stop_to_stop_distance_.count({ begin_stop, end_stop }))
			{
				stop_to_stop_distance_.insert({ { begin_stop, end_stop }, meters });
			}
			else
			{
				stop_to_stop_distance_[{begin_stop, end_stop}] = meters;
			}

			if (!stop_to_stop_distance_.count({ end_stop, begin_stop }))
			{
				stop_to_stop_distance_.insert({ { end_stop, begin_stop }, meters });
			}
		}




	}

	double TransportCatalogue::CalculateBusLength(const vector<Stop*>& stops) const
	{
		double length = 0;

		for (size_t i = 0; i < stops.size() - 1; ++i)
		{
			length += stop_to_stop_distance_.at({ stops[i], stops[i + 1] });
		}

		return length;
	}

	double TransportCatalogue::CalculateGeoLength(const vector<Stop*>& stops) const
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

	size_t TransportCatalogue::CalculateUniqueStops(const vector<Stop*>& stops) const
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

	void TransportCatalogue::AddBus(string_view bus_name, const vector<string>& stops, BusType type = BusType::CIRCULAR)
	{
		size_t bus_id = bus_vault_.size();
		auto pos = bus_vault_.insert(bus_vault_.end(), move(Bus(string(bus_name), {}, bus_id, type)));
		busname_to_address_[pos->bus_name_] = &(*pos);
		bus_id_to_adress_[pos->id_] = &(*pos);

		switch (type)
		{
		case BusType::CIRCULAR:
			pos->stops_.resize(stops.size());
			transform(stops.begin(), stops.end(), pos->stops_.begin(), [&](string_view stop_name)
				{
					Stop* s = stopname_to_address_.at(stop_name);
					if (!s->buses_.count(pos->bus_name_))
					{
						s->buses_.insert(pos->bus_name_);
					}
					return s;
				});
			break;

		case BusType::LINEAR:
			pos->stops_.resize(stops.size() * 2 - 1);
			auto transform_it = transform(stops.begin(), stops.end(), pos->stops_.begin(), [&](string_view stop_name)
				{
					Stop* s = stopname_to_address_.at(stop_name);
					if (!s->buses_.count(pos->bus_name_))
					{
						s->buses_.insert(pos->bus_name_);
					}
					return s;
				});
			transform(stops.rbegin() + 1, stops.rend(), transform_it, [&](string_view stop_name)
				{
					return stopname_to_address_.at(stop_name);
				});
			break;
		}
	}

	void TransportCatalogue::AddBus(const Bus& bus)
	{
		size_t id = bus_vault_.size();
		auto pos = bus_vault_.insert(bus_vault_.end(), move(bus));
		busname_to_address_[pos->bus_name_] = &(*pos);
		pos->id_ = id;
		bus_id_to_adress_[pos->id_] = &(*pos);

		for_each(pos->stops_.begin(), pos->stops_.end(), [&](Stop* s)
			{
				if (!s->buses_.count(pos->bus_name_))
				{
					s->buses_.insert(pos->bus_name_);
				}
			});

	}

	void TransportCatalogue::SetRawDistance(std::pair<Stop*, Stop*> stops, size_t distance)
	{
		stop_to_stop_distance_[stops] = distance;
	}

	StopData TransportCatalogue::StopInfo(string_view searched_stop) const
	{
		Stop* stop = FindStop(searched_stop);
		if (!stop)
		{
			return { StopInfoType::NOT_FOUND, string(searched_stop), {} };
		}

		else if (stop->buses_.empty())
		{
			return { StopInfoType::NO_BUSES, string(searched_stop), {} };
		}

		std::vector<string_view> buses;
		for (auto it = stop->buses_.begin(); it != stop->buses_.end(); ++it)
		{
			buses.push_back(*it);
		}
		std::sort(buses.begin(), buses.end());

		return { StopInfoType::FOUND, string(searched_stop), move(buses) };
	}

	BusData TransportCatalogue::BusInfo(string_view searched_bus) const
	{
		Bus* bus = FindBus(searched_bus);
		if (!bus)
		{
			return { BusInfoType::NOT_FOUND, string(searched_bus), 0, 0, 0, 0 };
		}

		const auto& stops = bus->stops_;

		double bus_length = CalculateBusLength(stops);
		double curvature = bus_length / CalculateGeoLength(stops);
		size_t unique_stops = CalculateUniqueStops(stops);


		return { BusInfoType::FOUND, bus->bus_name_, stops.size(), unique_stops, bus_length, curvature };
	}

	size_t TransportCatalogue::StopsCount() const noexcept
	{
		return stop_vault_.size();
	}

	size_t TransportCatalogue::BusCount() const noexcept
	{
		return bus_vault_.size();
	}

	Bus* TransportCatalogue::FindBus(string_view searched_bus) const
	{
		if (!busname_to_address_.count(searched_bus))
		{
			return nullptr;
		}

		return busname_to_address_.at(searched_bus);
	}

	Bus* TransportCatalogue::FindBus(size_t bus_id) const
	{
		if (!bus_id_to_adress_.count(bus_id))
		{
			return nullptr;
		}

		return bus_id_to_adress_.at(bus_id);
	}

	Stop* TransportCatalogue::FindStop(string_view searched_stop) const
	{
		if (!stopname_to_address_.count(searched_stop))
		{
			return nullptr;
		}

		return stopname_to_address_.at(searched_stop);
	}

	Stop* TransportCatalogue::FindStop(size_t stop_id) const
	{
		if(!stop_id_to_adress_.count(stop_id))
		{
			return nullptr;
		}

		return stop_id_to_adress_.at(stop_id);
	}

	size_t TransportCatalogue::TestMetersCount(Stop* a, Stop* b)
	{
		return stop_to_stop_distance_.at({ a, b });
	}

	std::vector<std::string> TransportCatalogue::GetBusesNameList() const
	{
		vector<string> buses(busname_to_address_.size());
		transform(busname_to_address_.begin(), busname_to_address_.end(), buses.begin(), [&](const auto& p)
			{
				return string(p.first);
			});
		return buses;
	}

	std::vector<Bus*> TransportCatalogue::GetBuses() const
	{
		vector<Bus*> buses(busname_to_address_.size());
		transform(busname_to_address_.begin(), busname_to_address_.end(), buses.begin(), [&](const auto& p)
			{
				return p.second;
			});
		return buses;
	}

	std::vector<Stop*> TransportCatalogue::GetStops() const
	{
		vector<Stop*> stops(stopname_to_address_.size());
		transform(stopname_to_address_.begin(), stopname_to_address_.end(), stops.begin(), [&](const auto& p)
			{
				return p.second;
			});
		return stops;
	}

	size_t TransportCatalogue::GetDistance(std::pair<Stop*, Stop*> stops) const
	{
		return stop_to_stop_distance_.at(stops);
	}

	const std::unordered_map <std::pair<Stop*, Stop*>, size_t, PairOfStopsHasher>& TransportCatalogue::GetDistances() const
	{
		return stop_to_stop_distance_;
	}
}
