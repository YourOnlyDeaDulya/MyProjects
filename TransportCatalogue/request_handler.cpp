#include "request_handler.h"


using namespace std;
using namespace transport_catalogue;

namespace request_handler
{
    RequestHandler::RequestHandler(TransportCatalogue& catalogue, map_renderer::MapRenderer& renderer)
        : catalogue_(catalogue), renderer_(renderer)
    {
    }
 
    //Возвращает информацию о маршруте (запрос Bus)
    const BusData RequestHandler::GetBusStat(string_view bus_name) const
    {
        return catalogue_.BusInfo(bus_name);
    }

    //Возвращает информацию об остановке (запрос Stop)
    const StopData RequestHandler::GetBusesByStop(string_view stop_name) const
    {
        return catalogue_.StopInfo(stop_name);
    }

    //Собираем в Node-вид информацию по запросу остановки
    void RequestHandler::StopRequestNode(json::Builder& builder, StopData data, int id) const
    {
        using namespace json;

        if (data.info_type_ == StopInfoType::NOT_FOUND)
        {
            builder.StartDict().Key("request_id").Value(id).Key("error_message").Value(string("not found")).EndDict();
            return;
        }

        builder.StartDict().Key("buses").StartArray();
        for(const auto& str : data.stop_buses_)
        {
            builder.Value(string(str));
        }

        builder.EndArray().Key("request_id").Value(id).EndDict();

        return;
    }

    //Собираем в Node-вид информацию о маршруте
    void RequestHandler::BusRequestNode(json::Builder& builder, BusData data, int id) const
    {
        using namespace json;

        if (data.info_type_ == BusInfoType::NOT_FOUND)
        {
            builder.StartDict().Key("request_id").Value(id).Key("error_message").Value(string("not found")).EndDict();
            return;
        }

        builder.StartDict()
            .Key("curvature").Value(data.curvature_)
            .Key("request_id").Value(id)
            .Key("route_length").Value(data.bus_length_)
            .Key("stop_count").Value(static_cast<int>(data.stop_count_))
            .Key("unique_stop_count").Value(static_cast<int>(data.unique_stops_)).EndDict();
        return;
    }

    void RequestHandler::RouteRequestNode(json::Builder& builder, std::string_view from, std::string_view to, int id) const
    {
        using namespace json;

        size_t from_id = catalogue_.FindStop(from)->id_;
        size_t to_id = catalogue_.FindStop(to)->id_;

        auto route = tr_->BuildRoute(from_id, to_id);

        if(!route.has_value())
        {
            builder.StartDict()
                               .Key("request_id").Value(id)
                               .Key("error_message").Value(string("not found"))
                   .EndDict();

            return;
        }

        builder.StartDict()
            .Key("request_id").Value(id)
            .Key("total_time").Value(route->weight.time_)
            .Key("items").StartArray();

        for(const auto& edge_id : route->edges)
        {
            auto edge = tr_->GetEdge(edge_id);
            builder.StartDict()
                .Key("type");

            if(edge.weight.type_ == transport_router::EdgeType::WAIT)
            {
                builder.Value(string("Wait"))
                    .Key("stop_name")
                    .Value(catalogue_.FindStop(edge.to)->stop_name_)
                    .Key("time").Value(edge.weight.time_)
                    .EndDict();
            }
            else
            {
                builder.Value(string("Bus"))
                    .Key("bus").Value(string(edge.weight.bus_name_))
                    .Key("span_count").Value(int(edge.weight.span_count_))
                    .Key("time").Value(edge.weight.time_)
                    .EndDict();
            }
        }

        builder.EndArray().EndDict();
    }

    svg::Document RequestHandler::RenderMap() const
    {
        return renderer_.RenderMap(catalogue_);
    }
    void RequestHandler::BuildRawTransportRouter()
    {
        tr_ = make_unique<transport_router::TransportRouter>();
    }
    transport_catalogue::TransportCatalogue& RequestHandler::GetCatalogue() const
    {
        return catalogue_;
    }
    transport_router::TransportRouter& RequestHandler::GetRouter() const
    {
        return *tr_;
    }
    map_renderer::MapRenderer& RequestHandler::GetRenderer() const
    {
        return renderer_;
    }

