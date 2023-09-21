#include "transport_catalogue.h"
#include "json_reader.h"
#include "json_builder.h"
#include "request_handler.h"
#include "serialization.h"

#include <fstream>
#include <iostream>
#include <string_view>

using namespace std::literals;

void PrintUsage(std::ostream& stream = std::cerr) {
    stream << "Usage: transport_catalogue [make_base|process_requests]\n"sv;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        PrintUsage();
        return 1;
    }

    const std::string_view mode(argv[1]);

    if (mode == "make_base"sv) {

        map_renderer::MapRenderer renderer;
        transport_catalogue::TransportCatalogue catalogue;

        RequestStorage storage(catalogue, renderer, std::cin);

        storage.ReadInputRequests();
        storage.ReadRendererSettings();
        storage.ReadRouterSettingsAndBuild();
        storage.SetSerializeToFile();
        storage.Serialize();
    }
    else if (mode == "process_requests"sv) {

        map_renderer::MapRenderer renderer;
        transport_catalogue::TransportCatalogue catalogue;

        RequestStorage storage(catalogue, renderer, std::cin);

        storage.SetDeserializeFrom();
        storage.Deserialize();

        storage.ReadOutputRequests();
    }
    else {
        PrintUsage();
        return 1;
    }
}