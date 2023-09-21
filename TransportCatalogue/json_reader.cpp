#include "json_reader.h"


using namespace std;
using namespace domain;

RequestStorage::RequestStorage(transport_catalogue::TransportCatalogue& catalogue, map_renderer::MapRenderer& renderer, istream& in)
	: storage_(json::Load(in)), rh_(catalogue, renderer)
{
}

//Считываем запросы на пополнение базы
void RequestStorage::ReadInputRequests() const
{
	const json::Node& base_node = storage_.GetRoot().AsDict().at("base_requests");
	ReadBaseRequests(base_node);
}

void RequestStorage::SetSerializeToFile()
{
	const json::Node& save_to_node = storage_.GetRoot().AsDict().at("serialization_settings");
	serializer_.SetSaveFile(save_to_node.AsDict().at("file").AsString());
}

void RequestStorage::SetDeserializeFrom()
{
	const json::Node& save_to_node = storage_.GetRoot().AsDict().at("serialization_settings");
	serializer_.SetReadFile(save_to_node.AsDict().at("file").AsString());
}

//Считываем запросы на вывод информации
void RequestStorage::ReadOutputRequests(std::ostream& out) const
{
	const json::Node& stat_node = storage_.GetRoot().AsDict().at("stat_requests");
	ReadStatRequests(stat_node, out);
}

//Рендерим карту в нужный поток
void RequestStorage::RenderMap(std::ostream& out) const
{
	svg::Document render = rh_.RenderMap();
	render.Render(out);
}

//Считываем настройки MapRenderer

//Можно поместить этот метод внутрь RH
void RequestStorage::ReadRendererSettings() const
{
	rh_.ReadRendererSettings(storage_);
}

void RequestStorage::ReadRouterSettingsAndBuild()
{
	const json::Dict& router_node = storage_.GetRoot().AsDict().at("routing_settings").AsDict();
	rh_.ReadRouterSettingsAndBuild(router_node);
}
//Считываем из Node типы запросов на пополнение базы
void RequestStorage::ReadBaseRequests(const json::Node& base_node) const
{
	const auto& base_array = base_node.AsArray();
	for_each(base_array.begin(), base_array.end(), [&](const json::Node& node)
		{
			const auto& value = node.AsDict();
			if(value.at("type").AsString() == "Stop")
			{
				rh_.LoadStopRequest(node);
			}
		});

	for_each(base_array.begin(), base_array.end(), [&](const json::Node& node)
		{
			const auto& value = node.AsDict();
			if (value.at("type").AsString() == "Bus")
			{
				rh_.LoadBusRequest(node);
			}
		});
}

//Считываем из Node типы запросов на вывод из базы
void RequestStorage::ReadStatRequests(const json::Node& stat_node, std::ostream& out) const

{
	const auto& stat_arr = stat_node.AsArray();
	json::Builder builder;
	builder.StartArray();
	for_each(stat_arr.begin(), stat_arr.end(), [&](const json::Node& node)
		{
			const auto& stat_dict = node.AsDict();
			auto id = stat_dict.at("id").AsInt();

			if (stat_dict.at("type").AsString() == "Stop")
			{
				auto stop_info = rh_.GetBusesByStop(stat_dict.at("name").AsString());
				rh_.StopRequestNode(builder, stop_info, id);
			}

			else if (stat_dict.at("type").AsString() == "Bus")
			{
				auto bus_info = rh_.GetBusStat(stat_dict.at("name").AsString());
				rh_.BusRequestNode(builder, bus_info, id);
			}

			else if (stat_dict.at("type").AsString() == "Map")
			{
				ostringstream out;
				RenderMap(out);
				builder.StartDict().Key("map").Value(out.str()).Key("request_id").Value(stat_dict.at("id").AsInt()).EndDict();
			}
			else if(stat_dict.at("type").AsString() == "Route")
			{
				string_view from = stat_dict.at("from").AsString();
				string_view to = stat_dict.at("to").AsString();
				rh_.RouteRequestNode(builder, from, to, id);
			}
		});
	builder.EndArray();
	json::Print(json::Document(move(builder.Build())), out);
}

void RequestStorage::Serialize() const
{
	serializer_.Serialize(rh_.GetCatalogue(), rh_.GetRenderer(), rh_.GetRouter());
}

void RequestStorage::Deserialize()
{
	rh_.BuildRawTransportRouter();
	serializer_.Deserialize(rh_.GetCatalogue(), rh_.GetRenderer(), rh_.GetRouter());
}

