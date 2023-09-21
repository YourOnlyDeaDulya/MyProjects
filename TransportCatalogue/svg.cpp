#include "svg.h"


namespace svg {

    using namespace std::literals;

    std::ostream& operator<<(std::ostream& out, StrokeLineCap line_cap)
    {
        switch (line_cap)
        {
        case StrokeLineCap::BUTT:
            out << "butt";
            break;

        case StrokeLineCap::ROUND:
            out << "round";
            break;

        case StrokeLineCap::SQUARE:
            out << "square";
            break;
        }

        return out;
    }

    std::ostream& operator<<(std::ostream& out, StrokeLineJoin join) {
        switch (join)
        {
        case StrokeLineJoin::ARCS:
            out << "arcs";
            break;

        case StrokeLineJoin::BEVEL:
            out << "bevel";
            break;
        case StrokeLineJoin::MITER:
            out << "miter";
            break;
        case StrokeLineJoin::MITER_CLIP:
            out << "miter-clip";
            break;
        case StrokeLineJoin::ROUND:
            out << "round";
        }

        return out;
    }

    Rgb::Rgb(uint8_t r, uint8_t g, uint8_t b)
        : red(r), green(g), blue(b)
    {
    }

    Rgba::Rgba(uint8_t r, uint8_t g, uint8_t b, double o)
        : red(r), green(g), blue(b), opacity(o)
    {
    }

    void ColorPrinter::operator()(std::monostate) const
    {
        out << "none";
    }

    void ColorPrinter::operator()(std::string str) const
    {
        out << str;
    }

    void ColorPrinter::operator()(svg::Rgb rgb) const
    {
        out << "rgb(" << std::to_string(rgb.red) << "," << std::to_string(rgb.green) << "," << std::to_string(rgb.blue) << ")";
    }

    void ColorPrinter::operator()(svg::Rgba rgba) const
    {
        out << "rgba(" << std::to_string(rgba.red) << "," << std::to_string(rgba.green) << "," << std::to_string(rgba.blue) << "," << rgba.opacity << ")";
    }

    void Object::Render(const RenderContext& context) const {
        context.RenderIndent();

        // Делегируем вывод тега своим подклассам
        RenderObject(context);

        context.out << std::endl;
    }
    // ---------- Drawable ----------------
    // ---------- Circle ------------------

    Circle& Circle::SetCenter(Point center) {
        center_ = center;
        return *this;
    }

    Circle& Circle::SetRadius(double radius) {
        radius_ = radius;
        return *this;
    }

    void Circle::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
        out << "r=\""sv << radius_ << "\""sv;
        RenderAttrs(out);
        out << "/>"sv;
    }

    // ------------ Polyline ---------------

    Polyline& Polyline::AddPoint(Point p)
    {
        points_.emplace_back(std::move(p));
        return *this;
    }

    void Polyline::RenderObject(const RenderContext& context) const
    {
        auto& out = context.out;
        out << "<polyline points=\""sv;
        bool first = true;
        for (const auto& point : points_)
        {
            if (first)
            {
                out << point.x << "," << point.y;
                first = false;
            }
            else
            {
                out << " "sv << point.x << "," << point.y;
            }
        }

        out << "\"";
        RenderAttrs(out);
        out << "/>"sv;
    }
    // ------------ Text -------------------

    Text& Text::SetPosition(Point pos)
    {
        text_coord_ = pos;
        return *this;
    }

    Text& Text::SetOffset(Point offset)
    {
        offset_ = offset;
        return *this;
    }

    Text& Text::SetFontSize(uint32_t size)
    {
        size_ = size;
        return *this;
    }

    Text& Text::SetFontFamily(std::string font_family)
    {
        font_family_ = font_family;
        return *this;
    }

    Text& Text::SetFontWeight(std::string font_weight)
    {
        font_weight_ = font_weight;
        return *this;
    }

    Text& Text::SetData(std::string data)
    {
        data_ = data;
        return *this;
    }

    void Text::RenderObject(const RenderContext& context) const
    {
        auto& out = context.out;
        out << "<text x=\""sv << text_coord_.x << "\" y=\""sv << text_coord_.y
            << "\" dx=\""sv << offset_.x << "\" dy=\""sv << offset_.y
            << "\" font-size=\""sv << size_ << "\""sv;
        if (!font_family_.empty())
        {
            out << " font-family=\""sv << font_family_ << "\""sv;
        }

        if (!font_weight_.empty())
        {
            out << " font-weight=\""sv << font_weight_ << "\""sv;
        }

        RenderAttrs(out);
        out << ">"sv;

        for (const auto& ch : data_)
        {
            switch (ch)
            {
            case '"':
                out << "& quot;"sv;
                break;

            case '\'':
                out << "&apos;"sv;
                break;

            case '<':
                out << "&lt;"sv;
                break;

            case '>':
                out << "&gt;"sv;
                break;

            case '&':
                out << "&amp;"sv;
                break;

            default:
                out << ch;
                break;
            }
        }

        out << "</text>"sv;
    }

    // ------------ Document ---------------

    void Document::AddPtr(std::unique_ptr<Object>&& obj)
    {
        documents_.emplace_back(std::move(obj));
    }

    void Document::Render(std::ostream& out) const
    {
        RenderContext render(out);
        auto& o = render.out;
        o << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv << std::endl
            << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv << std::endl;
        for (const auto& ptr : documents_)
        {
            ptr->Render(render);
        }

        o << "</svg>"sv;
    }

    //--------------------------------------------

}  // namespace svg