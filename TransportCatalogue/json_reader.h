#pragma once

#include "transport_catalogue.h"
#include "json.h"
#include "request_handler.h"
#include "map_renderer.h"
#include "json_builder.h"
#include "transport_router.h"
#include "serialization.h"

#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <optional>

class RequestStorage
{
public:
	explicit RequestStorage(transport_catalogue::TransportCatalogue& catalogue, map_renderer::MapRenderer& renderer, std::istream& in);

	void ReadInputRequests() const;
	void SetSerializeToFile();
	void SetDeserializeFrom();
	void ReadRendererSettings() const;
	void ReadOutputRequests(std::ostream& out = std::cout) const;
	void RenderMap(std::ostream& out = std::cout) const;
	void ReadRouterSettingsAndBuild();
	void Serialize() const;
	void Deserialize();
private:

	void ReadBaseRequests(const json::Node& base_node) const;
	void ReadStatRequests(const json::Node& stat_node, std::ostream& out) const;

	json::Document storage_;
	request_handler::RequestHandler rh_;
	SerializeManager serializer_;
};