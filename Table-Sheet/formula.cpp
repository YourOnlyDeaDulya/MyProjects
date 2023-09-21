#include "formula.h"
#include "FormulaAST.h"
#include "cell.h"
#include "sheet.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << fe.ToString();
}

namespace {
    class Formula : public FormulaInterface {
    public:

        explicit Formula(std::string expression) : ast_(ParseFormulaAST(expression)) {}
        Value Evaluate(const SheetInterface& sheet) const override
        {
            try
            {
                return ast_.Execute(sheet);
            }
            catch (const FormulaError& e)
            {
                return { e };
            }
        }
        std::string GetExpression() const override
        {
            std::ostringstream o;
            ast_.PrintFormula(o);
            return o.str();
        }

        std::vector<Position> GetReferencedCells() const override
        {
            const auto& cell_list = ast_.GetCells();
            std::vector<Position> result { cell_list.begin(), cell_list.end() };
            std::sort(result.begin(), result.end());
            auto erase_it = std::unique(result.begin(), result.end());
            result.erase(erase_it, result.end());

            return result;
        }

    private:
        FormulaAST ast_;
    };
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    try
    {
        return std::make_unique<Formula>(std::move(expression));
    }
    catch(...)
    {
        throw FormulaException("Invalid Formula");
    }
}