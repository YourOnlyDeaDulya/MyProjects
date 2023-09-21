#pragma once

class Cell;

#include "common.h"
#include "sheet.h"
#include "formula.h"
#include <optional>
#include <functional>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <deque>

class Cell : public CellInterface {
public:
    //Класс для отображения типов ячеек
    enum class CellType
    {
        EMPTY,
        TEXT,
        FORMULA
    };

    explicit Cell(const Sheet& sheet, const DependenceGraph& graph);
    Cell(const Cell& other);
    ~Cell() = default;

    void Set(std::string text);

    void SetNullImpl();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

    void AddDependentCell(Cell* dependent_cell);

    void ResetCache();

    void ReevaluateCache();

    void RemoveDependentCell(Cell* delete_cell);
    
    std::deque<Cell*> GetDependentCells() const;

    bool IsEmpty() const;

    bool IsDeleted() const;

private:
    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;

    std::shared_ptr<Impl> impl_;

    const DependenceGraph& graph_;

    std::optional<Value> value_cache_ = std::nullopt;

    std::deque<Cell*> dependent_cells_;

    const Sheet& sheet_;

    void ReevaluateDependentCache(const std::unordered_set<Cell*>& dependent_cells);

    void CollectDependentCells(Cell* cell, std::unordered_set<Cell*>& dependent_cells);

    class Impl
    {
    public:
        Impl() = default;
        Impl(CellType type);
        virtual Value GetValue() const = 0;
        virtual std::string GetText() const = 0;
        CellType GetCellType() const;
        virtual std::vector<Position> GetReferencedCells() const = 0;
        bool IsEmpty() const;
        virtual ~Impl() = default;
    private:
        CellType type_ = CellType::EMPTY;
    };

    class EmptyImpl : public Impl
    {
    public:
        EmptyImpl() = default;
        ~EmptyImpl() = default;

        Value GetValue() const override;
        std::string GetText() const override;
        std::vector<Position> GetReferencedCells() const override;
    };

    class TextImpl : public Impl
    {
    private:
        std::string raw_text_;
    public:
        TextImpl(std::string text, CellType type);
        ~TextImpl() = default;
        Value GetValue() const override;
        std::string GetText() const override;
        std::vector<Position> GetReferencedCells() const override;
    };

    class FormulaImpl : public Impl
    {
    private:
        const Sheet& sheet_;
        std::unique_ptr<FormulaInterface> formula_ = nullptr;
        std::string raw_text_;

    public:
        FormulaImpl(std::unique_ptr<FormulaInterface> formula, std::string text, CellType type, const Sheet& sheet);
        ~FormulaImpl() = default;
        Value GetValue() const override;
        std::string GetText() const override;
        std::vector<Position> GetReferencedCells() const override;
    };

};