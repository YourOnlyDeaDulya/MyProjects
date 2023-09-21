#include "transport_router.h"

using namespace std;

namespace transport_router
{
	GraphEdgeWeight::GraphEdgeWeight(double time) : time_(time) {}
	GraphEdgeWeight::GraphEdgeWeight(double time, EdgeType type) : time_(time), type_(type) {}
	GraphEdgeWeight::GraphEdgeWeight(double time, EdgeType type, string_view bus_name, size_t span_count)
		: time_(time), type_(type), bus_name_(bus_name), span_count_(span_count) {}

	bool operator>(const GraphEdgeWeight& one, const GraphEdgeWeight& another)
	{
		return one.time_ > another.time_;
	}
	bool operator<(const GraphEdgeWeight& one, const GraphEdgeWeight& another)
	{
		return !(one > another);
	}

	GraphEdgeWeight operator+(const GraphEdgeWeight& one, const GraphEdgeWeight another)
	{
		return GraphEdgeWeight{ one.time_ + another.time_ };
	}


	using Trip = GraphEdgeWeight;

	Trip TransportRouter::ConstructWeight(size_t road_distance, EdgeType type, string_view bus_name, size_t span_count)
	{
		if (type == EdgeType::WAIT)
		{
			return Trip(bus_wait_time_, type);
		}

		return Trip(1.0 * road_distance / bus_velocity_, type, bus_name, span_count);
	}

	EdgeId TransportRouter::AddEdge(size_t from, size_t to, Trip weight)
	{
		return graph_.AddEdge(Edge<Trip>{from, to, weight});
	}

	TransportRouter::TransportRouter(size_t stops_count) : graph_(stops_count * 2) {}

	void TransportRouter::SetVelocity(double velocity)
	{
		bus_velocity_ = velocity * 1000 / 60;
	}

	void TransportRouter::SetWaitTime(double time)
	{
		bus_wait_time_ = time;
	}

	void TransportRouter::ConstructEdgeAndAdd(EdgeSettings sett, const vector<Stop*>& stops, const transport_catalogue::TransportCatalogue& catalogue, size_t& road_distance)
	{
		size_t beginning_stop_wait_end = stops.at(sett.from_)->id_;
		size_t ending_stop_wait_begin = graph_.GetVertexCount() - 1 - stops.at(sett.to_)->id_;
		road_distance += catalogue.GetDistance({ stops.at(sett.to_ - 1), stops.at(sett.to_) });
		Trip stops_trip = ConstructWeight(road_distance, EdgeType::GO, sett.bus_name_, sett.span_count_);
		AddEdge(beginning_stop_wait_end, ending_stop_wait_begin, stops_trip);
	}

	void TransportRouter::BuildGraph(const transport_catalogue::TransportCatalogue& catalogue)
	{
		Trip wait_trip = ConstructWeight(1, EdgeType::WAIT);

		auto all_stops = catalogue.GetStops();
		for (size_t i = 0; i < all_stops.size(); ++i)
		{
			VertexId wait_begin_id = graph_.GetVertexCount() - 1 - all_stops.at(i)->id_;
			VertexId wait_end_id = all_stops.at(i)->id_;
			AddEdge(wait_begin_id, wait_end_id, wait_trip);
		}

		auto buses = catalogue.GetBuses();
		for (Bus* bus : buses)
		{
			const auto& stops = bus->stops_;
			switch (bus->bus_type_)
			{
			case BusType::CIRCULAR:
			{
				for (size_t from = 0; from < stops.size() - 1; ++from)
				{
					size_t road_distance = 0;
					for (size_t to = from + 1; to < stops.size(); ++to)
					{
						EdgeSettings sett{ from, to, to - from, bus->bus_name_ };
						ConstructEdgeAndAdd(sett, stops, catalogue, road_distance);
					}
				}
			}
			break;

			case BusType::LINEAR:
			{
				for (size_t from = 0; from < stops.size() / 2; ++from)
				{
					size_t road_distance = 0;
					for (size_t to = from + 1; to < stops.size() / 2 + 1; ++to)
					{
						EdgeSettings sett{ from, to, to - from, bus->bus_name_ };
						ConstructEdgeAndAdd(sett, stops, catalogue, road_distance);
					}
				}

				for (size_t from = stops.size() / 2; from < stops.size() - 1; ++from)
				{
					size_t road_distance = 0;
					for (size_t to = from + 1; to < stops.size(); ++to)
					{
						EdgeSettings sett{ from, to, to - from, bus->bus_name_ };
						ConstructEdgeAndAdd(sett, stops, catalogue, road_distance);
					}
				}
			}
			break;
			}
		}
	}

	void TransportRouter::BuildRouter()
	{
		router_ = make_unique<Router<Trip>>(graph_);
	}

	Edge<Trip> TransportRouter::GetEdge(EdgeId id) const
	{
		return graph_.GetEdge(id);
	}

	std::optional<Router<Trip>::RouteInfo> TransportRouter::BuildRoute(VertexId from, VertexId to) const
	{
		VertexId from_wait_id = graph_.GetVertexCount() - 1 - from;
		VertexId to_wait_id = graph_.GetVertexCount() - 1 - to;
		return router_->BuildRoute(from_wait_id, to_wait_id);
	}

	double TransportRouter::GetVelocity() const
	{
		return bus_velocity_;
	}

	double TransportRouter::GetWaitTime() const
	{
		return bus_wait_time_;
	}

	graph::DirectedWeightedGraph<Trip>& TransportRouter::GetGraph()
	{
		return graph_;
	}

	Router<Trip>& TransportRouter::GetRouter()
	{
		return *router_;
	}
	const graph::DirectedWeightedGraph<Trip>& TransportRouter::GetGraph() const
	{
		return graph_;
	}

	const Router<Trip>& TransportRouter::GetRouter() const
	{
		return *router_;
	}
	void TransportRouter::SetGraph(DirectedWeightedGraph<Trip>&& graph)
	{
		graph_ = graph;
	}
	void TransportRouter::BuildRawRouter(RID_Data&& rid)
	{
		router_ = make_unique<Router<Trip>>(graph_, move(rid));
	}
}