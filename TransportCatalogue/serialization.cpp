#include "serialization.h"

using namespace std;

static const int NO_BUS_EDGE = -1;
using namespace domain;

	void SerializeManager::SetSaveFile(const std::string& save_to_file)
	{
		save_to = save_to_file;
	}

	void SerializeManager::SetReadFile(const std::string& read_from_file)
	{
		read_from = read_from_file;
	}

	data_info::Stop CreateSerializedStop(Stop* stop, const transport_catalogue::TransportCatalogue& tc)
	{
		data_info::Stop proto_stop;
		vector<uint8_t> enc_name(stop->stop_name_.begin(), stop->stop_name_.end());
		proto_stop.set_stop_name(enc_name.data(), enc_name.size());
		
		proto_stop.set_id(stop->id_);
		proto_stop.set_lat(stop->lat_);
		proto_stop.set_lng(stop->lng_);

		return proto_stop;
	}

	data_info::Bus CreateSerializedBus(Bus* bus)
	{
		data_info::Bus proto_bus;
		vector<uint8_t> enc_name(bus->bus_name_.begin(), bus->bus_name_.end());
		proto_bus.set_bus_name_(enc_name.data(), enc_name.size());

		proto_bus.set_id(bus->id_);
		proto_bus.set_bus_type(static_cast<uint32_t>(bus->bus_type_));
		
		const auto& stops = bus->stops_;
		for(Stop* stop : stops)
		{
			proto_bus.add_stop_ids(stop->id_);
		}

		return proto_bus;
	}

	data_info::StopsDistancePair CreateSerializedDistancePair(const std::pair<Stop*, Stop*>& stops, size_t distance)
	{
		data_info::StopsDistancePair proto_sdp;
		proto_sdp.set_first_id(stops.first->id_);
		proto_sdp.set_second_id(stops.second->id_);
		proto_sdp.set_distance(distance);

		return proto_sdp;
	}

	data_info::Catalogue ConstructProtoCatalogue(const transport_catalogue::TransportCatalogue& tc)
	{
		data_info::StopList stop_list;

		for (int i = 0; i < tc.StopsCount(); ++i)
		{
			Stop* st = tc.FindStop(i);
			data_info::Stop proto_stop = move(CreateSerializedStop(st, tc));
			*stop_list.add_stops() = move(proto_stop);
		}

		data_info::BusList bus_list;

		const auto& buses = tc.GetBuses();
		for (Bus* b : buses)
		{
			data_info::Bus proto_bus = move(CreateSerializedBus(b));
			*bus_list.add_buses() = move(proto_bus);
		}


		data_info::Catalogue proto_cat;
		*proto_cat.mutable_stops() = move(stop_list);
		*proto_cat.mutable_buses() = move(bus_list);

		const auto& distances = tc.GetDistances();
		for (const auto& [stops, distance] : distances)
		{
			data_info::StopsDistancePair proto_sdp = CreateSerializedDistancePair(stops, distance);
			*proto_cat.add_stop_to_stop_distance() = move(proto_sdp);
		}

		return proto_cat;
	}

	proto_svg::Color ExtractColorType(const svg::Color& text_color)
	{
		proto_svg::Color proto_color;
		
		switch (text_color.index())
		{
		case 0:
			*proto_color.mutable_mono_color()->mutable_monostate() = move(google::protobuf::Empty());
			break;

		case 1:
			proto_color.set_str_color(get<std::string>(text_color));
			break;

		case 2:
			{
				proto_svg::Rgb proto_rgb;
				svg::Rgb text_rgb = get<svg::Rgb>(text_color);
				proto_rgb.set_blue(text_rgb.blue);
				proto_rgb.set_green(text_rgb.green);
				proto_rgb.set_red(text_rgb.red);
				*proto_color.mutable_rgb_color() = move(proto_rgb);
			break;
			}
		case 3:
			{
				proto_svg::Rgba proto_rgba;
				svg::Rgba text_rgba = get<svg::Rgba>(text_color);
				proto_rgba.set_blue(text_rgba.blue);
				proto_rgba.set_green(text_rgba.green);
				proto_rgba.set_red(text_rgba.red);
				proto_rgba.set_opacity(text_rgba.opacity);
				*proto_color.mutable_rgba_color() = move(proto_rgba);
			break;
			}
		}

		return proto_color;
	}

	render_sett::RenderSettings ConstructProtoRenderSett(const map_renderer::MapRenderer& mr)
	{
		render_sett::RenderSettings proto_rs;

		render_sett::PolylineSett proto_poly;
		proto_poly.set_line_width(mr.GetPolySett().line_width_);
		
		render_sett::CircleSett proto_circle;
		proto_circle.set_stop_radius(mr.GetCircleSett().stop_radius_);

		render_sett::SphereProjSett proto_sproj;
		{
			const auto& sproj_sett = mr.GetSphereProjectorSett();
			proto_sproj.set_height(sproj_sett.height_);
			proto_sproj.set_width(sproj_sett.width_);
			proto_sproj.set_padding(sproj_sett.padding_);
		}

		render_sett::TextSett proto_text;
		{
			const auto& text_sett = mr.GetTextSett();
			proto_text.set_bus_label_font_size(text_sett.bus_label_font_size_);

			render_sett::Point bus_offset_point;
			bus_offset_point.set_x(text_sett.bus_label_offset_.x);
			bus_offset_point.set_y(text_sett.bus_label_offset_.y);
			*proto_text.mutable_bus_label_offset() = move(bus_offset_point);

			proto_text.set_stop_label_font_size(text_sett.stop_label_font_size_);

			render_sett::Point stop_offset_point;
			stop_offset_point.set_x(text_sett.stop_label_offset_.x);
			stop_offset_point.set_y(text_sett.stop_label_offset_.y);
			*proto_text.mutable_stop_label_offset() = move(stop_offset_point);

			proto_svg::Color proto_underlayer_color = ExtractColorType(text_sett.underlayer_color_);
			*proto_text.mutable_underlayer_color() = move(proto_underlayer_color);

			proto_text.set_underlayer_width(text_sett.underlayer_width_);
		}
	
		render_sett::ColorPaletteSett proto_palette;
		for(const auto& color : mr.GetColorPaletteSett().color_palette_)
		{
			proto_svg::Color proto_color = ExtractColorType(color);
			*proto_palette.add_color_palette() = move(proto_color);
		}

		*proto_rs.mutable_circle() = move(proto_circle);
		*proto_rs.mutable_color_palette() = move(proto_palette);
		*proto_rs.mutable_polyline() = move(proto_poly);
		*proto_rs.mutable_text() = move(proto_text);
		*proto_rs.mutable_sph_proj() = move(proto_sproj);

		return proto_rs;
	}

	using Weight = transport_router::GraphEdgeWeight;
	proto_graph::Graph ConstructProtoGraph(const graph::DirectedWeightedGraph<Weight>& graph, const transport_catalogue::TransportCatalogue& tc)
	{
		const auto& edges = graph.GetEdges();

		proto_graph::Graph proto_gr;
		for (const auto& edge : edges)
		{
			proto_graph::Edge proto_edge;
			proto_edge.set_from(edge.from);
			proto_edge.set_to(edge.to);

			proto_edge.mutable_weight()->set_time(edge.weight.time_);
			proto_edge.mutable_weight()
				->set_edge_type(static_cast<uint32_t>(edge.weight.type_));
			proto_edge.mutable_weight()->set_span_count(edge.weight.span_count_);

			int32_t bus_id = edge.weight.bus_name_.empty() ?
				NO_BUS_EDGE : tc.FindBus(edge.weight.bus_name_)->id_;
			proto_edge.mutable_weight()->set_bus_id(bus_id);

			*proto_gr.add_edges() = move(proto_edge);
		}

		const auto& inc_lists = graph.GetIncLists();
		for(const auto& list : inc_lists)
		{
			proto_graph::IncidenceList proto_l;
			for(const auto& elem : list)
			{
				proto_l.add_edge_id(elem);
			}

			*proto_gr.add_incidence_lists() = move(proto_l);
		}

		return proto_gr;
	}

	proto_router::RouteIntDataBase ConstructProtoRIDB(const graph::Router<Weight>& router, const transport_catalogue::TransportCatalogue& tc)
	{
		proto_router::RouteIntDataBase proto_ridb;

		const auto& ridb = router.GetRID();
		for(const auto rid_vec : ridb)
		{
			proto_router::RouteIntDataLine proto_ridline;
			for(const auto& optional_rid : rid_vec)
			{
				if(!optional_rid.has_value())
				{
					*proto_ridline.add_opt_rid_file()->mutable_nullopt()
						->mutable_nullopt() = move(google::protobuf::Empty());
					continue;
				}

				const auto& rid_val = optional_rid.value();
				proto_router::RouteIntDataFile proto_ridf;

				int32_t bus_id = rid_val.weight.bus_name_.empty() ?
					NO_BUS_EDGE : tc.FindBus(rid_val.weight.bus_name_)->id_;
				proto_ridf.mutable_weight()->set_bus_id(bus_id);

				proto_ridf.mutable_weight()->set_time(rid_val.weight.time_);
				proto_ridf.mutable_weight()
					->set_edge_type(static_cast<uint32_t>(rid_val.weight.type_));
				proto_ridf.mutable_weight()->set_span_count(rid_val.weight.span_count_);

				if(rid_val.prev_edge.has_value())
				{
					proto_ridf.mutable_prev_edge()->set_prev_edge(rid_val.prev_edge.value());
				}
				else
				{
					*proto_ridf.mutable_prev_edge()
						->mutable_nullopt()->mutable_nullopt() = move(google::protobuf::Empty());
				}

				*proto_ridline.add_opt_rid_file()->mutable_rid_file() = move(proto_ridf);
			}

			*proto_ridb.add_rid_lines() = move(proto_ridline);
		}

		return proto_ridb;
	}

	proto_router::TransportRouter ConstructProtoRouter(const transport_catalogue::TransportCatalogue& tc, const transport_router::TransportRouter& tr)
	{
		proto_router::TransportRouter proto_tr;
		proto_tr.set_bus_velocity(tr.GetVelocity());
		proto_tr.set_bus_wait_time(tr.GetWaitTime());

		proto_graph::Graph proto_gr = ConstructProtoGraph(tr.GetGraph(), tc);
		proto_router::RouteIntDataBase proto_ridb = ConstructProtoRIDB(tr.GetRouter(), tc);

		*proto_tr.mutable_graph() = move(proto_gr);
		*proto_tr.mutable_routes_int_data() = move(proto_ridb);

		return proto_tr;
	}

	void SerializeManager::Serialize(const transport_catalogue::TransportCatalogue& tc, const map_renderer::MapRenderer& mr, const transport_router::TransportRouter& tr) const
	{
		ofstream out(save_to, ios::binary);
		
		data_info::Catalogue proto_cat = ConstructProtoCatalogue(tc);
		render_sett::RenderSettings proto_rs = ConstructProtoRenderSett(mr);
		proto_router::TransportRouter proto_tr = ConstructProtoRouter(tc, tr);

		data_info::SerializePackage proto_package;
		*proto_package.mutable_cat() = move(proto_cat);
		*proto_package.mutable_rs() = move(proto_rs);
		*proto_package.mutable_tr() = move(proto_tr);

		proto_package.SerializeToOstream(&out);
	}

	void DeserializeStops(const data_info::StopList& proto_stops, transport_catalogue::TransportCatalogue& tc)
	{
		for(int i = 0; i < proto_stops.stops_size(); ++i)
		{
			const auto& proto_stop = proto_stops.stops().at(i);
			Stop stop;
			stop.id_ = proto_stop.id();
			stop.lat_ = proto_stop.lat();
			stop.lng_ = proto_stop.lng();
			stop.stop_name_ = proto_stop.stop_name();
			tc.AddStop(stop, {});
		}
	}

	void DeserializeBuses(const data_info::BusList& proto_buses, transport_catalogue::TransportCatalogue& tc)
	{
		for(int i = 0; i < proto_buses.buses_size(); ++i)
		{
			const auto& proto_bus = proto_buses.buses().at(i);
			Bus bus;
			bus.bus_name_ = proto_bus.bus_name_();
			bus.bus_type_ = static_cast<BusType>(proto_bus.bus_type());
			bus.id_ = proto_bus.id();
			
			for(int y = 0; y < proto_bus.stop_ids_size(); ++y)
			{
				size_t stop_id = proto_bus.stop_ids().at(y);
				Stop* stop_ptr = tc.FindStop(stop_id);
				bus.stops_.push_back(stop_ptr);
			}

			tc.AddBus(bus);
		}
	}

	void DeserializeCatalogue(const data_info::Catalogue& proto_cat, transport_catalogue::TransportCatalogue& tc)
	{
		DeserializeStops(proto_cat.stops(), tc);

		for (int i = 0; i < proto_cat.stop_to_stop_distance_size(); ++i)
		{
			std::pair<Stop*, Stop*> stops;
			size_t distance;

			const auto& proto_sdp = proto_cat.stop_to_stop_distance().at(i);
			stops.first = tc.FindStop(proto_sdp.first_id());
			stops.second = tc.FindStop(proto_sdp.second_id());

			distance = proto_sdp.distance();

			tc.SetRawDistance(stops, distance);
		}

		DeserializeBuses(proto_cat.buses(), tc);
	}

	svg::Color DeserializeColor(const proto_svg::Color& proto_color)
	{
		svg::Color svg_color;
		
		if(proto_color.has_mono_color())
		{
			svg_color = std::monostate();
		}
		else if(proto_color.has_rgba_color())
		{
			svg::Rgba rgba;
			const auto& proto_rgba = proto_color.rgba_color();
			rgba.blue = proto_rgba.blue();
			rgba.green = proto_rgba.green();
			rgba.red = proto_rgba.red();
			rgba.opacity = proto_rgba.opacity();

			svg_color = rgba;
		}
		else if(proto_color.has_rgb_color())
		{
			svg::Rgb rgb;
			const auto& proto_rgb = proto_color.rgb_color();
			rgb.blue = proto_rgb.blue();
			rgb.green = proto_rgb.green();
			rgb.red = proto_rgb.red();

			svg_color = rgb;
		}
		else
		{
			svg_color = proto_color.str_color();
		}

		return svg_color;
	}


	void DeserializeMaRenderer(const render_sett::RenderSettings& rs, map_renderer::MapRenderer& mr)
	{
		using namespace map_renderer::image_settings;
		{
			Polyline poly_sett;
			poly_sett.line_width_ = rs.polyline().line_width();
			mr.SetPoly(poly_sett);
		}
		
		{
			Circle circ_sett;
			circ_sett.stop_radius_ = rs.circle().stop_radius();
			mr.SetCircle(circ_sett);
		}

		{
			SphereProjector sproj_sett;
			sproj_sett.height_ = rs.sph_proj().height();
			sproj_sett.width_ = rs.sph_proj().width();
			sproj_sett.padding_ = rs.sph_proj().padding();
			mr.SetSP(sproj_sett);
		}

		{
			Text text_sett;
			const auto& proto_text = rs.text();
			text_sett.bus_label_font_size_ = proto_text.bus_label_font_size();
			{
				text_sett.bus_label_offset_.x = proto_text.bus_label_offset().x();
				text_sett.bus_label_offset_.y = proto_text.bus_label_offset().y();
			}

			text_sett.stop_label_font_size_ = proto_text.stop_label_font_size();
			{
				text_sett.stop_label_offset_.x = proto_text.stop_label_offset().x();
				text_sett.stop_label_offset_.y = proto_text.stop_label_offset().y();
			}

			text_sett.underlayer_color_ = DeserializeColor(proto_text.underlayer_color());
			text_sett.underlayer_width_ = proto_text.underlayer_width();

			mr.SetText(text_sett);
		}

		{
			ColorPalette cp;
			for(int i = 0; i < rs.color_palette().color_palette_size(); ++i)
			{
				cp.color_palette_.push_back(DeserializeColor(rs.color_palette().color_palette().at(i)));
			}

			mr.SetCP(cp);
		}
	}

	void DeserializeGraph(const proto_graph::Graph& proto_gr, transport_router::TransportRouter& tr, const transport_catalogue::TransportCatalogue& tc)
	{
		auto& graph = tr.GetGraph();
		auto& edges = graph.GetMutableEdges();

		const auto& proto_edges = proto_gr.edges();
		for(const auto& proto_edge : proto_edges)
		{
			graph::Edge<Weight> edge;
			edge.from = proto_edge.from();
			edge.to = proto_edge.to();

			if(proto_edge.weight().bus_id() == NO_BUS_EDGE)
			{
				edge.weight.bus_name_ = "";
			}
			else
			{
				Bus* bus_ptr = tc.FindBus(proto_edge.weight().bus_id());
				edge.weight.bus_name_ = bus_ptr->bus_name_;
			}
			
			edge.weight.span_count_ = proto_edge.weight().span_count();
			edge.weight.time_ = proto_edge.weight().time();
			edge.weight.type_ = static_cast<transport_router::EdgeType>(proto_edge.weight().edge_type());
			
			edges.push_back(move(edge));
		}

		const auto& proto_inc_lists = proto_gr.incidence_lists();
		vector<vector<graph::EdgeId>> inc_lists;
		for(const auto& proto_list : proto_inc_lists)
		{
			vector<graph::EdgeId> inc_list;
			for(const auto& proto_id : proto_list.edge_id())
			{
				inc_list.push_back(proto_id);
			}

			inc_lists.push_back(move(inc_list));
		}
		graph.GetMutableIncLists() = move(inc_lists);
		tr.SetGraph(move(graph));
	}

	void DeserializeRIDB(const proto_router::RouteIntDataBase& proto_ridb, transport_router::TransportRouter& tr, const transport_catalogue::TransportCatalogue& tc)
	{
		vector<vector<optional<graph::Router<Weight>::RID>>> router_rid;
		for (const auto& proto_rid_line : proto_ridb.rid_lines())
		{
			vector<optional<graph::Router<Weight>::RID>> rid_line;
			for(const auto& proto_ridf : proto_rid_line.opt_rid_file())
			{
				if(proto_ridf.has_nullopt())
				{
					rid_line.push_back(nullopt);
					continue;
				}

				graph::Router<Weight>::RID ridf;
				if (proto_ridf.rid_file().weight().bus_id() == NO_BUS_EDGE)
				{
					ridf.weight.bus_name_ = "";
				}
				else
				{
					Bus* bus_ptr = tc.FindBus(proto_ridf.rid_file().weight().bus_id());
					ridf.weight.bus_name_ = bus_ptr->bus_name_;
				}
				ridf.weight.span_count_ = proto_ridf.rid_file().weight().span_count();
				ridf.weight.time_ = proto_ridf.rid_file().weight().time();
				ridf.weight.type_ = static_cast<transport_router::EdgeType>(proto_ridf.rid_file().weight().edge_type());

				if(!proto_ridf.rid_file().prev_edge().has_nullopt())
				{
					ridf.prev_edge.emplace(proto_ridf.rid_file().prev_edge().prev_edge());
				}

				rid_line.push_back(move(ridf));
			}

			router_rid.push_back(move(rid_line));
		}
		
		tr.BuildRawRouter(move(router_rid));
	}

	void DeserializeTransRouter(const proto_router::TransportRouter& proto_tr, transport_router::TransportRouter& tr, const transport_catalogue::TransportCatalogue& tc)
	{
		tr.SetVelocity(proto_tr.bus_velocity());
		tr.SetWaitTime(proto_tr.bus_wait_time());

		DeserializeGraph(proto_tr.graph(), tr, tc);
		DeserializeRIDB(proto_tr.routes_int_data(), tr, tc);
	}

	void SerializeManager::Deserialize(transport_catalogue::TransportCatalogue& tc, map_renderer::MapRenderer& mr, transport_router::TransportRouter& tr) const
	{
		ifstream in(read_from, ios::binary);
		data_info::SerializePackage proto_package;

		proto_package.ParseFromIstream(&in);

		DeserializeCatalogue(proto_package.cat(), tc);
		DeserializeMaRenderer(proto_package.rs(), mr);
		DeserializeTransRouter(proto_package.tr(), tr, tc);
	}