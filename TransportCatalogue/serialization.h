#pragma once

#include <fstream>
#include <filesystem>

#include <svg.pb.h>
#include <map_renderer.pb.h>
#include <transport_catalogue.pb.h>
#include <graph.pb.h>
#include <transport_router.pb.h>


#include "transport_catalogue.h"
#include "map_renderer.h"
#include "transport_router.h"

class SerializeManager
{
private:
	std::string save_to;
	std::string read_from;
public:
	
	void SetSaveFile(const std::string& save_to_file);
	void SetReadFile(const std::string& read_from_file);
	void Serialize(const transport_catalogue::TransportCatalogue& tc, const map_renderer::MapRenderer& mr, const transport_router::TransportRouter& tr) const;
	void Deserialize(transport_catalogue::TransportCatalogue& tc, map_renderer::MapRenderer& mr, transport_router::TransportRouter& tr) const;
};