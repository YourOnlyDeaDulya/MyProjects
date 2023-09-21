#pragma once

#include "graph.h"
#include "router.h"
#include "transport_catalogue.h"
#include <memory>

#include <optional>
#include <unordered_map>

using namespace graph;

namespace transport_router
{
	enum class EdgeType
	{
		WAIT,
		GO
	};

	struct GraphEdgeWeight
	{
		GraphEdgeWeight() = default;
		GraphEdgeWeight(double time);
		GraphEdgeWeight(double time, EdgeType type);
		GraphEdgeWeight(double time, EdgeType type, std::string_view bus_name, size_t span_count);

		double time_ = 0.0;
		EdgeType type_ = EdgeType::GO;
		std::string_view bus_name_ = "";
		size_t span_count_ = 0;
	};

	struct EdgeSettings
	{
		size_t from_;
		size_t to_;
		size_t span_count_;
		std::string_view bus_name_;
	};

	bool operator>(const GraphEdgeWeight& one, const GraphEdgeWeight& another);
	bool operator<(const GraphEdgeWeight& one, const GraphEdgeWeight& another);

	GraphEdgeWeight operator+(const GraphEdgeWeight& one, const GraphEdgeWeight another);

	class TransportRouter
	{

	private:

		using Trip = GraphEdgeWeight;
		using RID_Data = std::vector<std::vector<std::optional<graph::Router<Trip>::RID>>>;
		DirectedWeightedGraph<Trip> graph_;
		std::unique_ptr<Router<Trip>> router_;

		double bus_velocity_ = 1.0;
		double bus_wait_time_ = 0.0;

		Trip ConstructWeight(size_t road_distance, EdgeType type = EdgeType::GO, std::string_view bus_name = "", size_t span_count = 0);
		EdgeId AddEdge(size_t from, size_t to, Trip trip);
		void ConstructEdgeAndAdd(EdgeSettings sett, const std::vector<Stop*>& stops, const transport_catalogue::TransportCatalogue& catalogue, size_t& road_distance);
	public:

		explicit TransportRouter(size_t stops_count);
		TransportRouter() = default;
		void SetVelocity(double velocity);
		void SetWaitTime(double time);
		double GetVelocity() const;
		double GetWaitTime() const;
		DirectedWeightedGraph<Trip>& GetGraph();
		Router<Trip>& GetRouter();
		const DirectedWeightedGraph<Trip>& GetGraph() const;
		const Router<Trip>& GetRouter() const;
		void BuildGraph(const transport_catalogue::TransportCatalogue& catalogue);
		void SetGraph(DirectedWeightedGraph<Trip>&& graph);
		void BuildRawRouter(RID_Data&& rid);
		void BuildRouter();
		Edge<Trip> GetEdge(EdgeId id) const;
		std::optional<Router<Trip>::RouteInfo> BuildRoute(VertexId from, VertexId to) const;
	};
}