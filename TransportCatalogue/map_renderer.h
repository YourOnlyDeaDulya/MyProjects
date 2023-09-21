#pragma once

#include "svg.h"
#include "geo.h"
#include "domain.h"
#include "transport_catalogue.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <vector>


namespace map_renderer
{
    namespace image_settings
    {
        struct SphereProjector
        {
            SphereProjector& operator=(const SphereProjector& other);
            SphereProjector(const SphereProjector& other);
            SphereProjector() = default;

            double width_;
            double height_;
            double padding_;
        };

        struct Polyline
        {
            Polyline& operator=(const Polyline& other);
            Polyline(const Polyline& other);
            Polyline() = default;

            double line_width_;
        };

        struct Circle
        {
            Circle& operator=(const Circle& other);
            Circle(const Circle& other);
            Circle() = default;

            double stop_radius_;
        };

        struct Text
        {
            Text& operator=(const Text& other);
            Text(const Text& other);
            Text() = default;

            uint32_t bus_label_font_size_;
            svg::Point bus_label_offset_;

            uint32_t stop_label_font_size_;
            svg::Point stop_label_offset_;

            svg::Color underlayer_color_;
            double  underlayer_width_; //задает stroke_width;
        };            

        struct ColorPalette
        {
            ColorPalette& operator=(const ColorPalette& other);
            ColorPalette(const ColorPalette& other);
            ColorPalette() = default;

            std::vector<svg::Color> color_palette_;
        };
    }


    inline const double EPSILON = 1e-6;
    bool IsZero(double value);

    class SphereProjector {
    public:
        // points_begin и points_end задают начало и конец интервала элементов geo::Coordinates
        template <typename PointInputIt>
        SphereProjector(PointInputIt points_begin, PointInputIt points_end,
            double max_width, double max_height, double padding)
            : padding_(padding) //
        {
            // Если точки поверхности сферы не заданы, вычислять нечего
            if (points_begin == points_end) {
                return;
            }

            // Находим точки с минимальной и максимальной долготой
            const auto [left_it, right_it] = std::minmax_element(
                points_begin, points_end,
                [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
            min_lon_ = left_it->lng;
            const double max_lon = right_it->lng;

            // Находим точки с минимальной и максимальной широтой
            const auto [bottom_it, top_it] = std::minmax_element(
                points_begin, points_end,
                [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
            const double min_lat = bottom_it->lat;
            max_lat_ = top_it->lat;

            // Вычисляем коэффициент масштабирования вдоль координаты x
            std::optional<double> width_zoom;
            if (!IsZero(max_lon - min_lon_)) {
                width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
            }

            // Вычисляем коэффициент масштабирования вдоль координаты y
            std::optional<double> height_zoom;
            if (!IsZero(max_lat_ - min_lat)) {
                height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
            }

            if (width_zoom && height_zoom) {
                // Коэффициенты масштабирования по ширине и высоте ненулевые,
                // берём минимальный из них
                zoom_coeff_ = std::min(*width_zoom, *height_zoom);
            }
            else if (width_zoom) {
                // Коэффициент масштабирования по ширине ненулевой, используем его
                zoom_coeff_ = *width_zoom;
            }
            else if (height_zoom) {
                // Коэффициент масштабирования по высоте ненулевой, используем его
                zoom_coeff_ = *height_zoom;
            }
        }

        // Проецирует широту и долготу в координаты внутри SVG-изображения
        svg::Point operator()(geo::Coordinates coords) const {
            return {
                (coords.lng - min_lon_) * zoom_coeff_ + padding_,
                (max_lat_ - coords.lat) * zoom_coeff_ + padding_
            };
        }

    private:
        double padding_;
        double min_lon_ = 0;
        double max_lat_ = 0;
        double zoom_coeff_ = 0;
    };

    class MapRenderer
    {
    public:

        MapRenderer() = default;

        MapRenderer(const image_settings::Polyline& poly_sett, const image_settings::Circle& circle_sett,
                    const image_settings::SphereProjector& sproj_sett, const image_settings::Text& text_sett,
                    const image_settings::ColorPalette& palette_sett);
        
        MapRenderer& operator=(const MapRenderer& other);
        MapRenderer(const MapRenderer& other);

        //Создаем кривую линию по координатам
        template<typename StopCoordIt>
        svg::Polyline CreateBusPolyline(StopCoordIt begin, StopCoordIt end, const SphereProjector& proj) const
        {
            using namespace svg;
            Polyline p;

            std::vector<geo::Coordinates> coords(end - begin);
            transform(begin, end, coords.begin(), [&](domain::Stop* stop)
                {
                    geo::Coordinates c;
                    c.lat = stop->lat_;
                    c.lng = stop->lng_;
                    return c;
                });

            for(const auto& coord : coords)
            {
                p.AddPoint(proj(coord));
            }

            p.SetStrokeWidth(poly_sett_.line_width_);
            p.SetStrokeLineCap(StrokeLineCap::ROUND).SetStrokeLineJoin(StrokeLineJoin::ROUND);
            return p;
        }

        svg::Circle CreateStopCircle(domain::Stop* stop, const SphereProjector& proj) const;
        svg::Text CreateBusName(domain::Stop* stop, const std::string& name, const SphereProjector& proj, const svg::Color& color, bool lay = false) const;
        svg::Text CreateStopName(domain::Stop* stop, const SphereProjector& proj, bool lay = false) const;

        //Создаем SphereProjector
        template<typename ContainerIt>
        SphereProjector CreateProjector(ContainerIt begin, ContainerIt end) const
        {
            std::vector<geo::Coordinates> coords(std::distance(begin, end));
            transform(begin, end, coords.begin(), [&](domain::Stop* stop)
                {   
                    geo::Coordinates c;
                    c.lat = stop->lat_;
                    c.lng = stop->lng_;
                    return c;
                });

            double w = sproj_sett_.width_;
            double h = sproj_sett_.height_;
            double pad = sproj_sett_.padding_;
            return SphereProjector(coords.begin(), coords.end(), w, h, pad);
        }

        void SetPoly(const image_settings::Polyline& new_p);
        void SetCircle(const image_settings::Circle& new_c);
        void SetSP(const image_settings::SphereProjector& new_s);
        void SetText(const image_settings::Text& new_t);
        void SetCP(const image_settings::ColorPalette& new_cp);

        svg::Color SetBusColor(size_t index, svg::Polyline& p) const;

        svg::Document RenderMap(const transport_catalogue::TransportCatalogue& catalogue) const;

        const image_settings::Polyline& GetPolySett() const;
        const image_settings::Circle& GetCircleSett() const;
        const image_settings::SphereProjector& GetSphereProjectorSett() const;
        const image_settings::Text& GetTextSett() const;
        const image_settings::ColorPalette& GetColorPaletteSett() const;

    private:
        image_settings::Polyline poly_sett_;
        image_settings::Circle circle_sett_;
        image_settings::SphereProjector sproj_sett_;
        image_settings::Text text_sett_;
        image_settings::ColorPalette palette_sett_;

        void AddBusLabels(std::vector<svg::Text>& bus_labels, const std::vector<domain::Stop*>& stops, domain::Bus* r, svg::Color e, const map_renderer::SphereProjector& proj) const;

    };
}