#pragma once

#include "transport_catalogue.h"
#include "json_builder.h"
#include "svg.h"
#include "map_renderer.h"
#include "transport_router.h"
#include <memory>
#include <optional>


using namespace transport_catalogue;

namespace request_handler
{
    class RequestHandler {
    public:

        RequestHandler() = default;

        // MapRenderer понадобится в следующей части итогового проекта
        RequestHandler(TransportCatalogue& catalogue, map_renderer::MapRenderer& renderer);
        // Возвращает информацию о маршруте (запрос Bus)
        const BusData GetBusStat(std::string_view bus_name) const;

        // Возвращает маршруты, проходящие через
        const StopData GetBusesByStop(std::string_view stop_name) const;

        void StopRequestNode(json::Builder& builder, StopData data, int id) const;

        void BusRequestNode(json::Builder& builder, BusData data, int id) const;

        void RouteRequestNode(json::Builder& builder, std::string_view from, std::string_view to, int id) const;

        // Этот метод будет нужен в следующей части итогового проекта
        svg::Document RenderMap() const;

        transport_catalogue::TransportCatalogue& GetCatalogue() const;
        transport_router::TransportRouter& GetRouter() const;
        map_renderer::MapRenderer& GetRenderer() const;
        void BuildRawTransportRouter();
        void LoadStopRequest(const json::Node& stop_node) const;
        void LoadBusRequest(const json::Node& bus_node) const;
        void ReadRendererSettings(const json::Document& storage_) const;
        void ReadRouterSettingsAndBuild(const json::Dict& settings);
    private:

        // RequestHandler использует агрегацию объектов "Транспортный Справочник" и "Визуализатор Карты"
        TransportCatalogue& catalogue_;
        map_renderer::MapRenderer& renderer_;
        std::unique_ptr<transport_router::TransportRouter> tr_;
    };
}
