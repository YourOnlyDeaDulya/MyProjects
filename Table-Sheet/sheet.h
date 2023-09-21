#pragma once

class Sheet;
class DependenceGraph;

#include "cell.h"
#include "common.h"

#include <functional>
#include <deque>
#include <memory>
#include <unordered_set>


class DependenceGraph
{
private:

    std::unordered_map<Cell*, std::vector<Cell*>> adj_list;
public:

    bool Count(Cell* cell) const;

    void AddEdge(Cell* from, Cell* to);

    void AddRawVertex(Cell* from);

    void DeleteDependence(Cell* delete_cell);

    std::vector<Cell*> GetDependenceList(Cell* cell) const;

    bool HasCircularDependency(const std::vector<Cell*>& starting_cells, Cell* candidate_cell) const;

    bool DFS_search(Cell* starting_cell, Cell* candidate_cell, std::unordered_set<Cell*>& visited) const;
};

class Sheet : public SheetInterface {
private:

    struct Row
    {
        Row() = default;
        Row(size_t cols, const Sheet& sheet, DependenceGraph& graph);
        Row(const Row& other);
        Row& operator=(Row&& other);
        Row& operator=(const Row& other);
        std::deque<std::unique_ptr<Cell>> columns_;
        size_t filled_cells_ = 0;
    };

public:
    Sheet();
    ~Sheet();


    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;
    Cell* GetRawCell(Position pos) const;

private:

    std::deque<Row> sheet_rows_;

    std::deque<size_t> filled_columns_cells_count_;

    Size min_print_size_;

    DependenceGraph graph_;

    bool CheckForRangeAndResize(Position pos);

    void Resize(Size size);

    bool IsInRange(Position pos) const;

    void SetCellGraphDependency(const std::vector<Cell*>& cells, Cell* starting_cell);
};