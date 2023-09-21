#include "map_renderer.h"

using namespace std;

namespace map_renderer
{
    bool IsZero(double value) {
        return std::abs(value) < EPSILON;
    }

    namespace image_settings
    {
        SphereProjector& SphereProjector::operator=(const SphereProjector& other)
        {
            if (this == &other) {
                return *this;
            }
            height_ = other.height_;
            width_ = other.width_;
            padding_ = other.padding_;

            return *this;
        }

        SphereProjector::SphereProjector(const SphereProjector& other)
        {
            *this = other;
        }

        Polyline& Polyline::operator=(const Polyline& other)
        {
            if (this == &other) {
                return *this;
            }
            line_width_ = other.line_width_;
            return *this;
        }

        Polyline::Polyline(const Polyline& other)
        {
            *this = other;
        }

        Circle& Circle::operator=(const Circle& other)
        {
            if (this == &other) {
                return *this;
            }
            stop_radius_ = other.stop_radius_;
            return *this;
        }

        Circle::Circle(const Circle& other)
        {
            *this = other;
        }

        Text& Text::operator=(const Text& other)
        {
            if (this == &other) {
                return *this;
            }
            bus_label_font_size_ = other.bus_label_font_size_;
            bus_label_offset_ = other.bus_label_offset_;
            stop_label_font_size_ = other.stop_label_font_size_;
            stop_label_offset_ = other.stop_label_offset_;
            underlayer_color_ = other.underlayer_color_;
            underlayer_width_ = other.underlayer_width_;
            return *this;
        }

        Text::Text(const Text& other)
        {
            *this = other;
        }

        ColorPalette& ColorPalette::operator=(const ColorPalette& other)
        {
            if (this == &other) {
                return *this;
            }
            color_palette_ = other.color_palette_;
            return *this;
        }

        ColorPalette::ColorPalette(const ColorPalette& other)
        {
            *this = other;
        }
    }

    MapRenderer::MapRenderer(const image_settings::Polyline& poly_sett, const image_settings::Circle& circle_sett,
        const image_settings::SphereProjector& sproj_sett, const image_settings::Text& text_sett,
        const image_settings::ColorPalette& palette_sett)
        : poly_sett_(poly_sett), circle_sett_(circle_sett), sproj_sett_(sproj_sett), text_sett_(text_sett), palette_sett_(palette_sett)
    {
    }

    MapRenderer& MapRenderer::operator=(const MapRenderer& other)
    {
        if (this == &other) {
            return *this;
        }
        circle_sett_ = other.circle_sett_;
        palette_sett_ = other.palette_sett_;
        poly_sett_ = other.poly_sett_;
        sproj_sett_ = other.sproj_sett_;
        text_sett_ = other.text_sett_;
        return *this;
    }

    MapRenderer::MapRenderer(const MapRenderer& other)
    {
        *this = other;
    }

    //Создает окружность по настройкам для конкретной остановки
    svg::Circle MapRenderer::CreateStopCircle(domain::Stop* stop, const SphereProjector& proj) const
    {
        svg::Circle circle;
        geo::Coordinates c;
        c.lat = stop->lat_;
        c.lng = stop->lng_;
        circle.SetCenter(proj(c)).SetRadius(circle_sett_.stop_radius_).SetFillColor("white");
        return circle;
    }

    //Cоздает текст названия маршрута
    svg::Text MapRenderer::CreateBusName(domain::Stop* stop, const std::string& name, const SphereProjector& proj, const svg::Color& color, bool lay) const
    {
        svg::Text t;
        geo::Coordinates c;
        c.lat = stop->lat_;
        c.lng = stop->lng_;
        svg::Point screen_pos = proj(c);
        t.SetData(name).SetPosition(screen_pos).SetFontFamily("Verdana").SetFontSize(text_sett_.bus_label_font_size_)
         .SetFontWeight("bold").SetOffset(text_sett_.bus_label_offset_);

        if(lay)
        {
            t.SetFillColor(text_sett_.underlayer_color_).SetStrokeColor(text_sett_.underlayer_color_)
                .SetStrokeLineCap(svg::StrokeLineCap::ROUND).SetStrokeLineJoin(svg::StrokeLineJoin::ROUND)
                .SetStrokeWidth(text_sett_.underlayer_width_);
        }
        else
        {
            t.SetFillColor(color);
        }
        return t;
    }

    //Создаем текст названия остановки
    svg::Text MapRenderer::CreateStopName(domain::Stop* stop, const SphereProjector& proj, bool lay) const
    {
        svg::Text t; 
        geo::Coordinates c;
        c.lat = stop->lat_;
        c.lng = stop->lng_;
        t.SetData(stop->stop_name_).SetPosition(proj(c)).SetOffset(text_sett_.stop_label_offset_)
            .SetFontSize(text_sett_.stop_label_font_size_).SetFontFamily("Verdana");

        if(lay)
        {
            t.SetFillColor(text_sett_.underlayer_color_).SetStrokeColor(text_sett_.underlayer_color_)
                .SetStrokeWidth(text_sett_.underlayer_width_).SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
        }
        else
        {
            t.SetFillColor("black");
        }

        return t;
    }

