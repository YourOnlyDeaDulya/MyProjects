#include "json_builder.h"

using namespace std;
using namespace json;

namespace json
{
//--------------------- Controller -------------------
	Controller::Controller(Builder& builder)
		: builder_(builder)
	{
	}
	Controller::Controller(const Controller& other)
		: builder_(other.builder_)
	{
	}
	json::Node Controller::Build()
	{
		return builder_.Build();
	}
	StartDictController Controller::StartDict()
	{
		return builder_.StartDict();
	}
	ArrayController Controller::StartArray()
	{
		return builder_.StartArray();
	}
	Controller Controller::EndDict()
	{
		return builder_.EndDict();
	}
	Controller Controller::EndArray()
	{
		return builder_.EndArray();
	}
	KeyController Controller::Key(std::string key)
	{
		return builder_.Key(move(key));
	}
	Controller Controller::Value(json::Node::Value value)
	{
		return builder_.Value(value);
	}

//--------------------- KeyController ----------------

	KeyController::KeyController(Builder& builder)
		: Controller(builder)
	{
	}
	DictValueController KeyController::Value(json::Node::Value value)
	{
		return DictValueController(builder_.Value(value));
	}

//--------------------- DictValueController ----------
	DictValueController::DictValueController(Builder & builder)
		: Controller(builder)
	{}
	DictValueController::DictValueController(const Controller& controller)
		: Controller(controller)
	{}
//--------------------- StartDictController ----------
	StartDictController::StartDictController(Builder& builder)
		: Controller(builder)
	{
	}
//--------------------- StartArrayController ---------
	ArrayController::ArrayController(Builder& builder)
		: Controller(builder)
	{
	}
	ArrayController::ArrayController(const Controller& controller)
		: Controller(controller)
	{}

	ArrayController ArrayController::Value(json::Node::Value value)
	{
		return ArrayController(builder_.Value(value));
	}
//--------------------- Builder ----------------------
	KeyController Builder::Key(string key)
	{
		if (built == true || node_stack_.empty() || !node_stack_.back()->IsDict())
		{
			throw logic_error("calling \"Key\" command outside of dictionary");
		}

		else if (command_log_.back() == CommandType::KEY)
		{
			throw logic_error("calling one \"Key\" command after another");
		}

		Node& parent_node = *(node_stack_.back());
		Dict& parent_dict = const_cast<Dict&>(get<Dict>(parent_node.GetValue()));
		last_key_val_ptr_ = &(parent_dict[move(key)]);
		command_log_.push_back(CommandType::KEY);

		return KeyController(*this);
	}

	Controller Builder::Value(Node::Value value)
	{
		if (node_stack_.empty())
		{
			if (built == true)
			{
				throw logic_error("calling \"Value\" command after object is built");
			}

			Node::Value& value_node = const_cast<Node::Value&>(root_obj_.GetValue());
			value_node.swap(value);
			built = true;
			command_log_.push_back(CommandType::VALUE);
		}

		else if (node_stack_.back()->IsDict())
		{
			if (command_log_.back() != CommandType::KEY)
			{
				throw logic_error("calling \"Value\" command not after \"Key\"");
			}

			Node::Value& value_node = const_cast<Node::Value&>(last_key_val_ptr_->GetValue());
			value_node.swap(value);
			command_log_.push_back(CommandType::VALUE);
		}

		else if (node_stack_.back()->IsArray())
		{
			Node& parent_node = *(node_stack_.back());
			Array& parent_arr = const_cast<Array&>(get<Array>(parent_node.GetValue()));
			parent_arr.emplace_back(Node{});
			Node::Value& value_node = const_cast<Node::Value&>(parent_arr.back().GetValue());
			value_node.swap(value);
			command_log_.push_back(CommandType::VALUE);
		}

		command_log_.push_back(CommandType::VALUE);
		return Controller(*this);
	}

	StartDictController Builder::StartDict()
	{
		StartContainer(move(Dict{}), 2);
		command_log_.push_back(CommandType::START_DICT);
		return *this;
	}

	ArrayController Builder::StartArray()
	{
		StartContainer(move(Array{}), 1);
		command_log_.push_back(CommandType::START_ARRAY);
		return *this;
	}

	Controller Builder::EndDict()
	{
		if (built == true || node_stack_.empty() || !node_stack_.back()->IsDict())
		{
			throw logic_error("calling \"End_Dict\" command in wrong place");
			//можно разделить на разные ветви, чтобы прописать каждой ошибке свое объяснение
			// - вызов после конструктора, при готовом объекте и не после словаря
		}

		node_stack_.pop_back();
		if (node_stack_.empty())
		{
			built = true;
		}

		command_log_.push_back(CommandType::END_DICT);
		return Controller (*this);
	}

	Controller Builder::EndArray()
	{
		if (built == true || node_stack_.empty() || !node_stack_.back()->IsArray())
		{
			throw logic_error("calling \"End_Array\" command in wrong place");
			//можно разделить на разные ветви, чтобы прописать каждой ошибке свое объяснение
			// - вызов после конструктора, при готовом объекте и не после массива
		}

		node_stack_.pop_back();
		if (node_stack_.empty())
		{
			built = true;
		}

		command_log_.push_back(CommandType::END_ARRAY);
		return Controller(*this);
	}

	json::Node Builder::Build()
	{
		if (!built)
		{
			throw logic_error("JSON_Object isn`t built yet");
		}

		last_key_val_ptr_ = nullptr;

		return root_obj_;
	}
}