    //Считываем и добавляем остановку в каталог по запросу
    void RequestHandler::LoadStopRequest(const json::Node& stop_node) const
    {
        const auto& stop_dict = stop_node.AsDict();

        string stop_name = stop_dict.at("name").AsString();
        double lat = stop_dict.at("latitude").AsDouble();
        double lng = stop_dict.at("longitude").AsDouble();

        const auto& distance_dict = stop_dict.at("road_distances").AsDict();
        vector<pair<string, size_t>> distances(distance_dict.size());
        transform(distance_dict.begin(), distance_dict.end(), distances.begin(),
            [&](const pair<string, json::Node>& p)
            {
                string stop_name = p.first;
                size_t distance = static_cast<size_t>(p.second.AsInt());
                return pair<string, size_t>{ std::move(stop_name), distance };
            });

        catalogue_.AddStop(std::move(stop_name), lat, lng, std::move(distances));
    }

    //Считываем и добавляем маршрут в каталог по запросу
    void RequestHandler::LoadBusRequest(const json::Node& bus_node) const
    {
        const auto& bus_dict = bus_node.AsDict();

        string bus_name = bus_dict.at("name").AsString();
        const auto& stops_arr = bus_dict.at("stops").AsArray();

        vector<string> stops(stops_arr.size());
        transform(stops_arr.begin(), stops_arr.end(), stops.begin(), [&](const json::Node& node)
            {
                return node.AsString();
            });

        bool type_bool = bus_dict.at("is_roundtrip").AsBool();
        domain::BusType type = type_bool == true ? domain::BusType::CIRCULAR : domain::BusType::LINEAR;

        catalogue_.AddBus(std::move(bus_name), std::move(stops), type);
    }

    //Считываем формат цвета
    svg::Color ParseColor(const json::Node& color_node)
    {
        if (color_node.IsString())
        {
            return svg::Color(color_node.AsString());
        }

        const auto& color_arr = color_node.AsArray();
        if (color_arr.size() == 3)
        {
            return svg::Color
            (svg::Rgb(
                static_cast<uint8_t>(color_arr[0].AsInt()),
                static_cast<uint8_t>(color_arr[1].AsInt()),
                static_cast<uint8_t>(color_arr[2].AsInt())));
        }

        return svg::Color
        (svg::Rgba(
            static_cast<uint8_t>(color_arr[0].AsInt()),
            static_cast<uint8_t>(color_arr[1].AsInt()),
            static_cast<uint8_t>(color_arr[2].AsInt()),
            color_arr[3].AsDouble()
        ));
    }

    //Считываем настройки MapRenderer
    void RequestHandler::ReadRendererSettings(const json::Document& storage_) const
    {
        using namespace map_renderer::image_settings;
        const auto& rd = storage_.GetRoot().AsDict().at("render_settings").AsDict();

        Polyline p;
        {
            p.line_width_ = rd.at("line_width").AsDouble();
            renderer_.SetPoly(p);
        }

        SphereProjector sp;
        {
            sp.height_ = rd.at("height").AsDouble();
            sp.width_ = rd.at("width").AsDouble();
            sp.padding_ = rd.at("padding").AsDouble();
            renderer_.SetSP(sp);
        }

        Circle c;
        {
            c.stop_radius_ = rd.at("stop_radius").AsDouble();
            renderer_.SetCircle(c);
        }

        Text t;
        {
            t.bus_label_font_size_ = rd.at("bus_label_font_size").AsInt();

            auto b_off_arr = rd.at("bus_label_offset").AsArray();
            t.bus_label_offset_ = { b_off_arr[0].AsDouble(), b_off_arr[1].AsDouble() };
            t.stop_label_font_size_ = rd.at("stop_label_font_size").AsDouble();

            auto s_off_arr = rd.at("stop_label_offset").AsArray();
            t.stop_label_offset_ = { s_off_arr[0].AsDouble(), s_off_arr[1].AsDouble() };
            t.underlayer_width_ = rd.at("underlayer_width").AsDouble();

            const json::Node& color_node = rd.at("underlayer_color");
            t.underlayer_color_ = ParseColor(color_node);
            renderer_.SetText(t);
        }

        ColorPalette pal;
        {
            const auto& palette_arr = rd.at("color_palette").AsArray();
            std::vector<svg::Color> colors;
            for (const auto& color : palette_arr)
            {
                colors.push_back(ParseColor(color));
            }
            pal.color_palette_ = colors;
            renderer_.SetCP(pal);
        }
    }

    void RequestHandler::ReadRouterSettingsAndBuild(const json::Dict& settings)
    {
        tr_ = make_unique<transport_router::TransportRouter>(catalogue_.StopsCount());
        tr_->SetVelocity(settings.at("bus_velocity").AsDouble());
        tr_->SetWaitTime(settings.at("bus_wait_time").AsDouble());
        tr_->BuildGraph(catalogue_);
        tr_->BuildRouter();
    }
}