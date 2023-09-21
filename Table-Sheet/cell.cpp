#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <algorithm>


// Реализуйте следующие методы
Cell::Cell(const Sheet& sheet, const DependenceGraph& graph) : graph_(graph), sheet_(sheet) {}
Cell::Cell(const Cell& other) : impl_(other.impl_), graph_(other.graph_), value_cache_(other.value_cache_),
								dependent_cells_(other.dependent_cells_), sheet_(other.sheet_) {}

void Cell::Set(std::string text)
{
	ReevaluateCache();

	if (text.empty())
	{
		impl_.reset();
		impl_ = std::make_shared<EmptyImpl>();
		return;
	}
	else if (text.size() > 1 && text[0] == FORMULA_SIGN)
	{
		try
		{
			std::unique_ptr<FormulaInterface> formula = ParseFormula(text.substr(1));
			auto ref_positions = formula->GetReferencedCells();
			std::vector<Cell*> ref_cells(ref_positions.size());
			std::transform(ref_positions.begin(), ref_positions.end(), ref_cells.begin(), [&](Position pos)
				{
					return sheet_.GetRawCell(pos);
				});

			if (graph_.HasCircularDependency(ref_cells, this))
			{
				throw CircularDependencyException("Error! Circular Dependency");
			}

			impl_.reset();
			impl_ = std::make_shared<FormulaImpl>(std::move(formula), std::move(text), CellType::FORMULA, sheet_);
			for (Position pos : GetReferencedCells())
			{
				if (!pos.IsValid())
				{
					throw FormulaException("Invalid position");
				}
			}
		}
		catch(const CircularDependencyException& ce)
		{
			throw ce;
		}
		catch (...)
		{
			throw FormulaException("Invalid position");
		}
	}
	else
	{
		impl_.reset();
		impl_ = std::make_shared<TextImpl>(std::move(text), CellType::TEXT);
	}
}

void Cell::SetNullImpl()
{
	impl_.reset();
}

Cell::Value Cell::GetValue() const
{
	return impl_->GetValue();
}
std::string Cell::GetText() const
{
	return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const
{
	if(impl_->GetCellType() != CellType::FORMULA)
	{
		return {};
	}

	return impl_->GetReferencedCells();
}

void Cell::AddDependentCell(Cell* dependent_cell)
{
	dependent_cells_.push_back(dependent_cell);
}

void Cell::ResetCache()
{
	value_cache_.reset();
}

void Cell::RemoveDependentCell(Cell* delete_cell)
{
	auto erase_it = dependent_cells_.begin();
	for(; erase_it != dependent_cells_.end(); ++erase_it)
	{
		if(*erase_it == delete_cell)
		{
			break;
		}
	}

	dependent_cells_.erase(erase_it);
}

std::deque<Cell*> Cell::GetDependentCells() const
{
	return dependent_cells_;
}

bool Cell::IsEmpty() const
{
	return impl_->IsEmpty();
}

bool Cell::IsDeleted() const
{
	return impl_ == nullptr;
}

void Cell::ReevaluateCache()
{
	if(!value_cache_.has_value())
	{
		return;
	}

	std::unordered_set<Cell*> dependent_cells;
	CollectDependentCells(this, dependent_cells);
	ReevaluateDependentCache(dependent_cells);
}

void Cell::ReevaluateDependentCache(const std::unordered_set<Cell*>& dependent_cells)
{
	for (Cell* cell : dependent_cells)
	{
		cell->ResetCache();
	}
}

void Cell::CollectDependentCells(Cell* cell, std::unordered_set<Cell*>& dependent_cells)
{
	if(dependent_cells.count(cell))
	{
		return;
	}

	dependent_cells.insert(cell);
	for(auto ptr : cell->GetDependentCells())
	{
		CollectDependentCells(ptr, dependent_cells);
	}
}
//-----------------------------Impl--------------------------------

Cell::Impl::Impl(CellType type) : type_(type) {}

Cell::CellType Cell::Impl::GetCellType() const
{
	return type_;
}

bool Cell::Impl::IsEmpty() const
{
	return type_ == CellType::EMPTY;
}
//-----------------------------EmptyCell---------------------------

Cell::Value Cell::EmptyImpl::GetValue() const
{
	return 0.0;
}

std::string Cell::EmptyImpl::GetText() const
{
	return "";
}

std::vector<Position> Cell::EmptyImpl::GetReferencedCells() const
{
	return {};
}

//----------------------------TextCell-----------------------------
Cell::TextImpl::TextImpl(std::string text, CellType type) : Impl(type)
{
	raw_text_ = std::move(text);
}

Cell::Value Cell::TextImpl::GetValue() const
{
	return raw_text_.size() > 0 && raw_text_[0] == ESCAPE_SIGN ?
		raw_text_.substr(1) : raw_text_;
}

std::string Cell::TextImpl::GetText() const
{
	return raw_text_;
}

std::vector<Position> Cell::TextImpl::GetReferencedCells() const
{
	return {};
}
//---------------------------FormulaCell---------------------------
Cell::FormulaImpl::FormulaImpl(std::unique_ptr<FormulaInterface> formula, std::string text, CellType type, const Sheet& sheet) : Impl(type), sheet_(sheet)
{
	raw_text_ = std::move(text);
	formula_ = std::move(formula);
}

Cell::Value Cell::FormulaImpl::GetValue() const
{
	auto val = formula_->Evaluate(sheet_);
	switch (val.index())
	{
	case 0:
		return std::get<0>(val);
		break;
	case 1:
		return std::get<1>(val);
		break;
	}

	return 0.0;
}

std::string Cell::FormulaImpl::GetText() const
{
	return "=" + formula_->GetExpression();
}

std::vector<Position> Cell::FormulaImpl::GetReferencedCells() const
{
	return formula_->GetReferencedCells();
}