    //Определяем цвет маршрута по его индексу
    svg::Color MapRenderer::SetBusColor(size_t index, svg::Polyline& p) const
    {
        using namespace svg;
        const auto& colors = palette_sett_.color_palette_;
        Color color = colors.at(index % colors.size());

        p.SetStrokeColor(color);
        p.SetFillColor(NoneColor);

        return color;
    }

    //Вычисляем только те остановки, через которые проходит хотя бы один маршрут
    vector<domain::Stop*> CalculateUsedStops(const vector<domain::Stop*>& stops)
    {
        vector<domain::Stop*> stop_nodes(stops.size());
        auto it = std::copy_if(stops.begin(), stops.end(), stop_nodes.begin(), [](Stop* stop)
            {
                return !(stop->buses_.empty());
            });
        stop_nodes.erase(it, stop_nodes.end());

        return stop_nodes;
    }

    //Собираем вектор названий маршрутов в лексографическом порядке
    void MapRenderer::AddBusLabels(vector<svg::Text>& bus_labels, const vector<domain::Stop*>& stops, Bus* r, svg::Color color, const map_renderer::SphereProjector& proj) const
    {
        svg::Text t1_lay = CreateBusName(stops[0], r->bus_name_, proj, color, true);
        svg::Text t1 = CreateBusName(stops[0], r->bus_name_, proj, color);
        bus_labels.push_back(t1_lay);
        bus_labels.push_back(t1);
        if (r->bus_type_ == BusType::LINEAR && stops[0] != stops[(stops.size() - 1) / 2])
        {
            svg::Text t2_lay = CreateBusName(stops[(stops.size() - 1) / 2], r->bus_name_, proj, color, true);
            svg::Text t2 = CreateBusName(stops[(stops.size() - 1) / 2], r->bus_name_, proj, color);
            bus_labels.push_back(t2_lay);
            bus_labels.push_back(t2);
        }
    }

    //Собираем нашу карту-документ: сначала линии, затем - названия маршрутов, затем кольца и названия остановок
    svg::Document MapRenderer::RenderMap(const transport_catalogue::TransportCatalogue& catalogue) const
    {
        svg::Document svg_map;

        vector<domain::Stop*> stop_nodes = CalculateUsedStops(catalogue.GetStops());
        std::sort(stop_nodes.begin(), stop_nodes.end(), [](Stop* a, Stop* b)
            {
                return a->stop_name_ < b->stop_name_;
            });
        map_renderer::SphereProjector proj = CreateProjector(stop_nodes.begin(), stop_nodes.end());

        //Вектор названий маршрутов
        vector<svg::Text> bus_labels;

        //Создаем и добавляем линии маршрутов
        auto buses = catalogue.GetBusesNameList();
        std::sort(buses.begin(), buses.end());
        for (auto it = buses.begin(); it != buses.end(); ++it)
        {
            size_t bus_index = distance(buses.begin(), it);
            domain::Bus* r = catalogue.FindBus(*it);
            const auto& stops = r->stops_;
            if (r->stops_.empty())
            {
                continue;
            }

            svg::Polyline p = CreateBusPolyline(stops.begin(), stops.end(), proj);
            auto color = SetBusColor(bus_index, p);
            svg_map.AddPtr(make_unique<svg::Polyline>(p));

            AddBusLabels(bus_labels, stops, r, color, proj);
        }

        //Добавляем названия маршрутов
        for (const auto& label : bus_labels)
        {
            svg_map.AddPtr(make_unique<svg::Text>(label));
        }

        //Добавляем круги остановок
        for (const auto& stop : stop_nodes)
        {
            svg::Circle c = CreateStopCircle(stop, proj);
            svg_map.AddPtr(make_unique<svg::Circle>(c));
        }

        //Добавляем названия остановок
        for (const auto& stop : stop_nodes)
        {
            svg::Text t_lay = CreateStopName(stop, proj, true);
            svg::Text t = CreateStopName(stop, proj);

            svg_map.AddPtr(make_unique<svg::Text>(t_lay));
            svg_map.AddPtr(make_unique<svg::Text>(t));
        }

        return svg_map;
    }

    void MapRenderer::SetPoly(const image_settings::Polyline& new_p)
    {
        poly_sett_ = new_p;
    }
    void MapRenderer::SetCircle(const image_settings::Circle& new_c)
    {
        circle_sett_ = new_c;
    }
    void MapRenderer::SetSP(const image_settings::SphereProjector& new_s)
    {
        sproj_sett_ = new_s;
    }
    void MapRenderer::SetText(const image_settings::Text& new_t)
    {
        text_sett_ = new_t;
    }
    void MapRenderer::SetCP(const image_settings::ColorPalette& new_cp)
    {
        palette_sett_ = new_cp;
    }

    const image_settings::Polyline& MapRenderer::GetPolySett() const
    {
        return poly_sett_;
    }

    const image_settings::Circle& MapRenderer::GetCircleSett() const
    {
        return circle_sett_;
    }
    const image_settings::SphereProjector& MapRenderer::GetSphereProjectorSett() const
    {
        return sproj_sett_;
    }
    const image_settings::Text& MapRenderer::GetTextSett() const
    {
        return text_sett_;
    }
    const image_settings::ColorPalette& MapRenderer::GetColorPaletteSett() const
    {
        return palette_sett_;
    }
}