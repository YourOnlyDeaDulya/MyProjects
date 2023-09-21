#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

bool DependenceGraph::Count(Cell* cell) const
{
    return adj_list.count(cell);
}

void DependenceGraph::AddEdge(Cell* from, Cell* to)
{
    adj_list[from].push_back(to);
}

void DependenceGraph::AddRawVertex(Cell* from)
{
    adj_list[from];
}

void DependenceGraph::DeleteDependence(Cell* delete_cell)
{
    adj_list.erase(delete_cell);
}

bool DependenceGraph::HasCircularDependency(const std::vector<Cell*>& starting_cells, Cell* candidate_cell) const
{
    std::unordered_set<Cell*> visited;
    for (Cell* starting_cell : starting_cells)
    {
        if (visited.count(starting_cell))
        {
            continue;
        }
        else if (starting_cell == candidate_cell || DFS_search(starting_cell, candidate_cell, visited))
        {
            return true;
        }
    }
    return false;
}

bool DependenceGraph::DFS_search(Cell* starting_cell, Cell* candidate_cell, std::unordered_set<Cell*>& visited) const
{
    visited.insert(starting_cell);
    if (!adj_list.count(starting_cell))
    {
        return false;
    }

    for (Cell* ref_cell : adj_list.at(starting_cell))
    {
        if (candidate_cell == ref_cell || (!visited.count(ref_cell) && DFS_search(ref_cell, candidate_cell, visited)))
        {
            return true;
        }
    }

    return false;
}

std::vector<Cell*> DependenceGraph::GetDependenceList(Cell* cell) const
{
    if (!adj_list.count(cell))
    {
        return {};
    }

    return adj_list.at(cell);
}


Sheet::Sheet() {}
Sheet::~Sheet() {}

bool Sheet::CheckForRangeAndResize(Position pos)
{
    bool exists = true;
    if (sheet_rows_.size() <= static_cast<size_t>(pos.row))
    {
        size_t prev_size = sheet_rows_.size();
        sheet_rows_.resize(pos.row + 1);
        std::fill_n(sheet_rows_.begin() + prev_size, sheet_rows_.size() - prev_size, Row(sheet_rows_[0].columns_.size(), *this, graph_));
        exists = false;
    }

    if (sheet_rows_[0].columns_.size() <= static_cast<size_t>(pos.col))
    {
        for (auto& row : sheet_rows_)
        {
            size_t prev_size = row.columns_.size();
            row.columns_.resize(pos.col + 1);
            for (; prev_size < row.columns_.size(); ++prev_size)
            {
                row.columns_[prev_size] = std::make_unique<Cell>(*this, graph_);
            }
        }

        filled_columns_cells_count_.resize(pos.col + 1);
        exists = false;
    }

    return exists;
}

void Sheet::Resize(Size size)
{
    sheet_rows_[0].columns_.resize(size.cols);
    sheet_rows_.resize(size.rows);

    filled_columns_cells_count_.resize(size.cols);
}

void Sheet::SetCell(Position pos, std::string text)
{
    if (!pos.IsValid())
    {
        throw InvalidPositionException("Error! Invalid Position");
    }

    if (IsInRange(pos) && GetCell(pos) != nullptr && GetCell(pos)->GetText() == text)
    {
        return;
    }

    Size prev_size = { static_cast<int>(sheet_rows_.size()), static_cast<int>(filled_columns_cells_count_.size()) };
    CheckForRangeAndResize(pos);

    auto& row = sheet_rows_[pos.row];
    auto& candidate_cell = row.columns_[pos.col];

    try
    {
        candidate_cell->Set(text);
    }
    catch (const FormulaException& fe)
    {
        if (!(prev_size == Size{ (int)sheet_rows_.size(), (int)filled_columns_cells_count_.size() }))
        {
            Resize(prev_size);
        }
        throw fe;
    }
    catch (const CircularDependencyException& ce)
    {
        if (!(prev_size == Size{ (int)sheet_rows_.size(), (int)filled_columns_cells_count_.size() }))
        {
            Resize(prev_size);
        }
        throw ce;
    }

    for (Cell* prev_ref_cell : graph_.GetDependenceList(candidate_cell.get()))
    {
        prev_ref_cell->RemoveDependentCell(candidate_cell.get());
    }

    auto ref_positions = candidate_cell->GetReferencedCells();
    std::vector<Cell*> ref_cells;
    for (Position ref_pos : ref_positions)
    {
        CheckForRangeAndResize(ref_pos);
        const std::unique_ptr<Cell>& ref_cell = sheet_rows_[ref_pos.row].columns_[ref_pos.col];
        if (ref_cell->IsDeleted())
        {
            ref_cell->Set("");
        }
        ref_cell->AddDependentCell(candidate_cell.get());
        ref_cells.push_back(ref_cell.get());
    }

    SetCellGraphDependency(ref_cells, candidate_cell.get());

    ++row.filled_cells_;
    ++filled_columns_cells_count_[pos.col];

    if (pos.row >= min_print_size_.rows)
    {
        min_print_size_.rows = pos.row + 1;
    }

    if (pos.col >= min_print_size_.cols)
    {
        min_print_size_.cols = pos.col + 1;
    }
}

