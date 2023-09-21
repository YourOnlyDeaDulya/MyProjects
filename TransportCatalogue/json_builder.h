#pragma once

#include "json.h"
#include <memory>
#include <algorithm>

namespace json
{
	class Controller;
	class KeyController;
	class DictValueController;
	class StartDictController;
	class ArrayController;

	class Builder
	{
	private:

		enum class CommandType
		{
			KEY,
			VALUE,
			START_DICT,
			START_ARRAY,
			END_DICT,
			END_ARRAY,
		};

	public:

		Builder() = default;

		json::Node Build();

		KeyController Key(std::string);
		Controller Value(json::Node::Value);
		StartDictController StartDict();
		ArrayController StartArray();
		Controller EndDict();
		Controller EndArray();

	private:
		json::Node root_obj_;
		Node* last_key_val_ptr_;
		std::vector<json::Node*> node_stack_;
		std::vector<CommandType> command_log_;
		bool built = false;

		template<typename ContainerType>
		void StartContainer(ContainerType value, size_t index)
		{
			if (node_stack_.empty() && built == true)
			{
				throw std::logic_error("calling \"Start_Container\" command in wrong place");
			}

			if (node_stack_.empty())
			{
				json::Node::Value& value_node = const_cast<json::Node::Value&>(root_obj_.GetValue());

				switch (index)
				{
				case 1:
					value_node.emplace<1>(std::move(json::Array{}));
					break;

				case 2:
					value_node.emplace<2>(std::move(json::Dict{}));
					break;
				}

				node_stack_.emplace_back(&root_obj_);
			}

			else if (node_stack_.back()->IsArray())
			{
				json::Node& parent_node = *(node_stack_.back());
				json::Array& parent_arr = const_cast<json::Array&>(std::get<json::Array>(parent_node.GetValue()));

				parent_arr.emplace_back(Node(std::move(value)));
				node_stack_.emplace_back(&parent_arr.back());
			}

			else if (node_stack_.back()->IsDict())
			{
				if (command_log_.back() != CommandType::KEY)
				{
					throw std::logic_error("calling \"Start_Container\" command not after \"Key\"");
				}

				json::Node::Value& value_node = const_cast<json::Node::Value&>(last_key_val_ptr_->GetValue());

				switch (index)
				{
				case 1:
					value_node.emplace<1>(std::move(json::Array{}));
					break;

				case 2:
					value_node.emplace<2>(std::move(json::Dict{}));
					break;
				}

				node_stack_.emplace_back(last_key_val_ptr_);
			}
		}
	};

	class Controller
	{
	public:
		Controller(Builder& builder);
		Controller(const Controller& other);
		virtual ~Controller() = default;
		json::Node Build();
		StartDictController StartDict();
		ArrayController StartArray();
		Controller EndDict();
		Controller EndArray();
		KeyController Key(std::string key);
		Controller Value(json::Node::Value value);
	protected:
		Builder& builder_;
	};

	class KeyController : public Controller
	{
	public:
		KeyController(Builder& builder);
		DictValueController Value(json::Node::Value value);
	private:
		using Controller::Build;
		using Controller::EndArray;
		using Controller::EndDict;
		using Controller::Key;
	};

	class DictValueController : public Controller
	{
	public:
		DictValueController(Builder& builder);
		DictValueController(const Controller& controller);
	private:
		using Controller::Build;
		using Controller::EndArray;
		using Controller::StartArray;
		using Controller::StartDict;
		using Controller::Value;
	};

	class StartDictController : public Controller 
	{
	public:
		StartDictController(Builder& builder);
	private:
		using Controller::Build;
		using Controller::EndArray;
		using Controller::StartArray;
		using Controller::StartDict;
		using Controller::Value;
	};

	class ArrayController : public Controller
	{
	public:
		ArrayController(Builder& builder);
		ArrayController(const Controller& controller);
		ArrayController Value(json::Node::Value value);
	private:
		using Controller::Build;
		using Controller::EndDict;
		using Controller::Key;

	};
}