const CellInterface* Sheet::GetCell(Position pos) const
{
    if (!pos.IsValid())
    {
        throw InvalidPositionException("Invalid Position");
    }

    if (!IsInRange(pos))
    {
        return nullptr;
    }

    const Cell* cell = sheet_rows_[pos.row].columns_[pos.col].get();
    return cell->IsDeleted() ? nullptr : cell;
}

CellInterface* Sheet::GetCell(Position pos)
{
    if (!pos.IsValid())
    {
        throw InvalidPositionException("Invalid Position");
    }

    if (!IsInRange(pos))
    {
        return nullptr;
    }

    Cell* cell = sheet_rows_[pos.row].columns_[pos.col].get();
    return cell->IsDeleted() ? nullptr : cell;
}

void Sheet::ClearCell(Position pos)
{
    if (!pos.IsValid())
    {
        throw InvalidPositionException("Invalid Position");
    }

    if (!IsInRange(pos))
    {
        return;
    }

    auto& row = sheet_rows_[pos.row];
    auto& cell = row.columns_[pos.col];
    if (cell->IsDeleted())
    {
        return;
    }

    if (!cell->IsEmpty())
    {
        --row.filled_cells_;
        --filled_columns_cells_count_[pos.col];
    }

    for (Cell* dep_cell : graph_.GetDependenceList(cell.get()))
    {
        dep_cell->RemoveDependentCell(cell.get());
    }
    cell->ReevaluateCache();

    graph_.DeleteDependence(cell.get());
    cell->SetNullImpl();

    if (row.filled_cells_ == 0 && pos.row == min_print_size_.rows - 1)
    {
        auto row_id = pos.row;
        while (row_id >= 0 && sheet_rows_[row_id--].filled_cells_ == 0)
        {
            --min_print_size_.rows;
        }
    }

    if (filled_columns_cells_count_[pos.col] == 0 && pos.col == min_print_size_.cols - 1)
    {
        auto col_id = pos.col;
        while (col_id >= 0 && filled_columns_cells_count_[col_id--] == 0)
        {
            --min_print_size_.cols;
        }
    }
}

Size Sheet::GetPrintableSize() const
{
    return min_print_size_;
}

void Sheet::PrintValues(std::ostream& output) const
{
    for (int row = 0; row < min_print_size_.rows; ++row)
    {
        bool first = true;

        for (int col = 0; col < min_print_size_.cols; ++col)
        {
            if (!first)
            {
                output << "\t";
            }

            const auto& cell = sheet_rows_[row].columns_[col];
            if (!cell->IsDeleted() && !cell->IsEmpty())
            {
                const auto& val = sheet_rows_[row].columns_[col]->GetValue();
                switch (val.index())
                {
                case 0:
                    output << std::get<0>(val);
                    break;
                case 1:
                    output << std::get<1>(val);
                    break;
                case 2:
                    output << std::get<2>(val);
                }
            }

            first = false;
        }

        output << "\n";
    }
}
void Sheet::PrintTexts(std::ostream& output) const
{
    for (int row = 0; row < min_print_size_.rows; ++row)
    {
        bool first = true;

        for (int col = 0; col < min_print_size_.cols; ++col)
        {
            if (!first)
            {
                output << "\t";
            }

            const auto& cell = sheet_rows_[row].columns_[col];
            if (!cell->IsDeleted() && !cell->IsEmpty())
            {
                output << sheet_rows_[row].columns_[col]->GetText();

            }

            first = false;
        }

        output << "\n";
    }
}

bool Sheet::IsInRange(Position pos) const
{
    return sheet_rows_.size() > static_cast<size_t>(pos.row) && sheet_rows_[0].columns_.size() > static_cast<size_t>(pos.col);
}

Sheet::Row::Row(size_t cols, const Sheet& sheet, DependenceGraph& graph) : columns_(cols)
{
    for (auto& ptr : columns_)
    {
        ptr = std::make_unique<Cell>(sheet, graph);
    }
}

Sheet::Row::Row(const Row& other)
{
    for (const auto& ptr : other.columns_) {
        columns_.push_back(std::make_unique<Cell>(*ptr));
    }
    filled_cells_ = other.filled_cells_;
}

Sheet::Row& Sheet::Row::operator=(Row&& other)
{
    if (this == &other)
    {
        return *this;
    }

    for (auto& ptr : other.columns_) {
        columns_.push_back(std::move(ptr));
    }
    filled_cells_ = std::move(other.filled_cells_);

    return *this;
}

Sheet::Row& Sheet::Row::operator=(const Row& other)
{
    if (this == &other)
    {
        return *this;
    }

    columns_.clear();
    for (const auto& ptr : other.columns_) {
        columns_.push_back(std::make_unique<Cell>(*ptr));
    }
    filled_cells_ = other.filled_cells_;

    return *this;
}

void Sheet::SetCellGraphDependency(const std::vector<Cell*>& cells, Cell* starting_cell)
{
    if (graph_.Count(starting_cell))
    {
        graph_.DeleteDependence(starting_cell);
    }

    for (auto ref_cell : cells)
    {
        graph_.AddEdge(starting_cell, ref_cell);
        if (!graph_.Count(ref_cell))
        {
            graph_.AddRawVertex(ref_cell);
        }
    }
}

Cell* Sheet::GetRawCell(Position pos) const
{
    if (!pos.IsValid())
    {
        throw InvalidPositionException("Invalid Position");
    }

    if (!IsInRange(pos))
    {
        return nullptr;
    }

    return sheet_rows_[pos.row].columns_[pos.col].get();

}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}